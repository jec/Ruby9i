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

VALUE varchar_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   int size;
   char *value;
   VALUE str;

   switch (argc)
   {
      case 1:
         if (NIL_P(argv[0]))
            break;
         if (rb_respond_to(argv[0], rb_intern("to_s")))
            /* use arg (converted to a String) as initial value */
            str = rb_funcall(argv[0], rb_intern("to_s"), 0);
         else
            rb_raise(rb_eArgError, "Cannot handle argument of type %s", rb_class2name(CLASS_OF(argv[0])));
         value = rb_str2cstr(str, &size);
         bp->val = ALLOC_N(char, size + 1);
         strncpy(bp->val, value, size + 1);
         bp->len = size + 1;
         bp->ind = 0;
         break;

      case 5:
         /* argv[] = { internal SQLT, size, precision, scale, ses_h }
          * used only when defining select-list in Statement#allocate_row
          *
          * prepare to receive varchar of length size
          */
         bp->val = ALLOC_N(char, FIX2INT(argv[1]) + 1);
         bp->len = FIX2INT(argv[1]) + 1;
         break;

      default:
         rb_raise(rb_eArgError, "wrong # of arguments(%d for 1 or 5)", argc);
   }

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
}
