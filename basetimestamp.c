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

#define MAX_TS_LEN 100
#define MAX_TS_ARRAY_SZ 13

OCIInterval *UTC_OFFSET;

void bts_get_tz_offset(dvoid *hdl, char *tz, sb4 *hr, sb4 *min)
{
   OCIInterval* interval;
   if (OCIDescriptorAlloc(hdl, (dvoid**) &interval, OCI_DTYPE_INTERVAL_DS, 0, 0))
      error_raise("Could not allocate interval descriptor", "bts_get_tz_offset", __FILE__, __LINE__);
   if (OCIIntervalFromTZ(hdl, err_h, (CONST oratext*) tz, strlen(tz), interval))
      error_raise("Could not get interval from time zone", "bts_get_tz_offset", __FILE__, __LINE__);
   sb4 day, sec, fsec;
   if (OCIIntervalGetDaySecond(hdl, err_h, &day, hr, min, &sec, &fsec, interval))
      error_raise("Could not set interval", "bts_get_tz_offset", __FILE__, __LINE__);
   if (OCIDescriptorFree(interval, OCI_DTYPE_INTERVAL_DS))
      error_raise("Could not free interval descriptor", "bts_get_tz_offset", __FILE__, __LINE__);
}

VALUE bts_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (OCIDescriptorAlloc(env_h, &bp->desc, bp->dtype, 0, 0))
      error_raise("Could not allocate OCIDateTime descriptor", "bts_initialize", __FILE__, __LINE__);
   bp->len = sizeof(bp->desc);
   dvoid *hdl = datatype_get_session_handle(self);

   switch (argc)
   {
      case 0:
         /* initialize with current time */
         if (OCIDateTimeSysTimeStamp(hdl, err_h, bp->desc))
            error_raise("Could not set time", "bts_initialize", __FILE__, __LINE__);
         bp->ind = 0;
         break;

      case 1:
      case 2:
         /* argv[] = { (String | Time) [scale] } */
         bp->ind = 0;
         if (argc == 2)
         {
            if (TYPE(argv[1]) != T_FIXNUM)
               rb_raise(rb_eArgError, "argument 2 is wrong type %s (expected Fixnum)", rb_class2name(argv[1]));
            rb_iv_set(self, "@scale", argv[1]);
         }
         else
            rb_iv_set(self, "@scale", INT2FIX(6));

         if (TYPE(argv[0]) == T_STRING)
         {
            int tslen, fmtlen;
            char *ts = rb_str2cstr(argv[0], &tslen);
            char *fmt = rb_str2cstr(rb_iv_get(self, "@default_fmt"), &fmtlen);
            if (OCIDateTimeFromText(env_h, err_h, ts, tslen, fmt, fmtlen, 0, 0, bp->desc))
               error_raise("Could not convert text to timestamp", "bts_initialize", __FILE__, __LINE__);
         }
         else if (RTEST(rb_obj_is_kind_of(argv[0], rb_cTime)))
         {
            /* build array */
            VALUE v_ary = rb_funcall(argv[0], rb_intern("to_a"), 0);
            ub1 ary[13];
            ID id_subscript = rb_intern("[]");
            int yr = FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(5))); /* year */
            ary[0] = yr / 100 + 100;
            ary[1] = yr % 100 + 100;
            ary[2] = FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(4))); /* month */
            ary[3] = FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(3))); /* day */
            ary[4] = FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(2))) + 1; /* hour */
            ary[5] = FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(1))) + 1; /* minute */
            ary[6] = FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(0))) + 1; /* second */
            long fsec = FIX2LONG(rb_funcall(argv[0], rb_intern("usec"), 0)) * 1000; /* fractional secs */
            ary[7] = (fsec >> 24) & 0xff;
            ary[8] = (fsec >> 16) & 0xff;
            ary[9] = (fsec >> 8) & 0xff;
            ary[10] = fsec & 0xff;
            long offset_mins = FIX2INT(rb_funcall(argv[0], rb_intern("utc_offset"), 0)) / 60; /* tz offset */
            ary[11] = offset_mins / 60 + 20;
            ary[12] = offset_mins % 60 + 60;

            /* make an interval containing the time zone offset from local time */
            //long local_offset_mins = FIX2INT(rb_funcall(rb_funcall(rb_cTime, rb_intern("now"), 0), rb_intern("utc_offset"), 0)) / 60;
            OCIInterval *tz_offset;
            if (OCIDescriptorAlloc(env_h, (dvoid**) &tz_offset, OCI_DTYPE_INTERVAL_DS, 0, 0))
               error_raise("Could not allocate interval descriptor", "bts_initialize", __FILE__, __LINE__);
            //if (OCIIntervalSetDaySecond(hdl, err_h, 0, local_offset_mins / 60, local_offset_mins % 60, 0, 0, tz_offset))
            //if (OCIIntervalSetDaySecond(hdl, err_h, 0, 0, local_offset_mins % 60, 0, 0, tz_offset))
            if (OCIIntervalSetDaySecond(hdl, err_h, 0, offset_mins / 60, offset_mins % 60, 0, 0, tz_offset))
               error_raise("Could not set interval", "bts_initialize", __FILE__, __LINE__);

            /* set the date/time */
            if (OCIDateTimeFromArray(hdl, err_h, ary, sizeof(ary), bp->sqlt, bp->desc, tz_offset,
                  (ub1) FIX2INT(rb_iv_get(self, "@scale"))))
               error_raise("Could not convert Time to timestamp", "bts_initialize", __FILE__, __LINE__);

            /* free the interval */
            if (OCIDescriptorFree(tz_offset, OCI_DTYPE_INTERVAL_DS))
               error_raise("Could not free interval descriptor", "bts_initialize", __FILE__, __LINE__);
         }
         else
            rb_raise(rb_eArgError, "argument 1 is wrong type %s (expected String or Time)", rb_class2name(argv[0]));

      case 5:
         /* argv[] = { internal SQLT, size, precision, scale, ses_h }
          * used only when defining select-list in Statement#allocate_row
          *
          * prepare space for an OCIDateTime
          */
         break;

      default:
         rb_raise(rb_eArgError, "wrong # of arguments(%d for 1, 2 or 5)", argc);
   }

   bp->val = &bp->desc;

   return self;
}

