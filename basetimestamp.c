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

#define MAX_TS_LEN 100
#define MAX_TS_ARRAY_SZ 13

OCIInterval *UTC_OFFSET;

VALUE bts_systimestamp(VALUE klass)
{
   /* systimestamp always outputs a timestamp w/tz */
   VALUE sts = datatype_new(0, NULL, cTimestampTZ);
   oci9_define_buf *bpsts, *bpts;
   Data_Get_Struct(sts, oci9_define_buf, bpsts);
   bpsts->ind = 0;
   if (OCIDateTimeSysTimeStamp(datatype_get_session_handle(sts), err_h, bpsts->desc))
      error_raise("OCIDateTimeSysTimeStamp failed", "bts_systimestamp", __FILE__, __LINE__);

   if (RTEST(rb_obj_is_kind_of(sts, klass)))
      /* no coersion needed */
      return sts;

   /* convert to requested class */
   VALUE ts = datatype_new(0, NULL, klass);
   Data_Get_Struct(ts, oci9_define_buf, bpts);
   if (OCIDateTimeConvert(datatype_get_session_handle(ts), err_h, bpsts->desc, bpts->desc))
      error_raise("OCIDateTimeConvert failed", "bts_systimestamp", __FILE__, __LINE__);
   return ts;
}

VALUE bts_new(int argc, VALUE *argv, VALUE klass)
{
   if (argc == 0)
      return bts_systimestamp(klass);
   else
      return datatype_new(argc, argv, klass);
}

VALUE bts_assign(VALUE self, VALUE v_obj)
{
   dvoid *hdl = datatype_get_session_handle(self);
   oci9_define_buf *bp, *bpobj = NULL;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (TYPE(v_obj) == T_STRING)
   {
      bp->ind = 0;
      VALUE fmt = rb_iv_get(self, "@default_fmt");
      if (OCIDateTimeFromText(env_h, err_h, RSTRING(v_obj)->ptr, RSTRING(v_obj)->len,
            RSTRING(fmt)->ptr, RSTRING(fmt)->len, 0, 0, bp->desc))
         error_raise("OCIDateTimeFromText failed", "bts_assign", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(v_obj, cBaseTimestamp)))
   {
      Data_Get_Struct(v_obj, oci9_define_buf, bpobj);
      bp->ind = bpobj->ind;
      if (OCIDateTimeConvert(hdl, err_h, bpobj->desc, bp->desc))
         error_raise("OCIDateTimeAssign failed", "bts_assign", __FILE__, __LINE__);
   }
   else if (RTEST(rb_obj_is_kind_of(v_obj, rb_cTime)))
   {
      bp->ind = 0;
      VALUE v_ary = rb_funcall(v_obj, ID_TO_A, 0);
      long tz_offset_mins = FIX2INT(rb_funcall(v_obj, ID_UTC_OFFSET, 0)) / 60;
      char tz_offset[7];
      snprintf(tz_offset, 7, "%+02ld:%02ld", tz_offset_mins / 60, tz_offset_mins % 60);
      if (OCIDateTimeConstruct(hdl, err_h, bp->desc,
            FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(5))),
            FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(4))),
            FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(3))),
            FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(2))),
            FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(1))),
            FIX2INT(rb_funcall(v_ary, ID_SUBSCRIPT, 1, INT2FIX(0))),
            FIX2LONG(rb_funcall(v_obj, ID_USEC, 0)) * 1000,
            tz_offset, strlen(tz_offset)))
         error_raise("OCIDateTimeConstruct failed", "bts_assign", __FILE__, __LINE__);
   }
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(v_obj)));

   return self;
}

