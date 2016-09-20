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

VALUE number_assign(VALUE self, VALUE obj)
{
   uword big;
   double dbl;
   oci9_define_buf *bp, *bpobj = NULL;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM)
   {
      bp->ind = 0;
      big = NUM2LONG(obj);
      if (OCINumberFromInt(err_h, &big, sizeof(big), OCI_NUMBER_SIGNED, bp->val))
         error_raise("OCINumberFromInt failed", "number_assign", __FILE__, __LINE__);
   }
   else if (TYPE(obj) == T_FLOAT)
   {
      bp->ind = 0;
      dbl = NUM2DBL(obj);
      if (OCINumberFromReal(err_h, &dbl, sizeof(dbl), bp->val))
         error_raise("OCINumberFromReal failed", "number_assign", __FILE__, __LINE__);
   }
   else if (TYPE(obj) == T_STRING)
   {
      bp->ind = 0;
      VALUE fmt = rb_iv_get(self, "@default_fmt");
      if (OCINumberFromText(err_h, RSTRING(obj)->ptr, RSTRING(obj)->len, RSTRING(fmt)->ptr, RSTRING(fmt)->len, 
            0, 0, bp->val))
         error_raise("OCINumberFromText failed", "number_assign", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
   {
      Data_Get_Struct(obj, oci9_define_buf, bpobj);
      bp->ind = bpobj->ind;
      if (OCINumberAssign(err_h, bpobj->val, bp->val))
         error_raise("OCINumberAssign failed", "number_assign", __FILE__, __LINE__);
   }

   return self;
}

VALUE number_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   oci9_handle *parm_h;

   sb2 precision;
   sb1 scale;

   /* initialize OCINumber */
   bp->val = ALLOC(OCINumber);
   bp->len = sizeof(OCINumber);
   bp->sqlt = SQLT_VNU;
   rb_iv_set(self, "@default_fmt", rb_str_new2("TM"));
   rb_define_attr(CLASS_OF(self), "default_fmt", 1, 0);

   if (argc == 0)
   {
      /* set to zero */
      bp->ind = 0;
      OCINumberSetZero(err_h, bp->val);
   }
   else if (argc == 1 && TYPE(argv[0]) == T_ARRAY)
   {
      /* used by Statement to pick a type to receive column data
       * argv[0] = [ svc_h, ses_h, parm_h, ... ]
       *
       * get attributes for column_info */
      Data_Get_Struct(rb_ary_entry(argv[0], 2), oci9_handle, parm_h);
      if (OCIAttrGet(parm_h->ptr, OCI_DTYPE_PARAM, (dvoid*) &precision, 0, OCI_ATTR_PRECISION, err_h))
         error_raise("Could not get column precision", "datatype_initialize", __FILE__, __LINE__);
      if (OCIAttrGet(parm_h->ptr, OCI_DTYPE_PARAM, &scale, 0, OCI_ATTR_SCALE, err_h))
         error_raise("Could not get column scale", "datatype_initialize", __FILE__, __LINE__);
      rb_hash_aset(rb_iv_get(self, "@column_info"), rb_str_new2("precision"), INT2FIX(precision));
      rb_hash_aset(rb_iv_get(self, "@column_info"), rb_str_new2("scale"), INT2FIX(scale));
   }
   else
   {
      /* argv[] = { (Ruby9i::Number | Fixnum | Bignum | Float | String) [, format] } */
      VALUE val, fmt;
      rb_scan_args(argc, argv, "02", &val, &fmt);

      /* set the format first so #assign can use it */
      if (argc >= 2)
      {
         if (NIL_P(fmt) || TYPE(fmt) != T_STRING)
            rb_raise(rb_eTypeError, "argument 2 is wrong type (found %s, expected String)", rb_class2name(CLASS_OF(fmt)));
         rb_iv_set(self, "@default_fmt", fmt);
      }

      number_assign(self, val);
   }

   return self;
}