VALUE bts_add(VALUE self, VALUE v_obj)
{
   if (!rb_obj_is_kind_of(v_obj, cBaseInterval))
      rb_raise(rb_eArgError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(v_obj)));

   dvoid *hdl = datatype_get_session_handle(self);
   oci9_define_buf *bp, *bpobj, *bpout;
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(v_obj, oci9_define_buf, bpobj);

   VALUE out = datatype_new(0, 0, CLASS_OF(self)); // sometimes wants timestamp; sometimes wants timestamp_tz ???
   Data_Get_Struct(out, oci9_define_buf, bpout);
//printf("%s(%d,%d) + %s(%d,%d) = %s(%d,%d)\n", rb_class2name(CLASS_OF(self)), bp->sqlt, bp->dtype, rb_class2name(CLASS_OF(v_obj)), bpobj->sqlt, bpobj->dtype, rb_class2name(CLASS_OF(out)), bpout->sqlt, bpout->dtype);
   if (OCIDateTimeIntervalAdd(hdl, err_h, bp->desc, bpobj->desc, bpout->desc))
      error_raise("OCIDateTimeIntervalAdd failed", "bts_add", __FILE__, __LINE__);
   return out;
}

VALUE bts_subtract(VALUE self, VALUE v_obj)
{
   dvoid *hdl = datatype_get_session_handle(self);
   oci9_define_buf *bp, *bpobj, *bpout;
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(v_obj, oci9_define_buf, bpobj);
   VALUE out;

   if (rb_obj_is_kind_of(v_obj, cBaseInterval))
   {
      out = datatype_new(0, 0, CLASS_OF(self));
      Data_Get_Struct(out, oci9_define_buf, bpout);
      if (OCIDateTimeIntervalSub(hdl, err_h, bp->desc, bpobj->desc, bpout->desc))
         error_raise("OCIDateTimeIntervalSub failed", "bts_subtract", __FILE__, __LINE__);
   }
   else if (rb_obj_is_kind_of(v_obj, cBaseTimestamp))
   {
      out = datatype_new(0, 0, cIntervalDayToSecond);
      Data_Get_Struct(out, oci9_define_buf, bpout);
      if (OCIDateTimeSubtract(hdl, err_h, bp->desc, bpobj->desc, bpout->desc))
         error_raise("OCIDateTimeSubtract failed", "bts_subtract", __FILE__, __LINE__);
   }
   else
      rb_raise(rb_eArgError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(v_obj)));

   return out;
}

