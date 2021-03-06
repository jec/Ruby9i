/*
 * Ruby9i -- a Ruby library for accessing Oracle9i through the OCI
 * Copyright (C) 2003-2004 James Edwin Cain <ruby9i@jimcain.us>
 * 
 * This file is part of the Ruby9i library.  This Library is free software;
 * you can redistribute it and/or modify it under the terms of the license
 * contained in the file LICENCE.txt. If you did not receive a copy of the
 * license, please contact the copyright holder.
 */

#include "ruby9i.h"

VALUE stmt_finish(VALUE self)
{
   /* get statement handle & free it */
   oci9_handle *stmt_h;
   Data_Get_Struct(rb_iv_get(self, "@stmt_h"), oci9_handle, stmt_h);
   OCIHandleFree(stmt_h->ptr, OCI_HTYPE_STMT);
   return self;
}

VALUE stmt_prepare(VALUE klass, VALUE ses_h, VALUE svc_h, VALUE sql, VALUE yieldflag)
{
   VALUE self = rb_obj_alloc(cStatement);

   /* allocate statement handle */
   VALUE v_stmt_h = handle_new(INT2FIX(OCI_HTYPE_STMT));
   oci9_handle *stmt_h;
   Data_Get_Struct(v_stmt_h, oci9_handle, stmt_h);
   rb_iv_set(self, "@stmt_h", v_stmt_h);

   /* prepare statement */
   if (OCIStmtPrepare((OCIStmt*) stmt_h->ptr, err_h, RSTRING(sql)->ptr, RSTRING(sql)->len, OCI_NTV_SYNTAX, OCI_DEFAULT))
      error_raise("Could not prepare statement", "stmt_prepare", __FILE__, __LINE__);

   /* save session & service handles from connection */
   rb_iv_set(self, "@ses_h", ses_h);
   rb_iv_set(self, "@svc_h", svc_h);

   /* save statement text & allow read-only access */
   rb_iv_set(self, "@text", sql);
   rb_define_attr(CLASS_OF(self), "text", 1, 0);

   /* get & save the statement type */
   ub2 ocitype;
   if (OCIAttrGet(stmt_h->ptr, OCI_HTYPE_STMT, &ocitype, 0, OCI_ATTR_STMT_TYPE, err_h))
      error_raise("Could not get statement type", "stmt_prepare", __FILE__, __LINE__);
   rb_iv_set(self, "@ocitype", INT2FIX((int)ocitype));
   rb_define_attr(CLASS_OF(self), "ocitype", 1, 0);

   /* get & save the function code */
   ub2 sqlfcode;
   if (OCIAttrGet(stmt_h->ptr, OCI_HTYPE_STMT, &sqlfcode, 0, OCI_ATTR_SQLFNCODE, err_h))
      error_raise("Could not get function code", "stmt_prepare", __FILE__, __LINE__);
   rb_iv_set(self, "@sqlfcode", INT2FIX((int)sqlfcode));
   rb_define_attr(CLASS_OF(self), "sqlfcode", 1, 0);

   /* initialize executing & bindlist */
   rb_iv_set(self, "@executing", Qfalse);
   rb_iv_set(self, "@bindlist", rb_ary_new());

   if (rb_block_given_p() && RTEST(yieldflag))
   {
      rb_yield(self);
      stmt_finish(self);
      return Qnil;
   }
   else
      return self;
}

VALUE stmt_make_bind_object(VALUE obj)
{
   /* if it's a Datatype object, use it directly */
   if (RTEST(rb_obj_is_kind_of(obj, cDatatype)))
      return obj;

   /* else pick an appropriate Datatype object to use */
   switch(TYPE(obj))
   {
      case T_NIL:
         return datatype_new(0, NULL, cVarchar);

      case T_FIXNUM:
      case T_BIGNUM:
      case T_FLOAT:
         return datatype_new(1, &obj, cNumber);

      default:
         /* use String representation of object */
         return datatype_new(1, &obj, cVarchar);
   }
}

VALUE stmt_bind_param(VALUE self, VALUE v_label, VALUE val)
{
   /* get statement handle */
   oci9_handle *stmt_h;
   Data_Get_Struct(rb_iv_get(self, "@stmt_h"), oci9_handle, stmt_h);

   /* if it's a Datatype object, use it directly */
   VALUE obj = stmt_make_bind_object(val);
   oci9_define_buf *bp;
   Data_Get_Struct(obj, oci9_define_buf, bp);
   OCIBind *bind_h = NULL;
   if (OCIBindByName(stmt_h->ptr, &bind_h, err_h, RSTRING(v_label)->ptr, RSTRING(v_label)->len, bp->val, bp->len,
         bp->sqlt, &bp->ind, 0, 0, 0, 0, OCI_DEFAULT))
      error_raise("Could not bind variable", "stmt_bind_param", __FILE__, __LINE__);

   /* save bind object */
   rb_ary_push(rb_iv_get(self, "@bindlist"), obj);

   return self;
}