VALUE number_add(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpout, *bpobj = NULL, *bpnum = NULL;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) { bpout->ind = bp->ind; return out; }

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
   {
      bpout->ind = 0;
      VALUE num = datatype_new(1, &obj, cNumber);
      Data_Get_Struct(num, oci9_define_buf, bpnum);
      if (OCINumberAdd(err_h, bp->val, bpnum->val, bpout->val))
         error_raise("OCINumberAdd failed", "number_add", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
   {
      Data_Get_Struct(obj, oci9_define_buf, bpobj);
      if ((bpout->ind = bpobj->ind) == 0)
         if (OCINumberAdd(err_h, bp->val, bpobj->val, bpout->val))
            error_raise("OCINumberAdd failed", "number_add", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(obj, cDate)))
   {
      out = rb_funcall(obj, ID_ADD, 1, self);
   }
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   return out;
}

VALUE number_subtract(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpout, *bpobj = NULL, *bpnum = NULL;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) { bpout->ind = bp->ind; return out; }

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
   {
      bpout->ind = 0;
      VALUE num = datatype_new(1, &obj, cNumber);
      Data_Get_Struct(num, oci9_define_buf, bpnum);
      if (OCINumberSub(err_h, bp->val, bpnum->val, bpout->val))
         error_raise("OCINumberSub failed", "number_subtract", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
   {
      Data_Get_Struct(obj, oci9_define_buf, bpobj);
      if ((bpout->ind = bpobj->ind) == 0)
         if (OCINumberSub(err_h, bp->val, bpobj->val, bpout->val))
            error_raise("OCINumberSub failed", "number_subtract", __FILE__, __LINE__);
   }
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   return out;
}

VALUE number_multiply(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpout, *bpobj = NULL, *bpnum = NULL;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) { bpout->ind = bp->ind; return out; }

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
   {
      bpout->ind = 0;
      VALUE num = datatype_new(1, &obj, cNumber);
      Data_Get_Struct(num, oci9_define_buf, bpnum);
      if (OCINumberMul(err_h, bp->val, bpnum->val, bpout->val))
         error_raise("OCINumberMul failed", "number_multiply", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
   {
      Data_Get_Struct(obj, oci9_define_buf, bpobj);
      if ((bpout->ind = bpobj->ind) == 0)
         if (OCINumberMul(err_h, bp->val, bpobj->val, bpout->val))
            error_raise("OCINumberMul failed", "number_multiply", __FILE__, __LINE__);
   }
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   return out;
}

VALUE number_divide(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpout, *bpobj = NULL, *bpnum = NULL;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) { bpout->ind = bp->ind; return out; }

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
   {
      bpout->ind = 0;
      VALUE num = datatype_new(1, &obj, cNumber);
      Data_Get_Struct(num, oci9_define_buf, bpnum);
      if (OCINumberDiv(err_h, bp->val, bpnum->val, bpout->val))
         error_raise("OCINumberMul failed", "number_divide", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
   {
      Data_Get_Struct(obj, oci9_define_buf, bpobj);
      if ((bpout->ind = bpobj->ind) == 0)
         if (OCINumberDiv(err_h, bp->val, bpobj->val, bpout->val))
            error_raise("OCINumberMul failed", "number_divide", __FILE__, __LINE__);
   }
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   return out;
}

VALUE number_divmod(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpout1, *bpout2, *bptmp, *bpin = NULL;
   VALUE in;
   VALUE out1 = datatype_new(0, NULL, CLASS_OF(self));
   VALUE out2 = datatype_new(0, NULL, CLASS_OF(self));
   VALUE tmp = datatype_new(0, NULL, CLASS_OF(self));
   VALUE ary = rb_ary_new3(2, out1, out2);
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(out1, oci9_define_buf, bpout1);
   Data_Get_Struct(out2, oci9_define_buf, bpout2);
   Data_Get_Struct(tmp, oci9_define_buf, bptmp);

   if (bp->ind != 0) { bpout1->ind = bpout2->ind = -1; return ary; }

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
      in = datatype_new(1, &obj, cNumber);
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
      in = obj;
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   Data_Get_Struct(in, oci9_define_buf, bpin);
   if (bpin->ind != 0) { bpout1->ind = bpout2->ind = -1; return ary; }
   if (OCINumberDiv(err_h, bp->val, bpin->val, bpout1->val))
      error_raise("OCINumberDiv failed", "number_divmod", __FILE__, __LINE__);
   if (OCINumberFloor(err_h, bpout1->val, bpout1->val))
      error_raise("OCINumberFloor failed", "number_divmod", __FILE__, __LINE__);
   if (OCINumberMul(err_h, bpout1->val, bpin->val, bptmp->val))
      error_raise("OCINumberMul failed", "number_divmod", __FILE__, __LINE__);
   if (OCINumberSub(err_h, bp->val, bptmp->val, bpout2->val))
      error_raise("OCINumberSub failed", "number_divmod", __FILE__, __LINE__);
   return ary;
}

VALUE number_compare(VALUE self, VALUE obj)
{
   sword result;
   oci9_define_buf *bp, *bpobj = NULL, *bpnum = NULL;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
   {
      Data_Get_Struct(obj, oci9_define_buf, bpobj);
      if (OCINumberCmp(err_h, bp->val, bpobj->val, &result))
         error_raise("OCINumberCmp failed", "number_compare", __FILE__, __LINE__);
      if (result < 0) result = -1;
      else if (result > 0) result = 1;
      return INT2FIX(result);
   }
   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
   {
      VALUE num = datatype_new(1, &obj, cNumber);
      Data_Get_Struct(num, oci9_define_buf, bpnum);
      if (OCINumberCmp(err_h, bp->val, bpnum->val, &result))
         error_raise("OCINumberCmp failed", "number_compare", __FILE__, __LINE__);
      return INT2FIX(result);
   }

   rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));
}

VALUE number_abs(VALUE self)
{
   oci9_define_buf *bp, *bpout;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(out, oci9_define_buf, bpout);

   if (bp->ind != 0) { bpout->ind = -1; return out; }

   bpout->ind = 0;
   if (OCINumberAbs(err_h, bp->val, bpout->val))
      error_raise("OCINumberAbs failed", "number_abs", __FILE__, __LINE__);
   return out;
}

VALUE number_ceil(VALUE self)
{
   oci9_define_buf *bp, *bpout;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(out, oci9_define_buf, bpout);

   if (bp->ind != 0) { bpout->ind = -1; return out; }

   if (OCINumberCeil(err_h, bp->val, bpout->val))
      error_raise("OCINumberCeil failed", "number_ceil", __FILE__, __LINE__);
   return out;
}

VALUE number_floor(VALUE self)
{
   oci9_define_buf *bp, *bpout;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(out, oci9_define_buf, bpout);

   if (bp->ind != 0) { bpout->ind = -1; return out; }

   if (OCINumberFloor(err_h, bp->val, bpout->val))
      error_raise("OCINumberFloor failed", "number_floor", __FILE__, __LINE__);
   return out;
}

VALUE number_round(int argc, VALUE *argv, VALUE self)
{
   VALUE v_digits;
   rb_scan_args(argc, argv, "01", &v_digits);

   oci9_define_buf *bp, *bpout;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(out, oci9_define_buf, bpout);
   sword digits = 0;

   if (bp->ind != 0) { bpout->ind = -1; return out; }

   if (argc > 0)
      digits = NUM2INT(v_digits);
   if (OCINumberRound(err_h, bp->val, digits, bpout->val))
      error_raise("OCINumberRound failed", "number_round", __FILE__, __LINE__);
   return out;
}

VALUE number_modulo(VALUE self, VALUE obj)
{
   return rb_ary_entry(number_divmod(self, obj), 1);
}

VALUE number_to_s(int argc, VALUE *argv, VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return rb_str_new2("NULL");

   VALUE fmt;
   rb_scan_args(argc, argv, "01", &fmt);
   if (argc == 0)
      fmt = rb_iv_get(self, "@default_fmt");

   char buf[ORACLE9_MAX_NUM_LEN];
   ub4 buflen = ORACLE9_MAX_NUM_LEN;
   if (OCINumberToText(err_h, bp->val, RSTRING(fmt)->ptr, RSTRING(fmt)->len, 0, 0, &buflen, buf))
      error_raise("OCINumberToText failed", "number_to_s", __FILE__, __LINE__);
   return rb_str_new(buf, buflen);
}

VALUE number_to_int(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return Qnil;

   /* attempt to keep as much precision as possible */
   return rb_funcall(rb_funcall(self, ID_TO_S, 1, rb_str_new2("TM")), ID_TO_I, 0);
}

VALUE number_to_float(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return Qnil;

   /* attempt to keep as much precision as possible */
   return rb_funcall(rb_funcall(self, ID_TO_S, 1, rb_str_new2("TM")), ID_TO_F, 0);
}

VALUE number_coerce(VALUE self, VALUE obj)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return rb_ary_new3(2, obj, Qnil);

   boolean is_int;
   if (OCINumberIsInt(err_h, bp->val, &is_int))
      error_raise("OCINumberIsInt failed", "number_coerce", __FILE__, __LINE__);

   if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
      return rb_ary_new3(2, obj, self);
   if (TYPE(obj) == T_FLOAT)
      return rb_ary_new3(2, obj, number_to_float(self));
   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM)
   {
      if (is_int)
         return rb_ary_new3(2, obj, number_to_int(self));
      else
         return rb_ary_new3(2, obj, number_to_float(self));
   }

   rb_raise(rb_eTypeError, "%s cannot be coerced into %s", rb_class2name(CLASS_OF(obj)), rb_class2name(CLASS_OF(self)));
}

