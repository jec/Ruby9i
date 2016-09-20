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

VALUE biv_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (OCIDescriptorAlloc(env_h, &bp->desc, bp->dtype, 0, 0))
      error_raise("Could not allocate OCIInterval descriptor", "biv_initialize", __FILE__, __LINE__);
   bp->len = sizeof(bp->desc);

   switch (argc)
   {
      case 0:
         /* create a zero interval */
         bp->ind = 0;
         int zerolen;
         char *zero = rb_str2cstr(rb_iv_get(self, "@zero"), &zerolen);
         if (OCIIntervalFromText(env_h, err_h, zero, zerolen, bp->desc))
            error_raise("OCIIntervalFromText failed", "biv_initialize", __FILE__, __LINE__);
         break;

      case 1:
      case 2:
      case 3:
         /* argv[] = { String [precision [scale]] }
          * precision = # of day digits for interval_ds
          * scale = # of year digits for interval_ym or decimal seconds for interval_ds
          */
         bp->ind = 0;
         VALUE val, precision, scale;
         rb_scan_args(argc, argv, "12", &val, &precision, &scale);

         /* override default precision/scale if specified */
         if (RTEST(precision))
         {
            if (TYPE(precision) != T_FIXNUM)
               rb_raise(rb_eArgError, "argument 2 is wrong type %s (expected Fixnum)", rb_class2name(argv[1]));
            rb_iv_set(self, "@precision", precision);
         }
         if (RTEST(scale))
         {
            if (TYPE(scale) != T_FIXNUM)
               rb_raise(rb_eArgError, "argument 3 is wrong type %s (expected Fixnum)", rb_class2name(argv[1]));
            rb_iv_set(self, "@scale", scale);
         }

         /* set interval */
         int ivlen;
         char *iv = rb_str2cstr(argv[0], &ivlen);
         if (OCIIntervalFromText(env_h, err_h, iv, ivlen, bp->desc))
            error_raise("Could not convert text to interval", "biv_initialize", __FILE__, __LINE__);
         break;

      case 5:
         /* argv[] = { internal SQLT, size, precision, scale, ses_h }
          * used only when defining select-list in Statement#allocate_row
          */
         break;

      default:
         rb_raise(rb_eArgError, "wrong # of arguments(%d for 1, 2 or 5)", argc);
   }

   bp->val = &bp->desc;

   return self;
}

VALUE biv_to_s(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return rb_str_new2("NULL");

   char buf[MAX_IV_LEN];
   size_t bufoutlen;
   if (OCIIntervalToText(env_h, err_h, bp->desc, FIX2INT(rb_iv_get(self, "@precision")), (ub1) FIX2INT(rb_iv_get(self, "@scale")),
         (OraText*) buf, MAX_IV_LEN, &bufoutlen))
      error_raise("Could not convert interval to text", "biv_to_s", __FILE__, __LINE__);
   return rb_str_new(buf, bufoutlen);
}

VALUE biv_to_builtin(VALUE self)
{
   /* should eventually return #years or #days */
   return self;
}

void Init_BaseInterval()
{
   rb_define_method(cBaseInterval, "initialize", biv_initialize, -1);
   rb_enable_super(cBaseInterval, "initialize");
   rb_define_method(cBaseInterval, "to_s", biv_to_s, 0);
   rb_define_method(cBaseInterval, "to_builtin", biv_to_builtin, 0);
}