VALUE stmt_bind_params(VALUE self, VALUE args)
{
   /* get statement handle */
   oci9_handle *stmt_h;
   Data_Get_Struct(rb_iv_get(self, "@stmt_h"), oci9_handle, stmt_h);

   oci9_define_buf *bp;
   OCIBind *bind_h;
   VALUE arg, obj;
   VALUE bindlist = rb_iv_get(self, "@bindlist");

   /* bind each object */
   if (RTEST(args))
      while(RTEST(arg = rb_ary_shift(args)))
      {
         /* create suitable bind object & bind it */
         obj = stmt_make_bind_object(arg);
         bind_h = NULL;
         Data_Get_Struct(obj, oci9_define_buf, bp);
         if (OCIBindByPos(stmt_h->ptr, &bind_h, err_h, RARRAY(bindlist)->len + 1, bp->val, bp->len, bp->sqlt, &bp->ind,
               0, 0, 0, 0, OCI_DEFAULT))
            error_raise("Could not bind variable", "stmt_bind", __FILE__, __LINE__);

         /* save bind object */
         rb_ary_push(bindlist, obj);
      }

   return self;
}

void stmt_allocate_row(VALUE self)
{
   /* get statement handle */
   oci9_handle *stmt_h;
   Data_Get_Struct(rb_iv_get(self, "@stmt_h"), oci9_handle, stmt_h);

   /* get column count */
   ub4 columncount;
   if (OCIAttrGet(stmt_h->ptr, OCI_HTYPE_STMT, &columncount, 0, OCI_ATTR_PARAM_COUNT, err_h))
      error_raise("Could not get column count", "stmt_allocate_row", __FILE__, __LINE__);

   /* build array of column info & array of objects to receive column values */
   VALUE cols = rb_ary_new2(columncount);
   VALUE selectlist = rb_ary_new2(columncount);
   VALUE col;
   int i;
   OCIParam *parm_h;
   char *col_name;
   ub4 col_name_len;
   for (i=1; i<=columncount; i++)
   {
      /* get column i parameter */
      parm_h = (OCIParam*) NULL;
      if (OCIParamGet(stmt_h->ptr, OCI_HTYPE_STMT, err_h, (dvoid **) &parm_h, i))
         error_raise("Could not get statement parameter", "stmt_allocate_row", __FILE__, __LINE__);

      /* get column name from parameter */
      if (OCIAttrGet(parm_h, OCI_DTYPE_PARAM, &col_name, &col_name_len, OCI_ATTR_NAME, err_h))
         error_raise("Could not get column name", "stmt_allocate_row", __FILE__, __LINE__);

      /* create & define a suitable receiver for this column's data;
       * add the object to the select-list array
       */
      VALUE obj = rb_funcall(cDatatype, ID_SELECTNEW, 5, TYPE_REGISTRY, rb_iv_get(self, "@svc_h"),
         rb_iv_get(self, "@ses_h"), handle_wrap(parm_h), rb_ary_new());
      rb_funcall(obj, ID_OCI_DEFINE, 2, rb_iv_get(self, "@stmt_h"), INT2FIX(i));
      rb_ary_push(selectlist, obj);

      /* put object's attributes into a hash;
       * add the hash to the column_info array
       */
      col = rb_funcall(obj, rb_intern("column_info"), 0);
      rb_hash_aset(col, rb_str_new2("name"), rb_str_new(col_name, col_name_len));
      rb_ary_push(cols, col);

   }
   rb_iv_set(self, "@column_info", cols);
   rb_define_attr(CLASS_OF(self), "column_info", 1, 0);

   /* create an array that fetch will populate & return on each call */
   rb_iv_set(self, "@selectlist", selectlist);
}