VALUE bts_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (OCIDescriptorAlloc(env_h, &bp->desc, bp->dtype, 0, 0))
      error_raise("OCIDescriptorAlloc failed", "bts_initialize", __FILE__, __LINE__);
   bp->len = sizeof(bp->desc);
   bp->val = &bp->desc;
   dvoid *hdl = datatype_get_session_handle(self);

   rb_iv_set(self, "@secondprecision", INT2FIX(6));
   rb_define_attr(CLASS_OF(self), "secondprecision", 1, 0);

   if (argc == 0)
   {
      /* initialize with current time */
      bp->ind = 0;
      if (OCIDateTimeSysTimeStamp(hdl, err_h, bp->desc))
         error_raise("OCIDateTimeSysTimeStamp failed", "bts_initialize", __FILE__, __LINE__);
   }
   else if (argc == 1 && TYPE(argv[0]) == T_ARRAY)
   {
      /* used by Statement to pick a type to receive column data
       * argv[0] = [ svc_h, ses_h, parm_h, position ]
       */
      oci9_handle *parm_h;
      Data_Get_Struct(rb_ary_entry(argv[0], 2), oci9_handle, parm_h);

      /* get precision & edit default format to match */
      ub1 fsprecision;
      if (OCIAttrGet(parm_h->ptr, OCI_DTYPE_PARAM, &fsprecision, 0, OCI_ATTR_FSPRECISION, err_h))
         error_raise("Could not get second precision", "ivds_initialize", __FILE__, __LINE__);
      rb_funcall(rb_iv_get(self, "@default_fmt"), rb_intern("sub!"), 2, rb_reg_new("[fF][fF]\\d", strlen("[fF][fF]\\d"), 0),
         rb_str_concat(rb_str_new2("ff"), rb_funcall(INT2FIX(fsprecision), ID_TO_S, 0)));
      rb_hash_aset(rb_iv_get(self, "@column_info"), rb_str_new2("secondprecision"), INT2FIX(fsprecision));
   }
   else
   {
      /* argv[] = { [(String | Time | BaseTimestamp) [format]] } */
      VALUE val, fmt;
      rb_scan_args(argc, argv, "02", &val, &fmt);

      /* set the format first so #assign can use it */
      if (argc >= 2)
      {
         if (NIL_P(fmt) || TYPE(fmt) != T_STRING)
            rb_raise(rb_eTypeError, "argument 2 is wrong type (found %s, expected String)", rb_class2name(CLASS_OF(fmt)));
         rb_iv_set(self, "@default_fmt", fmt);
         /* get the 'ffx' in fmt and set secprec = x */
         VALUE ary = rb_funcall(fmt, rb_intern("scan"), 1, rb_reg_new("[fF][fF](\\d)", strlen("[fF][fF](\\d)"), 0));
         VALUE secprec = rb_ary_entry(rb_funcall(ary, rb_intern("flatten"), 0), 0);
         if (RTEST(secprec))
            rb_iv_set(self, "@secondprecision", rb_funcall(secprec, ID_TO_I, 0));
      }

      if (argc > 0)
         bts_assign(self, val);
   }

   /* allow read access to format */
   rb_define_attr(CLASS_OF(self), "default_fmt", 1, 0);

   return self;
}

VALUE bts_coerce(VALUE self, VALUE obj)
{
   if (!RTEST(rb_obj_is_kind_of(obj, cBaseTimestamp)))
      rb_raise(rb_eTypeError, "%s cannot be coerced into %s", rb_class2name(CLASS_OF(self)), rb_class2name(CLASS_OF(obj)));

   oci9_define_buf *bp, *bpobj, *bpout;
   VALUE out = datatype_new(0, NULL, CLASS_OF(obj));
   VALUE ary = rb_ary_new3(2, obj, out);
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(obj, oci9_define_buf, bpobj);
   Data_Get_Struct(out, oci9_define_buf, bpout);

   if (bp->ind != 0 || bpobj->ind != 0) { bpout->ind = -1; return ary; }
   if (OCIDateTimeConvert(datatype_get_session_handle(self), err_h, bp->desc, bpout->desc))
      error_raise("OCIDateTimeConvert failed", "bts_coerce", __FILE__, __LINE__);
   return ary;
}

