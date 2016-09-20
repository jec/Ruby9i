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

#define MAX_DATE_LEN 100

VALUE date_assign(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpobj = NULL;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (TYPE(obj) == T_STRING)
   {
      bp->ind = 0;
      VALUE fmt = rb_iv_get(self, "@default_fmt");
      if (OCIDateFromText(err_h, RSTRING(obj)->ptr, RSTRING(obj)->len, RSTRING(fmt)->ptr, RSTRING(fmt)->len, 0, 0, bp->val))
         error_raise("OCIDateFromText failed", "date_assign", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(obj, cDate)))
   {
      Data_Get_Struct(obj, oci9_define_buf, bpobj);
      bp->ind = bpobj->ind;
      if (OCIDateAssign(err_h, bpobj->val, bp->val))
         error_raise("OCIDateTimeAssign failed", "bts_assign", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(obj, rb_cTime)))
   {
      bp->ind = 0;
      VALUE v_ary = rb_funcall(obj, ID_TO_A, 0);
      OCIDateSetDate((OCIDate*)bp->val,
         FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(5))),
         FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(4))),
         FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(3))));
      OCIDateSetTime((OCIDate*)bp->val,
         FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(2))),
         FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(1))),
         FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(0))));
      uword flags;
      if (OCIDateCheck(err_h, bp->val, &flags))
         error_raise("Could not convert Time to date", "date_assign", __FILE__, __LINE__);
   }
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   return self;
}

VALUE date_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   /* initialize OCIDate */
   bp->val = ALLOC(OCIDate);
   bp->len = sizeof(OCIDate);
   bp->sqlt = SQLT_ODT;
   rb_iv_set(self, "@default_fmt", rb_str_new2("yyyy-mm-dd hh24:mi:ss"));
   rb_define_attr(CLASS_OF(self), "default_fmt", 1, 0);

   if (argc == 0)
   {
      /* set to current time */
      bp->ind = 0;
      if (OCIDateSysDate(err_h, bp->val))
         error_raise("Could not set date", "date_initialize", __FILE__, __LINE__);
   }
   else if (argc == 1 && TYPE(argv[0]) == T_ARRAY)
   {
      /* used by Statement to pick a type to receive column data
       * argv[0] = [ svc_h, ses_h, parm_h, position ]
       *
       * nothing more to do here
       */
   }
   else
   {
      /* argv[] = { (Ruby9i::Date | String | Time) [, format] } */
      VALUE val, fmt;
      rb_scan_args(argc, argv, "02", &val, &fmt);

      /* set the format first so #assign can use it */
      if (argc >= 2)
      {
         if (NIL_P(fmt) || TYPE(fmt) != T_STRING)
            rb_raise(rb_eTypeError, "argument 2 is wrong type (found %s, expected String)", rb_class2name(CLASS_OF(fmt)));
         rb_iv_set(self, "@default_fmt", fmt);
      }

      date_assign(self, val);
   }

   return self;
}