VALUE stmt_execute(VALUE self)
{
   /* get statement handle */
   oci9_handle *stmt_h;
   Data_Get_Struct(rb_iv_get(self, "@stmt_h"), oci9_handle, stmt_h);

   /* get service handle */
   oci9_handle *svc_h;
   Data_Get_Struct(rb_iv_get(self, "@svc_h"), oci9_handle, svc_h);

   /* set iteration parameter */
   int iters = 0;
   if (FIX2INT(rb_iv_get(self, "@ocitype")) != 1) /* not a SELECT */
      iters = 1;

   /* execute statement */
   int rc = OCIStmtExecute(svc_h->ptr, stmt_h->ptr, err_h, iters, 0, 0, 0, OCI_DEFAULT);
   if (rc == OCI_STILL_EXECUTING)
   {
      rb_iv_set(self, "@executing", Qtrue);
      return Qfalse;
   }
   rb_iv_set(self, "@executing", Qfalse);
   if (rc != OCI_SUCCESS && rc != OCI_SUCCESS_WITH_INFO && rc != OCI_NO_DATA)
   {
      ub2 offset;
      OCIAttrGet(stmt_h->ptr, OCI_HTYPE_STMT, &offset, 0, OCI_ATTR_PARSE_ERROR_OFFSET, err_h);
      error_raise_sql("Could not execute statement", "stmt_execute", __FILE__, __LINE__,
         RSTRING(rb_iv_get(self, "@text"))->ptr, offset);
   }

   /* if statement is a SELECT, then create the space to receive the row data */
   if (FIX2INT(rb_iv_get(self, "@ocitype")) == 1)
      stmt_allocate_row(self);

   if (rb_block_given_p())
   {
      rb_yield(self);
      stmt_finish(self);
      return Qtrue;
   }
   return Qtrue;
}

VALUE stmt_is_executing(VALUE self)
{
   return rb_iv_get(self, "@executing");
}

VALUE stmt_rows(VALUE self)
{
   /* get statement handle */
   oci9_handle *stmt_h;
   Data_Get_Struct(rb_iv_get(self, "@stmt_h"), oci9_handle, stmt_h);

   /* get row count */
   ub4 rowcount;
   if (OCIAttrGet(stmt_h->ptr, OCI_HTYPE_STMT, &rowcount, 0, OCI_ATTR_ROW_COUNT, err_h))
      error_raise("Could not get row count", "stmt_rows", __FILE__, __LINE__);

   return INT2FIX((int)rowcount);
}

int stmt_do_fetch(dvoid *stmt_h)
{
   switch (OCIStmtFetch(stmt_h, err_h, 1, OCI_FETCH_NEXT, OCI_DEFAULT))
   {
      case OCI_SUCCESS:
      case OCI_SUCCESS_WITH_INFO:
         return 1;
      case OCI_NO_DATA:
         return 0;
      case OCI_INVALID_HANDLE:
         error_raise("Could not fetch row: Invalid handle", "stmt_fetch", __FILE__, __LINE__);
      case OCI_NEED_DATA:
         error_raise("Could not fetch row: Need data", "stmt_fetch", __FILE__, __LINE__);
      default:
         error_raise("Could not fetch row", "stmt_fetch", __FILE__, __LINE__);
   }
   return 0;
}

VALUE stmt_fetch(VALUE self)
{
   /* get statement handle */
   oci9_handle *stmt_h;
   Data_Get_Struct(rb_iv_get(self, "@stmt_h"), oci9_handle, stmt_h);

   if (rb_block_given_p())
   {
      while (stmt_do_fetch(stmt_h->ptr))
         rb_yield(rb_iv_get(self, "@selectlist"));
      stmt_finish(self);
      return Qnil;
   }
   else if (stmt_do_fetch(stmt_h->ptr))
      return rb_iv_get(self, "@selectlist");
   else
      return Qnil;
}

VALUE stmt_cancel(VALUE self)
{
   /* TODO: actually do something here */
   return self;
}

VALUE stmt_break(VALUE self)
{
   oci9_handle *svc_h;
   Data_Get_Struct(rb_iv_get(self, "@svc_h"), oci9_handle, svc_h);
   if (OCIBreak(svc_h->ptr, err_h))
      error_raise("OCIBreak failed", "stmt_break", __FILE__, __LINE__);
   if (OCIReset(svc_h->ptr, err_h))
      error_raise("OCIReset failed", "stmt_break", __FILE__, __LINE__);
   return self;
}

void Init_Statement()
{
   rb_define_singleton_method(cStatement, "prepare", stmt_prepare, 4);
   rb_define_method(cStatement, "bind_param", stmt_bind_param, 2);
   rb_define_method(cStatement, "bind_params", stmt_bind_params, -2);
   rb_define_method(cStatement, "execute", stmt_execute, 0);
   rb_define_method(cStatement, "executing?", stmt_is_executing, 0);
   rb_define_method(cStatement, "rows", stmt_rows, 0);
   rb_define_method(cStatement, "fetch", stmt_fetch, 0);
   rb_define_method(cStatement, "cancel", stmt_cancel, 0);
   rb_define_method(cStatement, "break", stmt_break, 0);
   rb_define_method(cStatement, "finish", stmt_finish, 0);
}