VALUE number_to_builtin(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return Qnil;

   /* return a Fixnum as appropriate */
   boolean is_int;
   if (OCINumberIsInt(err_h, bp->val, &is_int))
      error_raise("OCINumberIsInt failed", "number_to_builtin", __FILE__, __LINE__);
   if (is_int)
      return rb_funcall(rb_funcall(self, ID_TO_S, 0), ID_TO_I, 0);

   /* return a Float */
   return rb_funcall(rb_funcall(self, ID_TO_S, 1, rb_str_new2("TM")), ID_TO_F, 0);
}

VALUE number_set_default_fmt(VALUE self, VALUE fmt)
{
   if (TYPE(fmt) != T_STRING)
      rb_raise(rb_eTypeError, "argument is wrong type (found %s, expected String)", rb_class2name(CLASS_OF(fmt)));
   rb_iv_set(self, "@default_fmt", fmt);
   return fmt;
}

void Init_Number()
{
   rb_include_module(cNumber, rb_mComparable);

   rb_define_method(cNumber, "initialize", number_initialize, -1);
   rb_enable_super(cNumber, "initialize");
   rb_define_method(cNumber, "assign", number_assign, 1);
   rb_define_method(cNumber, "+", number_add, 1);
   rb_define_method(cNumber, "-", number_subtract, 1);
   rb_define_method(cNumber, "*", number_multiply, 1);
   rb_define_method(cNumber, "/", number_divide, 1);
   rb_define_method(cNumber, "divmod", number_divmod, 1);
   rb_define_method(cNumber, "modulo", number_modulo, 1);
   rb_define_method(cNumber, "<=>", number_compare, 1);
   rb_define_method(cNumber, "abs", number_abs, 0);
   rb_define_method(cNumber, "ceil", number_ceil, 0);
   rb_define_method(cNumber, "floor", number_floor, 0);
   rb_define_method(cNumber, "round", number_round, -1);
   rb_define_method(cNumber, "coerce", number_coerce, 1);
   rb_define_method(cNumber, "to_i", number_to_int, 0);
   rb_define_method(cNumber, "to_f", number_to_float, 0);
   rb_define_method(cNumber, "to_s", number_to_s, -1);
   rb_define_method(cNumber, "to_builtin", number_to_builtin, 0);
   rb_define_method(cNumber, "default_fmt=", number_set_default_fmt, 1);

   /* register types handled by me */
   registry_add(TYPE_REGISTRY, SQLT_NUM, cNumber);
   registry_add(TYPE_REGISTRY, SQLT_INT, cNumber);
   registry_add(TYPE_REGISTRY, SQLT_FLT, cNumber);
}