VALUE bts_assign(VALUE self, VALUE v_obj)
{
   dvoid *hdl = datatype_get_session_handle(self);
   oci9_define_buf *bp, *bpobj;
   Data_Get_Struct(self, oci9_define_buf, bp);
   Data_Get_Struct(v_obj, oci9_define_buf, bpobj);

   if (rb_obj_is_kind_of(v_obj, cBaseTimestamp))
   {
      if (OCIDateTimeConvert(hdl, err_h, bpobj->desc, bp->desc))
         error_raise("OCIDateTimeAssign failed", "bts_assign", __FILE__, __LINE__);
   }
   else if (rb_obj_is_kind_of(v_obj, rb_cTime))
   {
      VALUE v_ary = rb_funcall(v_obj, rb_intern("to_a"), 0);
      ID id_subscript = rb_intern("[]");
      long tz_offset_mins = FIX2INT(rb_funcall(v_obj, rb_intern("utc_offset"), 0)) / 60;
      char tz_offset[7];
      snprintf(tz_offset, 7, "%+02ld:%02ld", tz_offset_mins / 60, tz_offset_mins % 60);
      if (OCIDateTimeConstruct(hdl, err_h, bp->desc,
            FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(5))),
            FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(4))),
            FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(3))),
            FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(2))),
            FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(1))),
            FIX2INT(rb_funcall(v_ary, id_subscript, 1, INT2FIX(0))),
            FIX2LONG(rb_funcall(v_obj, rb_intern("usec"), 0)) * 1000,
            tz_offset, strlen(tz_offset)))
         error_raise("OCIDateTimeConstruct failed", "bts_assign", __FILE__, __LINE__);
   }
   else
      rb_raise(rb_eArgError, "no implicit conversion of type %s", rb_class2name(CLASS_OF(v_obj)));

   return self;
}

VALUE bts_to_s(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return rb_str_new2("NULL");

   char buf[MAX_TS_LEN];
   ub4 buflen = (ub4) MAX_TS_LEN;
   dvoid *hdl = datatype_get_session_handle(self);
   int fmtlen;
   char * fmt = rb_str2cstr(rb_iv_get(self, "@default_fmt"), &fmtlen);
   if (OCIDateTimeToText(hdl, err_h, bp->desc, fmt, fmtlen, (ub1) FIX2INT(rb_iv_get(self, "@scale")),
         0, 0, (ub4*) &buflen, (OraText*) buf))
      error_raise("Could not convert timestamp to text", "bts_to_s", __FILE__, __LINE__);
   return rb_str_new(buf, buflen);
}

VALUE bts_to_builtin(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0)
      return Qnil;

   ub1 ary[MAX_TS_ARRAY_SZ];
   ub4 arylen = MAX_TS_ARRAY_SZ;

   if (OCIDateTimeToArray(env_h, err_h, bp->desc, UTC_OFFSET, ary, &arylen, (ub1) FIX2INT(rb_iv_get(self, "@scale"))))
      error_raise("Could not convert timestamp to array", "bts_to_builtin", __FILE__, __LINE__);
   
   ID method = rb_intern("gm");
   if (bp->sqlt == SQLT_TIMESTAMP)
      method = rb_intern("local");
   return rb_funcall(rb_cTime, method, 7, INT2FIX((ary[0]-100)*100 + ary[1]-100), INT2FIX(ary[2]), INT2FIX(ary[3]),
      INT2FIX(ary[4]-1), INT2FIX(ary[5]-1), INT2FIX(ary[6]-1),
      INT2FIX(((ary[7] << 24) + (ary[8] << 16) + (ary[9] << 8) + ary[10]) / 1000));
}

void Init_BaseTimestamp()
{
   rb_define_method(cBaseTimestamp, "initialize", bts_initialize, -1);
   rb_enable_super(cBaseTimestamp, "initialize");
   rb_define_method(cBaseTimestamp, "assign", bts_assign, 1);
   rb_define_method(cBaseTimestamp, "+", bts_add, 1);
   rb_define_method(cBaseTimestamp, "-", bts_subtract, 1);
   rb_define_method(cBaseTimestamp, "to_s", bts_to_s, 0);
   rb_define_method(cBaseTimestamp, "to_builtin", bts_to_builtin, 0);

   /* initialize global var with zero offset */
   if (OCIDescriptorAlloc(env_h, (dvoid**) &UTC_OFFSET, OCI_DTYPE_INTERVAL_DS, 0, 0))
      error_raise("Could not allocate interval descriptor", "Init_Timestamp", __FILE__, __LINE__);
   if (OCIIntervalSetDaySecond(env_h, err_h, 0, 0, 0, 0, 0, UTC_OFFSET))
      error_raise("Could not set interval", "Init_Timestamp", __FILE__, __LINE__);
}