VALUE bts_add(VALUE self, VALUE obj)
{
   dvoid *hdl = datatype_get_session_handle(self);
   oci9_define_buf *bp, *bpin, *bpout;
   VALUE in, out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(out, oci9_define_buf, bpout);

   if (bp->ind != 0) { bpout->ind = -1; return out; }

   if (RTEST(rb_obj_is_kind_of(obj, cBaseInterval)))
      in = obj;
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)) || TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
      in = datatype_new(1, &obj, cIntervalDS);
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   Data_Get_Struct(in, oci9_define_buf, bpin);
   if (bpin->ind != 0) { bpout->ind = -1; return out; }
   if (OCIDateTimeIntervalAdd(hdl, err_h, bp->desc, bpin->desc, bpout->desc))
      error_raise("OCIDateTimeIntervalAdd failed", "bts_add", __FILE__, __LINE__);
   return out;
}

VALUE bts_subtract(VALUE self, VALUE obj)
{
   dvoid *hdl = datatype_get_session_handle(self);
   oci9_define_buf *bp, *bpobj, *bpin, *bpout = NULL;
   Data_Get_Struct(self, oci9_define_buf, bp);
   VALUE in, out = Qnil;

   if (bp->ind != 0) { bpout->ind = -1; return out; }

   if (RTEST(rb_obj_is_kind_of(obj, cBaseTimestamp)))
   {
      Data_Get_Struct(obj, oci9_define_buf, bpobj);
      if (bpobj->ind != 0) { bpout->ind = -1; return out; }
      out = datatype_new(0, NULL, cIntervalDS);
      Data_Get_Struct(out, oci9_define_buf, bpout);
      if (OCIDateTimeSubtract(hdl, err_h, bp->desc, bpobj->desc, bpout->desc))
         error_raise("OCIDateTimeSubtract failed", "bts_subtract", __FILE__, __LINE__);
      return out;
   }

   if (RTEST(rb_obj_is_kind_of(obj, cBaseInterval)))
      in = obj;
   else if (RTEST(rb_obj_is_kind_of(obj, cNumber)) || TYPE(obj) == T_FIXNUM || TYPE(obj) == T_BIGNUM || TYPE(obj) == T_FLOAT)
      in = datatype_new(1, &obj, cIntervalDS);
   else
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   out = datatype_new(0, NULL, CLASS_OF(self));
   Data_Get_Struct(out, oci9_define_buf, bpout);
   Data_Get_Struct(in, oci9_define_buf, bpin);
   if (bpin->ind != 0) { bpout->ind = -1; return out; }
   if (OCIDateTimeIntervalSub(hdl, err_h, bp->desc, bpin->desc, bpout->desc))
      error_raise("OCIDateTimeIntervalSub failed", "bts_subtract", __FILE__, __LINE__);
   return out;
}

VALUE bts_compare(VALUE self, VALUE obj)
{
   if (!RTEST(rb_obj_is_kind_of(obj, cBaseTimestamp)))
      rb_raise(rb_eTypeError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(obj)));

   sword result;
   oci9_define_buf *bp, *bpobj, *bpts;
   VALUE ts;
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(obj, oci9_define_buf, bpobj);

   if (bp->ind != 0 || bpobj->ind != 0) return Qnil;

   if (RTEST(rb_obj_is_kind_of(obj, CLASS_OF(self))))
      ts = obj;
   else
      ts = rb_ary_entry(bts_coerce(obj, self), 1);

   Data_Get_Struct(ts, oci9_define_buf, bpts);
   if (OCIDateTimeCompare(datatype_get_session_handle(self), err_h, bp->desc, bpts->desc, &result))
      error_raise("OCIDateTimeCompare failed", "bts_compare", __FILE__, __LINE__);
   return INT2FIX(result);
}

VALUE bts_year(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   sb2 year;
   ub1 month, day;
   if (OCIDateTimeGetDate(datatype_get_session_handle(self), err_h, (OCIDateTime*) bp->desc, &year, &month, &day))
      error_raise("OCIDateTimeGetDate failed", "bts_year", __FILE__, __LINE__);
   return INT2FIX(year);
}

