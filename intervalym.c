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

#define MAX_IV_LEN 100

VALUE ivym_assign(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpin = NULL;
   VALUE in;
   Data_Get_Struct(self, oci9_define_buf, bp);
   dvoid *hdl = datatype_get_session_handle(self);

   if (TYPE(obj) == T_STRING)
   {
      if (OCIIntervalFromText(env_h, err_h, RSTRING(obj)->ptr, RSTRING(obj)->len, bp->desc))
         error_raise("OCIIntervalFromText failed", "ivym_assign", __FILE__, __LINE__);
      bp->ind = 0;
      return self;
   }

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
      in = datatype_new(1, &obj, cNumber);
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
      in = obj;
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   Data_Get_Struct(in, oci9_define_buf, bpin);
   if (bpin->ind != 0) { bp->ind = -1; return self; }
   if (OCIIntervalFromNumber(hdl, err_h, bp->desc, bpin->val))
      error_raise("OCIIntervalFromNumber failed", "ivym_assign", __FILE__, __LINE__);
   bp->ind = 0;
   return self;
}

VALUE ivym_initialize(int argc, VALUE *argv, VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   /* initialize values needed by super, then call super */
   bp->sqlt = SQLT_INTERVAL_YM;
   bp->dtype = OCI_DTYPE_INTERVAL_YM;
   rb_iv_set(self, "@zero", rb_str_new2("0-0"));
   rb_call_super(argc, argv);

   rb_iv_set(self, "@yearprecision", INT2FIX(2));
   rb_define_attr(CLASS_OF(self), "yearprecision", 1, 0);

   if (argc == 0)
      ; /* nothing to do; handled by super */
   else if (argc == 1 && TYPE(argv[0]) == T_ARRAY)
   {
      /* used by Statement to pick a type to receive column data
       * argv[0] = [ svc_h, ses_h, parm_h, position ]
       */
      oci9_handle *parm_h;
      ub1 lfprecision;
      Data_Get_Struct(rb_ary_entry(argv[0], 2), oci9_handle, parm_h);
      if (OCIAttrGet(parm_h->ptr, OCI_DTYPE_PARAM, &lfprecision, 0, OCI_ATTR_LFPRECISION, err_h))
         error_raise("Could not get year precision", "ivds_initialize", __FILE__, __LINE__);
      rb_hash_aset(rb_iv_get(self, "@column_info"), rb_str_new2("yearprecision"), INT2FIX(lfprecision));
      rb_iv_set(self, "@yearprecision", INT2FIX(lfprecision));
   }
   else
   {
      /* argv[] = { (String | years) [, yearprecision] } */
      VALUE val, yearprec;
      rb_scan_args(argc, argv, "02", &val, &yearprec);
      ivym_assign(self, val);

      if (argc == 2)
      {
         if (NIL_P(yearprec) || TYPE(yearprec) != T_FIXNUM)
            rb_raise(rb_eTypeError, "argument 2 is wrong type (found %s, expected Fixnum)", rb_class2name(CLASS_OF(yearprec)));
         rb_iv_set(self, "@yearprecision", yearprec);
      }
   }

   return self;
}

VALUE ivym_lfprecision(VALUE self)
{
   return rb_iv_get(self, "@yearprecision");
}

VALUE ivym_fsprecision(VALUE self)
{
   return INT2FIX(0);
}

void Init_IntervalYM()
{
   rb_define_method(cIntervalYM, "initialize", ivym_initialize, -1);
   rb_enable_super(cIntervalYM, "initialize");
   rb_define_method(cIntervalYM, "assign", ivym_assign, 1);
   rb_define_method(cIntervalYM, "lfprecision", ivym_lfprecision, 0);
   rb_define_method(cIntervalYM, "fsprecision", ivym_fsprecision, 0);

   /* register types handled by me */
   registry_add(TYPE_REGISTRY, SQLT_INTERVAL_YM, cIntervalYM);
}