VALUE date_do_add(VALUE self, VALUE obj, int sign)
{
   oci9_define_buf *bp, *bpobj = NULL, *bpout;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(self, oci9_define_buf, bp);

   /* if i'm NULL, return NULL date */
   if (bp->ind != 0)
   {
      bpout->ind = bp->ind;
      return out;
   }

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM)
   {
      bpout->ind = 0;
      if (OCIDateAddDays(err_h, bp->val, NUM2INT(obj) * sign, bpout->val))
         error_raise("OCIDateAddDays failed", "date_do_add", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
   {
      Data_Get_Struct(obj, oci9_define_buf, bpobj);
      /* do the addition only if obj is not NULL */
      if ((bpout->ind = bpobj->ind) == 0)
      {
         sb4 num = NUM2INT(rb_funcall(obj, ID_TO_I, 0));
         if (OCIDateAddDays(err_h, bp->val, num * sign, bpout->val))
            error_raise("OCIDateAddDays failed", "date_do_add", __FILE__, __LINE__);
      }
   }
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   return out;
}

VALUE date_add(VALUE self, VALUE obj)
{
   return date_do_add(self, obj, 1);
}

VALUE date_subtract(VALUE self, VALUE obj)
{
   /* if obj is not a Date, use negative #add */
   if (!RTEST(rb_obj_is_kind_of(obj, cDate)))
      return date_do_add(self, obj, -1);

   oci9_define_buf *bp, *bpobj = NULL, *bpout;
   VALUE out = datatype_new(0, NULL, cNumber);
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(out, oci9_define_buf, bpout);

   /* if i'm NULL, return NULL Number */
   if (bp->ind != 0)
   {
      bpout->ind = bp->ind;
      return out;
   }

   Data_Get_Struct(obj, oci9_define_buf, bpobj);
   sb4 days;
   if (OCIDateDaysBetween(err_h, bp->val, bpobj->val, &days))
      error_raise("OCIDateDaysBetween failed", "date_subtract", __FILE__, __LINE__);
   rb_funcall(out, ID_ASSIGN, 1, INT2FIX(days));
   return out;
}

VALUE date_add_months(VALUE self, VALUE obj)
{
   oci9_define_buf *bp, *bpobj = NULL, *bpout;
   VALUE out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(self, oci9_define_buf, bp);

   /* if i'm NULL, return NULL date */
   if (bp->ind != 0)
   {
      bpout->ind = bp->ind;
      return out;
   }

   if (TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
   {
      bpout->ind = 0;
      if (OCIDateAddMonths(err_h, bp->val, NUM2INT(obj), bpout->val))
         error_raise("OCIDateAddMonths failed", "date_add_months", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)))
   {
      Data_Get_Struct(obj, oci9_define_buf, bpobj);
      /* do the addition only if obj is not NULL */
      if ((bpout->ind = bpobj->ind) == 0)
      {
         sb4 num = NUM2INT(rb_funcall(obj, ID_TO_I, 0));
         if (OCIDateAddMonths(err_h, bp->val, num, bpout->val))
            error_raise("OCIDateAddMonths failed", "date_add_months", __FILE__, __LINE__);
      }
   }
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   return out;
}

VALUE date_compare(VALUE self, VALUE obj)
{
   if (!RTEST(rb_obj_is_kind_of(obj, cDate)))
      rb_raise(rb_eTypeError, "argument is wrong type (found %s, expected %s)", rb_class2name(CLASS_OF(obj)), rb_class2name(cDate));

   sword result;
   oci9_define_buf *bp, *bpobj;
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(obj, oci9_define_buf, bpobj);

   if (bp->ind != 0)
      return Qnil;

   if (OCIDateCompare(err_h, bp->val, bpobj->val, &result))
      error_raise("OCIDateCompare failed", "date_compare", __FILE__, __LINE__);

   return INT2FIX(result);
}

VALUE date_last_day(VALUE self)
{
   VALUE lastday = datatype_new(0, 0, cDate);
   oci9_define_buf *bpself, *bpld;
   Data_Get_Struct(self, oci9_define_buf, bpself);
   Data_Get_Struct(lastday, oci9_define_buf, bpld);
   if (OCIDateLastDay(err_h, (CONST OCIDate*) bpself->val, bpld->val))
      error_raise("OCIDateLastDay failed", "date_last_day", __FILE__, __LINE__);
   return lastday;
}

VALUE date_next_day(VALUE self, VALUE v_day)
{
   VALUE nextday = datatype_new(0, 0, cDate);
   oci9_define_buf *bpself, *bpnd;
   Data_Get_Struct(self, oci9_define_buf, bpself);
   Data_Get_Struct(nextday, oci9_define_buf, bpnd);
   if (OCIDateNextDay(err_h, (CONST OCIDate*) bpself->val, RSTRING(v_day)->ptr, RSTRING(v_day)->len, bpnd->val))
      error_raise("OCIDateNextDay failed", "date_next_day", __FILE__, __LINE__);
   return nextday;
}

VALUE date_year(VALUE self)
{
   sb2 year;
   ub1 month, day;
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0) return Qnil;
   OCIDateGetDate((OCIDate*) bp->val, &year, &month, &day);
   return INT2FIX(year);
}

VALUE date_month(VALUE self)
{
   sb2 year;
   ub1 month, day;
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0) return Qnil;
   OCIDateGetDate((OCIDate*) bp->val, &year, &month, &day);
   return INT2FIX(month);
}

VALUE date_day(VALUE self)
{
   sb2 year;
   ub1 month, day;
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0) return Qnil;
   OCIDateGetDate((OCIDate*) bp->val, &year, &month, &day);
   return INT2FIX(day);
}

VALUE date_hour(VALUE self)
{
   ub1 hour, min, sec;
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0) return Qnil;
   OCIDateGetTime((OCIDate*) bp->val, &hour, &min, &sec);
   return INT2FIX(hour);
}

VALUE date_min(VALUE self)
{
   ub1 hour, min, sec;
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0) return Qnil;
   OCIDateGetTime((OCIDate*) bp->val, &hour, &min, &sec);
   return INT2FIX(min);
}