VALUE bts_month(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   sb2 year;
   ub1 month, day;
   if (OCIDateTimeGetDate(datatype_get_session_handle(self), err_h, (OCIDateTime*) bp->desc, &year, &month, &day))
      error_raise("OCIDateTimeGetDate failed", "bts_month", __FILE__, __LINE__);
   return INT2FIX(month);
}

VALUE bts_day(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   sb2 year;
   ub1 month, day;
   if (OCIDateTimeGetDate(datatype_get_session_handle(self), err_h, (OCIDateTime*) bp->desc, &year, &month, &day))
      error_raise("OCIDateTimeGetDate failed", "bts_day", __FILE__, __LINE__);
   return INT2FIX(day);
}

VALUE bts_hour(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   ub1 hour, min, sec;
   ub4 fsec;
   if (OCIDateTimeGetTime(datatype_get_session_handle(self), err_h, (OCIDateTime*) bp->desc, &hour, &min, &sec, &fsec))
      error_raise("OCIDateTimeGetTime failed", "bts_hour", __FILE__, __LINE__);
   return INT2FIX(hour);
}

VALUE bts_min(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   ub1 hour, min, sec;
   ub4 fsec;
   if (OCIDateTimeGetTime(datatype_get_session_handle(self), err_h, (OCIDateTime*) bp->desc, &hour, &min, &sec, &fsec))
      error_raise("OCIDateTimeGetTime failed", "bts_min", __FILE__, __LINE__);
   return INT2FIX(min);
}

VALUE bts_sec(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   ub1 hour, min, sec;
   ub4 fsec;
   if (OCIDateTimeGetTime(datatype_get_session_handle(self), err_h, (OCIDateTime*) bp->desc, &hour, &min, &sec, &fsec))
      error_raise("OCIDateTimeGetTime failed", "bts_sec", __FILE__, __LINE__);
   return INT2FIX(sec);
}

VALUE bts_usec(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   ub1 hour, min, sec;
   ub4 fsec;
   if (OCIDateTimeGetTime(datatype_get_session_handle(self), err_h, (OCIDateTime*) bp->desc, &hour, &min, &sec, &fsec))
      error_raise("OCIDateTimeGetTime failed", "bts_usec", __FILE__, __LINE__);
   return INT2FIX(fsec / 1000);
}

VALUE bts_fsec(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   ub1 hour, min, sec;
   ub4 fsec;
   if (OCIDateTimeGetTime(datatype_get_session_handle(self), err_h, (OCIDateTime*) bp->desc, &hour, &min, &sec, &fsec))
      error_raise("OCIDateTimeGetTime failed", "bts_fsec", __FILE__, __LINE__);
   return INT2FIX(fsec);
}

VALUE bts_do_to_s(dvoid* hdl, OCIDateTime *date, char *fmt, ub1 secprec)
{
   char buf[MAX_TS_LEN];
   ub4 buflen = MAX_TS_LEN;
   if (OCIDateTimeToText(hdl, err_h, date, fmt, strlen(fmt), secprec, 0, 0, &buflen, buf))
      error_raise("OCIDateTimeToText failed", "bts_do_to_s", __FILE__, __LINE__);
   return rb_str_new(buf, buflen);
}

VALUE bts_wday(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0) return Qnil;
   return rb_funcall(bts_do_to_s(datatype_get_session_handle(self), (OCIDateTime*) bp->desc, "D",
      FIX2INT(rb_iv_get(self, "@secondprecision"))), ID_TO_I, 0);
}

VALUE bts_yday(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0) return Qnil;
   return rb_funcall(bts_do_to_s(datatype_get_session_handle(self), (OCIDateTime*) bp->desc, "DDD",
      FIX2INT(rb_iv_get(self, "@secondprecision"))), ID_TO_I, 0);
}

