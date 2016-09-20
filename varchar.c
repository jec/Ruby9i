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

VALUE varchar_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   VALUE str;
   oci9_handle *parm_h;
   ub4 col_size;

   if (argc == 1)
      switch (TYPE(argv[0]))
      {
         case T_ARRAY:
            /* used by Statement to pick a type to receive column data
             * argv[0] = [ svc_h, ses_h, parm_h, ... ]
             *
             * get attributes for column_info
             */
            Data_Get_Struct(rb_ary_entry(argv[0], 2), oci9_handle, parm_h);
            if (OCIAttrGet(parm_h->ptr, OCI_DTYPE_PARAM, (dvoid*) &col_size, 0, OCI_ATTR_DATA_SIZE, err_h))
               error_raise("Could not get column size", "varchar_initialize", __FILE__, __LINE__);
            col_size &= 0x0000ffff; /* XXX high bytes getting corrupted */
            rb_hash_aset(rb_iv_get(self, "@column_info"), rb_str_new2("size"), INT2FIX(col_size));
            /* prepare to receive varchar of length col_size */
            bp->val = ALLOC_N(char, col_size + 1);
            bp->len = col_size + 1;
            break;

         case T_NIL:
            /* leave as empty string */
            break;

         default:
            /* use arg (converted to a String) as initial value */
            str = rb_funcall(argv[0], ID_TO_S, 0);
            bp->len = RSTRING(str)->len + 1;
            bp->val = ALLOC_N(char, bp->len + 1);
            strncpy(bp->val, RSTRING(str)->ptr, bp->len + 1);
            bp->ind = 0;
            break;
      }
   else
      rb_raise(rb_eArgError, "wrong # of arguments(%d for 1)", argc);

   /* read/write this as a null-terminated C string */
   bp->sqlt = SQLT_STR;

   return self;
}

VALUE varchar_to_s(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0)
      return rb_str_new2("NULL");
   return rb_str_new2(bp->val);
}

VALUE varchar_to_builtin(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0)
      return Qnil;
   return rb_str_new2(bp->val);
}

void Init_Varchar()
{
   rb_define_method(cVarchar, "initialize", varchar_initialize, -1);
   rb_enable_super(cVarchar, "initialize");
   rb_define_method(cVarchar, "to_s", varchar_to_s, 0);
   rb_define_method(cVarchar, "to_builtin", varchar_to_builtin, 0);

   /* register types handled by me */
   registry_add(TYPE_REGISTRY, SQLT_CHR, cVarchar);
   registry_add(TYPE_REGISTRY, SQLT_AFC, cVarchar);
}
