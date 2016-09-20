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

#define TS_BUF_SIZE 100

VALUE tstz_initialize(int argc, VALUE *argv, VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   /* initialize values needed by super, then call super */
   bp->sqlt = SQLT_TIMESTAMP_TZ;
   bp->dtype = OCI_DTYPE_TIMESTAMP_TZ;
   rb_iv_set(self, "@default_fmt", rb_str_new2("yyyy-mm-dd hh24:mi:ss.ff6 tzh:tzm"));
   rb_call_super(argc, argv);

   return self;
}

VALUE tstz_to_a(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   VALUE ary = rb_call_super(0, NULL);
   char buf[TS_BUF_SIZE];
   ub4 buflen = TS_BUF_SIZE;
   if (OCIDateTimeGetTimeZoneName(datatype_get_session_handle(self), err_h, bp->desc, buf, &buflen))
      error_raise("OCIDateTimeGetTimeZoneName failed", "tstz_to_a", __FILE__, __LINE__);
   rb_ary_push(ary, rb_str_new(buf, buflen));
   return ary;
}

void Init_TimestampTZ()
{
   rb_define_method(cTimestampTZ, "initialize", tstz_initialize, -1);
   rb_enable_super(cTimestampTZ, "initialize");
   rb_define_method(cTimestampTZ, "to_a", tstz_to_a, 0);
   rb_enable_super(cTimestampTZ, "to_a");

   /* register types handled by me */
   registry_add(TYPE_REGISTRY, SQLT_TIMESTAMP_TZ, cTimestampTZ);
}