VALUE bts_to_a(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   sb2 year;
   ub1 month, day, hour, min, sec;
   ub4 fsec;
   dvoid *hdl = datatype_get_session_handle(self);
   ub1 secprec = FIX2INT(rb_iv_get(self, "@secondprecision"));
   if (OCIDateTimeGetDate(hdl, err_h, (OCIDateTime*) bp->desc, &year, &month, &day))
      error_raise("OCIDateTimeGetDate failed", "bts_to_a", __FILE__, __LINE__);
   if (OCIDateTimeGetTime(hdl, err_h, (OCIDateTime*) bp->desc, &hour, &min, &sec, &fsec))
      error_raise("OCIDateTimeGetTime failed", "bts_to_a", __FILE__, __LINE__);
   return rb_ary_new3(8, INT2FIX(sec), INT2FIX(min), INT2FIX(hour), INT2FIX(day), INT2FIX(month), INT2FIX(year),
      INT2FIX(FIX2INT(rb_funcall(bts_do_to_s(hdl, (OCIDateTime*) bp->desc, "D", secprec), ID_TO_I, 0)) - 1),
      rb_funcall(bts_do_to_s(hdl, (OCIDateTime*) bp->desc, "DDD", secprec), ID_TO_I, 0));
}

VALUE bts_to_s(int argc, VALUE *argv, VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return rb_str_new2("NULL");

   VALUE fmt;
   rb_scan_args(argc, argv, "01", &fmt);
   if (argc == 0)
      fmt = rb_iv_get(self, "@default_fmt");
   return bts_do_to_s(datatype_get_session_handle(self), (OCIDateTime*) bp->desc, RSTRING(fmt)->ptr,
      FIX2INT(rb_iv_get(self, "@secondprecision")));
}

VALUE bts_oci_array(int argc, VALUE *argv, VALUE self)
{
   VALUE tz_offset;
   rb_scan_args(argc, argv, "01", &tz_offset);
   if (argc == 0)
      tz_offset = datatype_new(0, NULL, cIntervalDS);
   else if (!RTEST(rb_obj_is_kind_of(tz_offset, cIntervalDS)))
      rb_raise(rb_eTypeError, "argument is wrong type (found %s, expected %s)", rb_class2name(CLASS_OF(tz_offset)),
         rb_class2name(cIntervalDS));

   oci9_define_buf *bp, *bptz;
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(tz_offset, oci9_define_buf, bptz);

   if (bp->ind != 0 || bptz->ind != 0) return Qnil;

   ub1 ary[MAX_TS_ARRAY_SZ];
   ub4 arylen = MAX_TS_ARRAY_SZ;
   if (OCIDateTimeToArray(datatype_get_session_handle(self), err_h, bp->desc, bptz->desc, ary, &arylen,
         FIX2INT(rb_iv_get(self, "@secondprecision"))))
      error_raise("OCIDateTimeToArray failed", "bts_oci_array", __FILE__, __LINE__);
   VALUE v_ary = rb_ary_new3(6, INT2FIX((ary[0]-100)*100 + ary[1]-100), INT2FIX(ary[2]), INT2FIX(ary[3]),
      INT2FIX(ary[4]-1), INT2FIX(ary[5]-1), INT2FIX(ary[6]-1));
   if (arylen >= 11)
      rb_ary_push(v_ary, INT2FIX((ary[7] << 24) + (ary[8] << 16) + (ary[9] << 8) + ary[10]));
   if (arylen == 13)
      rb_ary_push(rb_ary_push(v_ary, INT2FIX(ary[11]-20)), INT2FIX(ary[12]-60));
   return v_ary;
}

VALUE bts_to_builtin(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   /* get array with time zone offset of zero */
   VALUE v_ary = bts_oci_array(0, NULL, self);
   VALUE ary[7];
   int i, arylen = FIX2INT(rb_funcall(v_ary, ID_SIZE, 0));
   for (i=0; i<7 && i<arylen; i++)
      ary[i] = rb_ary_entry(v_ary, i);
   if (arylen == 6)
      ary[6] = INT2FIX(0);
   else
      ary[6] = INT2FIX(FIX2INT(ary[6]) / 1000);
   return rb_funcall2(rb_cTime, ID_GM, 7, ary);
}

