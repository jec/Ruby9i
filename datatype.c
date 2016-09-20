/*
 * Ruby9i -- a Ruby library for accessing Oracle9i through the OCI
 * Copyright (C) 2003 James Edwin Cain <info@jimcain.us>
 * 
 * This file is part of the Ruby9i library.  This Library is free software;
 * you can redistribute it and/or modify it under the terms of the license
 * contained in the file LICENCE.txt. If you did not receive a copy of the
 * license, please contact the copyright holder.
 */

#include "ruby9i.h"

dvoid *datatype_get_session_handle(VALUE self)
{
   /* use the session handle if available */
   dvoid *hdl = env_h;
   VALUE v_ses_h = rb_iv_get(self, "@ses_h");
   if (RTEST(v_ses_h))
   {
      oci9_handle *ses_h;
      Data_Get_Struct(v_ses_h, oci9_handle, ses_h);
      if (ses_h->ptr)
         hdl = ses_h->ptr;
   }
   return hdl;
}

void datatype_mark(void *bp)
{
   printf("in datatype_mark for %p\n", bp); /* XXX for testing purposes to know when the gc runs */
}

void datatype_free(void *bp)
{
   /* free the descriptor for the OCI type if there is one */
   if (((oci9_define_buf*)bp)->desc)
      if (OCIDescriptorFree(((oci9_define_buf*)bp)->desc, ((oci9_define_buf*)bp)->dtype))
         error_raise("Could not free descriptor", "datatype_free", __FILE__, __LINE__);
}

VALUE datatype_select_coltype(int argc, VALUE *argv, VALUE self)
{
   ub2 col_type;
   VALUE v_svc_h, v_ses_h, v_parm_h, restary;
   oci9_handle *parm_h;

   rb_scan_args(argc, argv, "4", &v_svc_h, &v_ses_h, &v_parm_h, &restary);
   Data_Get_Struct(v_parm_h, oci9_handle, parm_h);

   /* get column type */
   if (OCIAttrGet(parm_h->ptr, OCI_DTYPE_PARAM, &col_type, 0, OCI_ATTR_DATA_TYPE, err_h))
      error_raise("Could not get column type", "datatype_select_coltype", __FILE__, __LINE__);

   return rb_ary_new3(1, INT2FIX(col_type));
}

/* used by Statement to create a suitable receiver for SELECT data */
VALUE datatype_selectnew(int argc, VALUE *argv, VALUE klass)
{
printf("in datatype_selectnew for %s\n", rb_class2name(klass));
   VALUE typehash, v_svc_h, v_ses_h, v_parm_h, restary;
   rb_scan_args(argc, argv, "5", &typehash, &v_svc_h, &v_ses_h, &v_parm_h, &restary);
printf("typehash=%s\n", RSTRING(rb_funcall(typehash, rb_intern("inspect"), 0))->ptr);

   /* ask the class to get the column type & any additional parameters */
   VALUE typeary = rb_funcall(klass, rb_intern("select_coltype"), 4, v_svc_h, v_ses_h, v_parm_h, restary);
   VALUE type = rb_ary_entry(typeary, 0);
printf("type=%d\n", FIX2INT(type));

   /* add any additional parameters to restary */
   rb_funcall(restary, rb_intern("concat"), 1, rb_funcall(typeary, ID_SUBSCRIPT, 2, INT2FIX(1), 
      INT2FIX(FIX2INT(rb_funcall(typeary, ID_SIZE, 0)) - 1)));

   /* look up class that handles type */
   VALUE dispatch_ary = rb_hash_aref(typehash, type);
   if (NIL_P(dispatch_ary))
      rb_raise(rb_eTypeError, "Unsupported SQL data type %d in SELECT list", FIX2INT(type));

   /* if 2nd arg is not nil, there are subclasses of the class, so call its #selectnew */
   VALUE subclass_hash = rb_ary_entry(dispatch_ary, 1);
   if (RTEST(subclass_hash))
   {
      VALUE argv2[] = { subclass_hash, v_svc_h, v_ses_h, v_parm_h, restary };
      return rb_funcall2(rb_ary_entry(dispatch_ary, 0), ID_SELECTNEW, 5, argv2);
   }
   /* else there are no subclasses of the class, so call its #new */
   else
   {
      /* bundle args into single Ruby array */
      VALUE ary = rb_funcall(rb_ary_new3(4, v_svc_h, v_ses_h, v_parm_h, restary), rb_intern("flatten!"), 0);
      return rb_funcall2(rb_ary_entry(dispatch_ary, 0), ID_NEW, 1, &ary);
   }
}

