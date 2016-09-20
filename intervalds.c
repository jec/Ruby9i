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

VALUE ivds_assign(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpin = NULL;
   VALUE in;
   Data_Get_Struct(self, oci9_define_buf, bp);
   dvoid *hdl = datatype_get_session_handle(self);

   if (TYPE(obj) == T_STRING)
   {
      if (OCIIntervalFromText(env_h, err_h, RSTRING(obj)->ptr, RSTRING(obj)->len, bp->desc))
         error_raise("OCIIntervalFromText failed", "ivds_assign", __FILE__, __LINE__);
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
      error_raise("OCIIntervalFromNumber failed", "ivds_assign", __FILE__, __LINE__);
   bp->ind = 0;
   return self;
}

VALUE ivds_initialize(int argc, VALUE *argv, VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   /* initialize values needed by super, then call super */
   bp->sqlt = SQLT_INTERVAL_DS;
   bp->dtype = OCI_DTYPE_INTERVAL_DS;
   rb_iv_set(self, "@zero", rb_str_new2("0 00:00:00"));
   rb_call_super(argc, argv);

   rb_iv_set(self, "@dayprecision", INT2FIX(2));
   rb_iv_set(self, "@secondprecision", INT2FIX(6));
   rb_define_attr(CLASS_OF(self), "secondprecision", 1, 0);
   rb_define_attr(CLASS_OF(self), "dayprecision", 1, 0);

   if (argc == 0)
      ; /* nothing more to do; super took care of it */
   else if (argc == 1 && TYPE(argv[0]) == T_ARRAY)
   {
      /* used by Statement to pick a type to receive column data
       * argv[0] = [ svc_h, ses_h, parm_h, position ]
       */
      oci9_handle *parm_h;
      ub1 fsprecision, lfprecision;
      Data_Get_Struct(rb_ary_entry(argv[0], 2), oci9_handle, parm_h);
      if (OCIAttrGet(parm_h->ptr, OCI_DTYPE_PARAM, &fsprecision, 0, OCI_ATTR_FSPRECISION, err_h))
         error_raise("Could not get second precision", "ivds_initialize", __FILE__, __LINE__);
      if (OCIAttrGet(parm_h->ptr, OCI_DTYPE_PARAM, &lfprecision, 0, OCI_ATTR_LFPRECISION, err_h))
         error_raise("Could not get day precision", "ivds_initialize", __FILE__, __LINE__);
      rb_hash_aset(rb_iv_get(self, "@column_info"), rb_str_new2("secondprecision"), INT2FIX(fsprecision));
      rb_hash_aset(rb_iv_get(self, "@column_info"), rb_str_new2("dayprecision"), INT2FIX(lfprecision));
      rb_iv_set(self, "@secondprecision", INT2FIX(fsprecision));
      rb_iv_set(self, "@dayprecision", INT2FIX(lfprecision));
   }
   else
   {
      /* argv[] = { (String | days) [, dayprecision [, secondprecision]] } */
      VALUE val, dayprec, secprec;
      rb_scan_args(argc, argv, "03", &val, &dayprec, &secprec);
      ivds_assign(self, val);

      if (argc >= 2)
      {
         if (NIL_P(dayprec) || TYPE(dayprec) != T_FIXNUM)
            rb_raise(rb_eTypeError, "argument 2 is wrong type (found %s, expected Fixnum)", rb_class2name(CLASS_OF(dayprec)));
         rb_iv_set(self, "@dayprecision", dayprec);
      }

      if (argc >= 3)
      {
         if (NIL_P(secprec) || TYPE(secprec) != T_FIXNUM)
            rb_raise(rb_eTypeError, "argument 2 is wrong type (found %s, expected Fixnum)", rb_class2name(CLASS_OF(secprec)));
         rb_iv_set(self, "@secondprecision", secprec);
      }
   }

   return self;
}

VALUE ivds_lfprecision(VALUE self)
{
   return rb_iv_get(self, "@dayprecision");
}

VALUE ivds_fsprecision(VALUE self)
{
   return rb_iv_get(self, "@secondprecision");
}

void Init_IntervalDS()
{
   rb_define_method(cIntervalDS, "initialize", ivds_initialize, -1);
   rb_enable_super(cIntervalDS, "initialize");
   rb_define_method(cIntervalDS, "assign", ivds_assign, -1);
   rb_define_method(cIntervalDS, "lfprecision", ivds_lfprecision, 0);
   rb_define_method(cIntervalDS, "fsprecision", ivds_fsprecision, 0);

   /* register types handled by me */
   registry_add(TYPE_REGISTRY, SQLT_INTERVAL_DS, cIntervalDS);
}