VALUE bts_set_default_fmt(VALUE self, VALUE fmt)
{
   if (TYPE(fmt) != T_STRING)
      rb_raise(rb_eTypeError, "argument is wrong type (found %s, expected String)", rb_class2name(CLASS_OF(fmt)));
   rb_iv_set(self, "@default_fmt", fmt);
   return self;
}

VALUE bts_set_secprec(VALUE self, VALUE secprec)
{
   if (TYPE(secprec) != T_FIXNUM)
      rb_raise(rb_eTypeError, "argument is wrong type (found %s, expected Fixnum)", rb_class2name(CLASS_OF(secprec)));
   long val = FIX2INT(secprec);
   if (val < 0 || val > 9)
      rb_raise(rb_eRangeError, "Fixint %ld out of range (expected 0-9)", val);
   rb_iv_set(self, "@secondprecision", secprec);
   return self;
}

void Init_BaseTimestamp()
{
   rb_include_module(cBaseTimestamp, rb_mComparable);

   rb_define_singleton_method(cBaseTimestamp, "systimestamp", bts_systimestamp, 0);
   rb_define_singleton_method(cBaseTimestamp, "new", bts_new, -1);

   rb_define_method(cBaseTimestamp, "initialize", bts_initialize, -1);
   rb_enable_super(cBaseTimestamp, "initialize");
   rb_define_method(cBaseTimestamp, "assign", bts_assign, 1);
   rb_define_method(cBaseTimestamp, "coerce", bts_coerce, 1);
   rb_define_method(cBaseTimestamp, "+", bts_add, 1);
   rb_define_method(cBaseTimestamp, "-", bts_subtract, 1);
   rb_define_method(cBaseTimestamp, "<=>", bts_compare, 1);
   rb_define_method(cBaseTimestamp, "year", bts_year, 0);
   rb_define_method(cBaseTimestamp, "mon", bts_month, 0);
   rb_define_method(cBaseTimestamp, "month", bts_month, 0);
   rb_define_method(cBaseTimestamp, "day", bts_day, 0);
   rb_define_method(cBaseTimestamp, "hour", bts_hour, 0);
   rb_define_method(cBaseTimestamp, "min", bts_min, 0);
   rb_define_method(cBaseTimestamp, "sec", bts_sec, 0);
   rb_define_method(cBaseTimestamp, "usec", bts_usec, 0);
   rb_define_method(cBaseTimestamp, "fsec", bts_fsec, 0);
   rb_define_method(cBaseTimestamp, "wday", bts_wday, 0);
   rb_define_method(cBaseTimestamp, "yday", bts_yday, 0);
   rb_define_method(cBaseTimestamp, "default_fmt=", bts_set_default_fmt, 1);
   rb_define_method(cBaseTimestamp, "secondprecision=", bts_set_secprec, 1);
   rb_define_method(cBaseTimestamp, "to_a", bts_to_a, 0);
   rb_define_method(cBaseTimestamp, "to_s", bts_to_s, -1);
   rb_define_method(cBaseTimestamp, "oci_array", bts_oci_array, -1);
   rb_define_method(cBaseTimestamp, "to_builtin", bts_to_builtin, 0);

   /* initialize global var with zero offset */
   if (OCIDescriptorAlloc(env_h, (dvoid**) &UTC_OFFSET, OCI_DTYPE_INTERVAL_DS, 0, 0))
      error_raise("OCIDescriptorAlloc failed", "Init_Timestamp", __FILE__, __LINE__);
   if (OCIIntervalSetDaySecond(env_h, err_h, 0, -7, 0, 0, 0, UTC_OFFSET))
      error_raise("OCIIntervalSetDaySecond failed", "Init_Timestamp", __FILE__, __LINE__);
}
