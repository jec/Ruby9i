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

#define MAX_IV_LEN 100

VALUE biv_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (OCIDescriptorAlloc(env_h, &bp->desc, bp->dtype, 0, 0))
      error_raise("OCIDescriptorAlloc failed", "biv_initialize", __FILE__, __LINE__);
   bp->len = sizeof(bp->desc);
   bp->val = &bp->desc;

   if (argc == 0)
   {
      /* create a zero interval */
      bp->ind = 0;
      VALUE zero = rb_iv_get(self, "@zero");
      if (OCIIntervalFromText(env_h, err_h, RSTRING(zero)->ptr, RSTRING(zero)->len, bp->desc))
         error_raise("OCIIntervalFromText failed", "biv_initialize", __FILE__, __LINE__);
   }

   return self;
}

VALUE biv_add(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpout, *bpobj = NULL;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(self, oci9_define_buf, bp);
   dvoid *hdl = datatype_get_session_handle(self);

   if (bp->ind != 0) { bpout->ind = bp->ind; return out; }

   /* OCI only supports adding intervals of same type */
   if (!RTEST(rb_obj_is_kind_of(obj, CLASS_OF(self))))
      rb_raise(rb_eTypeError, "no implicit conversion of type %s to %s", rb_class2name(CLASS_OF(obj)),
         rb_class2name(CLASS_OF(self)));

   Data_Get_Struct(obj, oci9_define_buf, bpobj);
   if (OCIIntervalAdd(hdl, err_h, bp->desc, bpobj->desc, bpout->desc))
      error_raise("OCIIntervalAdd failed", "biv_add", __FILE__, __LINE__);
   return out;
}

VALUE biv_subtract(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpout, *bpobj = NULL;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(self, oci9_define_buf, bp);
   dvoid *hdl = datatype_get_session_handle(self);

   if (bp->ind != 0) { bpout->ind = bp->ind; return out; }

   /* OCI only supports adding intervals of same type */
   if (!RTEST(rb_obj_is_kind_of(obj, CLASS_OF(self))))
      rb_raise(rb_eTypeError, "no implicit conversion of type %s to %s", rb_class2name(CLASS_OF(obj)),
         rb_class2name(CLASS_OF(self)));

   Data_Get_Struct(obj, oci9_define_buf, bpobj);
   if (OCIIntervalSubtract(hdl, err_h, bp->desc, bpobj->desc, bpout->desc))
      error_raise("OCIIntervalSubtract failed", "biv_subtract", __FILE__, __LINE__);
   return out;
}

VALUE biv_multiply(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpout, *bpnum = NULL;
   VALUE num, out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(self, oci9_define_buf, bp);
   dvoid *hdl = datatype_get_session_handle(self);

   if (bp->ind != 0) { bpout->ind = -1; return out; }

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
      num = datatype_new(1, &obj, cNumber);
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
      num = obj;
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   Data_Get_Struct(num, oci9_define_buf, bpnum);
   if (bpnum->ind != 0) { bpout->ind = -1; return out; }
   if (OCIIntervalMultiply(hdl, err_h, bp->desc, bpnum->val, bpout->desc))
      error_raise("OCIIntervalMultiply failed", "biv_multiply", __FILE__, __LINE__);
   return out;
}

VALUE biv_divide(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpout, *bpnum = NULL;
   VALUE num, out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(self, oci9_define_buf, bp);
   dvoid *hdl = datatype_get_session_handle(self);

   if (bp->ind != 0) { bpout->ind = -1; return out; }

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
      num = datatype_new(1, &obj, cNumber);
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
      num = obj;
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   Data_Get_Struct(num, oci9_define_buf, bpnum);
   if (bpnum->ind != 0) { bpout->ind = -1; return out; }
   if (OCIIntervalDivide(hdl, err_h, bp->desc, bpnum->val, bpout->desc))
      error_raise("OCIIntervalDivide failed", "biv_divide", __FILE__, __LINE__);
   return out;
}

VALUE biv_compare(VALUE self, VALUE obj)
{
   sword result;
   oci9_define_buf *bp, *bpobj = NULL;
   Data_Get_Struct(self, oci9_define_buf, bp);
   dvoid *hdl = datatype_get_session_handle(self);

   if (bp->ind != 0) return Qnil;

   /* OCI only supports comparing intervals of same type */
   if (!RTEST(rb_obj_is_kind_of(obj, CLASS_OF(self))))
      rb_raise(rb_eTypeError, "no implicit conversion of type %s to %s", rb_class2name(CLASS_OF(obj)),
         rb_class2name(CLASS_OF(self)));

   Data_Get_Struct(obj, oci9_define_buf, bpobj);
   if (OCIIntervalCompare(hdl, err_h, bp->desc, bpobj->desc, &result))
      error_raise("OCIIntervalCompare failed", "biv_compare", __FILE__, __LINE__);
   return INT2FIX(result);
}

VALUE biv_to_s(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return rb_str_new2("NULL");

   char buf[MAX_IV_LEN];
   size_t bufoutlen;
   if (OCIIntervalToText(env_h, err_h, bp->desc, FIX2INT(rb_funcall(self, ID_LFPRECISION, 0)),
         (ub1) FIX2INT(rb_funcall(self, ID_FSPRECISION, 0)), (OraText*) buf, MAX_IV_LEN, &bufoutlen))
      error_raise("OCIIntervalToText failed", "biv_to_s", __FILE__, __LINE__);
   return rb_str_new(buf, bufoutlen);
}

VALUE biv_to_builtin(VALUE self)
{
   oci9_define_buf *bp, *bpnum;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return Qnil;

   VALUE zero = INT2FIX(0);
   VALUE num = datatype_new(1, &zero, cNumber);
   Data_Get_Struct(num, oci9_define_buf, bpnum);
   if (OCIIntervalToNumber(env_h, err_h, bp->desc, bpnum->val))
      error_raise("OCIIntervalToNumber failed", "biv_to_builtin", __FILE__, __LINE__);

   return rb_funcall(num, ID_TO_BUILTIN, 0);
}

void Init_BaseInterval()
{
   rb_include_module(cBaseInterval, rb_mComparable);

   rb_define_method(cBaseInterval, "initialize", biv_initialize, -1);
   rb_enable_super(cBaseInterval, "initialize");
   rb_define_method(cBaseInterval, "+", biv_add, 1);
   rb_define_method(cBaseInterval, "-", biv_subtract, 1);
   rb_define_method(cBaseInterval, "*", biv_multiply, 1);
   rb_define_method(cBaseInterval, "/", biv_divide, 1);
   rb_define_method(cBaseInterval, "<=>", biv_compare, 1);
   rb_define_method(cBaseInterval, "to_s", biv_to_s, 0);
   rb_define_method(cBaseInterval, "to_builtin", biv_to_builtin, 0);
}
