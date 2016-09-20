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

VALUE number_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   ub4 big;
   double dbl;

   switch (argc)
   {
      case 1:
         switch (TYPE(argv[0]))
         {
            case T_FIXNUM:
            case T_BIGNUM:
               bp->val = ALLOC(OCINumber);
               bp->len = sizeof(OCINumber);
               bp->ind = 0;
               big = (ub4) NUM2LONG(argv[0]);
               if (OCINumberFromInt(err_h, &big, sizeof(big), OCI_NUMBER_SIGNED, bp->val))
                  error_raise("Could not convert to number", "number_initialize", __FILE__, __LINE__);
               break;

            case T_FLOAT:
               bp->val = ALLOC(OCINumber);
               bp->len = sizeof(OCINumber);
               bp->ind = 0;
               dbl = NUM2DBL(argv[0]);
               if (OCINumberFromReal(err_h, &dbl, (uword)sizeof(dbl), bp->val))
                  error_raise("Could not convert to number", "number_initialize", __FILE__, __LINE__);
               break;

            default:
               rb_raise(rb_eArgError, "wrong argument type %s (expected Number)", rb_class2name(argv[0]));
         }
         break;

      case 5:
         /* argv[] = { internal SQLT, size, precision, scale, ses_h }
          * used only when defining select-list in Statement#allocate_row
          *
          * prepare to receive a number/float/integer
          */
         bp->val = ALLOC(OCINumber);
         bp->len = sizeof(OCINumber);
         break;

      default:
         rb_raise(rb_eArgError, "wrong # of arguments(%d for 1 or 5)", argc);
   }

   /* read/write this as an OCINumber */
   bp->sqlt = SQLT_VNU;

   return self;
}

VALUE number_to_s(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return rb_str_new2("NULL");

   char buf[ORACLE9_MAX_NUM_LEN];
   ub4 buflen = ORACLE9_MAX_NUM_LEN;
   if (OCINumberToText(err_h, bp->val, "TM", 2, 0, 0, &buflen, buf))
      error_raise("Could not convert to number to text", "number_to_s", __FILE__, __LINE__);
   return rb_str_new(buf, buflen);
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
      error_raise("Could not test whether number is an integer", "number_to_builtin", __FILE__, __LINE__);
   if (is_int)
      return rb_funcall(rb_funcall(self, rb_intern("to_s"), 0), rb_intern("to_i"), 0);

   /* return a Float */
   double num;
   if (OCINumberToReal(err_h, bp->val, sizeof(double), &num))
      error_raise("Could not convert number to text", "number_to_builtin", __FILE__, __LINE__);
   return rb_float_new(num);
}

void Init_Number()
{
   rb_define_method(cNumber, "initialize", number_initialize, -1);
   rb_enable_super(cNumber, "initialize");
   rb_define_method(cNumber, "to_s", number_to_s, 0);
   rb_define_method(cNumber, "to_builtin", number_to_builtin, 0);
}
