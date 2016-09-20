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

VALUE ivym_initialize(int argc, VALUE *argv, VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   bp->sqlt = SQLT_INTERVAL_YM;
   bp->dtype = OCI_DTYPE_INTERVAL_YM;
   rb_iv_set(self, "@precision", INT2FIX(0)); /* not used for year to month */
   rb_iv_set(self, "@scale", INT2FIX(2)); /* # of year digits */
   rb_iv_set(self, "@zero", rb_str_new2("0-0"));
   rb_call_super(argc, argv);
   return self;
}

void Init_IntervalYearToMonth()
{
   rb_define_method(cIntervalYearToMonth, "initialize", ivym_initialize, -1);
   rb_enable_super(cIntervalYearToMonth, "initialize");
}