VALUE date_sec(VALUE self)
{
   ub1 hour, min, sec;
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0) return Qnil;
   OCIDateGetTime((OCIDate*) bp->val, &hour, &min, &sec);
   return INT2FIX(sec);
}

VALUE date_do_to_s(OCIDate *date, char *fmt)
{
   char buf[MAX_DATE_LEN];
   ub4 buflen = MAX_DATE_LEN;
   if (OCIDateToText(err_h, date, fmt, strlen(fmt), 0, 0, &buflen, buf))
      error_raise("OCIDateToText failed", "date_do_to_s", __FILE__, __LINE__);
   return rb_str_new(buf, buflen);
}

VALUE date_wday(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0) return Qnil;
   return rb_funcall(date_do_to_s((OCIDate*) bp->val, "D"), ID_TO_I, 0);
}

VALUE date_yday(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0) return Qnil;
   return rb_funcall(date_do_to_s((OCIDate*) bp->val, "DDD"), ID_TO_I, 0);
}

VALUE date_to_a(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   sb2 year;
   ub1 month, day, hour, min, sec;
   OCIDateGetDate((OCIDate*) bp->val, &year, &month, &day);
   OCIDateGetTime((OCIDate*) bp->val, &hour, &min, &sec);
   return rb_ary_new3(8, INT2FIX(sec), INT2FIX(min), INT2FIX(hour), INT2FIX(day), INT2FIX(month), INT2FIX(year),
      INT2FIX(FIX2INT(rb_funcall(date_do_to_s((OCIDate*) bp->val, "D"), ID_TO_I, 0)) - 1),
      rb_funcall(date_do_to_s((OCIDate*) bp->val, "DDD"), ID_TO_I, 0));
}

VALUE date_to_s(int argc, VALUE *argv, VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return rb_str_new2("NULL");

   VALUE fmt;
   rb_scan_args(argc, argv, "01", &fmt);
   if (argc == 0)
      fmt = rb_iv_get(self, "@default_fmt");

   return date_do_to_s((OCIDate*) bp->val, RSTRING(fmt)->ptr);
}

VALUE date_set_default_fmt(VALUE self, VALUE fmt)
{
   if (TYPE(fmt) != T_STRING)
      rb_raise(rb_eTypeError, "argument is wrong type (found %s, expected String)", rb_class2name(CLASS_OF(fmt)));
   rb_iv_set(self, "@default_fmt", fmt);
   return fmt;
}

VALUE date_to_builtin(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   sb2 year;
   ub1 month, day, hour, min, sec;
   OCIDateGetDate((OCIDate*)bp->val, &year, &month, &day);
   OCIDateGetTime((OCIDate*)bp->val, &hour, &min, &sec);
   return rb_funcall(rb_cTime, ID_LOCAL, 6, INT2FIX(year), INT2FIX(month), INT2FIX(day),
      INT2FIX(hour), INT2FIX(min), INT2FIX(sec));
}

void Init_Date()
{
   rb_include_module(cDate, rb_mComparable);

   rb_define_method(cDate, "initialize", date_initialize, -1);
   rb_enable_super(cDate, "initialize");
   rb_define_method(cDate, "assign", date_assign, 1);
   rb_define_method(cDate, "+", date_add, 1);
   rb_define_method(cDate, "-", date_subtract, 1);
   rb_define_method(cDate, "<=>", date_compare, 1);
   rb_define_method(cDate, "add_months", date_add_months, 1);
   rb_define_method(cDate, "year", date_year, 0);
   rb_define_method(cDate, "mon", date_month, 0);
   rb_define_method(cDate, "month", date_month, 0);
   rb_define_method(cDate, "day", date_day, 0);
   rb_define_method(cDate, "hour", date_hour, 0);
   rb_define_method(cDate, "min", date_min, 0);
   rb_define_method(cDate, "sec", date_sec, 0);
   rb_define_method(cDate, "wday", date_wday, 0);
   rb_define_method(cDate, "yday", date_yday, 0);
   rb_define_method(cDate, "last_day", date_last_day, 0);
   rb_define_method(cDate, "next_day", date_next_day, 1);
   rb_define_method(cDate, "default_fmt=", date_set_default_fmt, 1);
   rb_define_method(cDate, "to_a", date_to_a, 0);
   rb_define_method(cDate, "to_s", date_to_s, -1);
   rb_define_method(cDate, "to_builtin", date_to_builtin, 0);

   /* register types handled by me */
   registry_add(TYPE_REGISTRY, SQLT_DAT, cDate);
}
