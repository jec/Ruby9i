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

#define ORACLE9_DEFAULT_DATE_FMT "yyyy-mm-dd hh24:mi:ss"
#define ORACLE9_MAX_DATE_LEN 100

VALUE date_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   int datelen;

   bp->val = ALLOC(OCIDate);
   bp->len = sizeof(OCIDate);
   switch (argc)
   {
      case 0:
         /* set to current time */
         if (OCIDateSysDate(err_h, bp->val))
            error_raise("Could not set date", "date_initialize", __FILE__, __LINE__);
         bp->ind = 0;
         break;

      case 1:
         bp->ind = 0;
         if (TYPE(argv[0]) == T_STRING)
         {
            char *date = rb_str2cstr(argv[0], &datelen);
            if (OCIDateFromText(err_h, date, datelen, ORACLE9_DEFAULT_DATE_FMT, strlen(ORACLE9_DEFAULT_DATE_FMT),
                  0, 0, bp->val))
               error_raise("Could not convert text to date", "date_initialize", __FILE__, __LINE__);
         }
         else if (RTEST(rb_obj_is_kind_of(argv[0], rb_cTime)))
         {
            VALUE v_ary = rb_funcall(argv[0], rb_intern("to_a"), 0);
            int i;
            long ary[6];
            for (i=0; i<6; i++)
               ary[i] = FIX2INT(rb_funcall(v_ary, rb_intern("[]"), 1, INT2FIX(i)));
            OCIDateSetDate((OCIDate*)bp->val, ary[5], ary[4], ary[3]);
            OCIDateSetTime((OCIDate*)bp->val, ary[2], ary[1], ary[0]);
            uword flags;
            if (OCIDateCheck(err_h, bp->val, &flags))
               error_raise("Could not convert Time to date", "date_initialize", __FILE__, __LINE__);
         }
         else
            rb_raise(rb_eArgError, "wrong argument type %s (expected String or Time)", rb_class2name(argv[0]));
         break;

      case 5:
         /* argv[] = { internal SQLT, size, precision, scale, ses_h }
          * used only when defining select-list in Statement#allocate_row
          */
         break;

      default:
         rb_raise(rb_eArgError, "wrong # of arguments(%d for 1 or 5)", argc);
   }

   /* read/write this as an OCIDate */
   bp->sqlt = SQLT_ODT;

   return self;
}

VALUE date_last_day(VALUE self)
{
   VALUE lastday = datatype_new(0, 0, cDate);
   oci9_define_buf *bpself, *bpld;
   Data_Get_Struct(self, oci9_define_buf, bpself);
   Data_Get_Struct(lastday, oci9_define_buf, bpld);
   if (OCIDateLastDay(err_h, (CONST OCIDate*) bpself->val, bpld->val))
      error_raise("Could not get last day", "date_last_day", __FILE__, __LINE__);
   return lastday;
}

VALUE date_next_day(VALUE self, VALUE v_day)
{
   VALUE nextday = datatype_new(0, 0, cDate);
   oci9_define_buf *bpself, *bpnd;
   Data_Get_Struct(self, oci9_define_buf, bpself);
   Data_Get_Struct(nextday, oci9_define_buf, bpnd);
   int daylen;
   char *day = rb_str2cstr(v_day, &daylen);
   if (OCIDateNextDay(err_h, (CONST OCIDate*) bpself->val, day, daylen, bpnd->val))
      error_raise("Could not get next day", "date_next_day", __FILE__, __LINE__);
   return nextday;
}

VALUE date_to_s(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return rb_str_new2("NULL");

   char buf[100];
   ub4 buflen = 100;
   if (OCIDateToText(err_h, (CONST OCIDate*) bp->val, "yyyy-mm-dd hh24:mi:ss", 21, 0, 0, &buflen, buf))
      error_raise("Could not convert date to text", "date_to_s", __FILE__, __LINE__);
   return rb_str_new(buf, buflen);
}

VALUE date_to_builtin(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0)
      return Qnil;

   sb2 year;
   ub1 month, day, hour, min, sec;
   OCIDateGetDate((OCIDate*)bp->val, &year, &month, &day);
   OCIDateGetTime((OCIDate*)bp->val, &hour, &min, &sec);
   return rb_funcall(rb_cTime, rb_intern("local"), 6, INT2FIX(year), INT2FIX(month), INT2FIX(day),
      INT2FIX(hour), INT2FIX(min), INT2FIX(sec));
}

void Init_Date()
{
   rb_define_method(cDate, "initialize", date_initialize, -1);
   rb_enable_super(cDate, "initialize");
   rb_define_method(cDate, "last_day", date_last_day, 0);
   rb_define_method(cDate, "next_day", date_next_day, 1);
   rb_define_method(cDate, "to_s", date_to_s, 0);
   rb_define_method(cDate, "to_builtin", date_to_builtin, 0);
}