VALUE datatype_new(int argc, VALUE *argv, VALUE klass)
{
   oci9_define_buf *bp;
   VALUE datatype = Data_Make_Struct(klass, oci9_define_buf, datatype_mark, datatype_free, bp);
   bp->val = NULL;
   bp->desc = NULL;
   bp->dtype = 0;
   bp->len = 0;
   bp->ind = -1; /* initially NULL */
   bp->sqlt = SQLT_STR;
   rb_obj_call_init(datatype, argc, argv);
   return datatype;
}

VALUE datatype_initialize(int argc, VALUE *argv, VALUE self)
{
   if (argc == 1 && TYPE(argv[0]) == T_ARRAY)
   {
      /* used by Statement to pick a type to receive column data
       * argv[0] = [ svc_h, ses_h, parm_h, position ]
       *
       * set some instance vars
       */
      rb_iv_set(self, "@svc_h", rb_ary_entry(argv[0], 0));
      rb_iv_set(self, "@ses_h", rb_ary_entry(argv[0], 1));

      /* prepare empty column_info hash */
      rb_iv_set(self, "@column_info", rb_hash_new());
      rb_define_attr(CLASS_OF(self), "column_info", 1, 0);
   }

   return self;
}

VALUE datatype_oci_define(VALUE self, VALUE v_stmt_h, VALUE position)
{
   /* get my pointers */
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   /* get statement handle */
   oci9_handle *stmt_h;
   Data_Get_Struct(v_stmt_h, oci9_handle, stmt_h);

   /* The define handle is owned by the statement. When stmt_h is destroyed,
    * all associated def_h will be destroyed.
    */
   OCIDefine *def_h = NULL;

   /* define object as output for column at position */
   if (OCIDefineByPos(stmt_h->ptr, &def_h, err_h, FIX2INT(position), bp->val, bp->len, bp->sqlt,
         (dvoid*) &bp->ind, 0, 0, OCI_DEFAULT))
      error_raise("OCIDefineByPos failed", "datatype_oci_define", __FILE__, __LINE__);
   VALUE v_def_h = handle_wrap(def_h);
   rb_iv_set(self, "@def_h", v_def_h);

   return self;
}

/* provide read-only access to the indicator to check truncated lengths */
VALUE datatype_indicator(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   return INT2FIX(bp->ind);
}

VALUE datatype_set_null(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   bp->ind = -1;
   return self;
}

VALUE datatype_is_null(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind == 0)
      return Qfalse;
   return Qtrue;
}

/* attempt to convert to a built-in type */
VALUE datatype_to_builtin(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   /* by default return self if not null, else return nil */
   if (bp->ind == 0)
      return self;
   return Qnil;
}

void Init_Datatype()
{
   rb_define_singleton_method(cDatatype, "new", datatype_new, -1);
   rb_define_singleton_method(cDatatype, "selectnew", datatype_selectnew, -1);
   rb_define_singleton_method(cDatatype, "select_coltype", datatype_select_coltype, -1);
   rb_define_method(cDatatype, "initialize", datatype_initialize, -1);
   rb_define_method(cDatatype, "oci_define", datatype_oci_define, 2);
   rb_define_method(cDatatype, "indicator", datatype_indicator, 0);
   rb_define_method(cDatatype, "null?", datatype_is_null, 0);
   rb_define_method(cDatatype, "null", datatype_set_null, 0);
   rb_define_method(cDatatype, "to_builtin", datatype_to_builtin, 0);
}
