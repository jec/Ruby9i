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

VALUE tsltz_initialize(int argc, VALUE *argv, VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   bp->sqlt = SQLT_TIMESTAMP_LTZ;
   bp->dtype = OCI_DTYPE_TIMESTAMP_LTZ;
   rb_iv_set(self, "@default_fmt", rb_str_new2("yyyy-mm-dd hh24:mi:ss.ff9"));
   rb_call_super(argc, argv);
   return self;
}

void Init_TimestampLocalTZ()
{
   rb_define_method(cTimestampLocalTZ, "initialize", tsltz_initialize, -1);
   rb_enable_super(cTimestampLocalTZ, "initialize");
}
