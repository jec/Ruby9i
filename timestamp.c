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

VALUE ts_initialize(int argc, VALUE *argv, VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   /* initialize values needed by super, then call super */
   bp->sqlt = SQLT_TIMESTAMP;
   bp->dtype = OCI_DTYPE_TIMESTAMP;
   rb_iv_set(self, "@default_fmt", rb_str_new2("yyyy-mm-dd hh24:mi:ss.ff6"));
   rb_call_super(argc, argv);

   return self;
}

VALUE ts_to_builtin(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind != 0) return Qnil;

   /* get array with time zone offset of zero (ignored for timestamp) */
   VALUE v_ary = rb_funcall(self, ID_OCI_ARRAY, 0);
   VALUE ary[7];
   int i, arylen = FIX2INT(rb_funcall(v_ary, ID_SIZE, 0));
   for (i=0; i<7 && i<arylen; i++)
      ary[i] = rb_ary_entry(v_ary, i);
   if (arylen == 6)
      ary[6] = INT2FIX(0);
   else
      ary[6] = INT2FIX(FIX2INT(ary[6]) / 1000);
   return rb_funcall2(rb_cTime, ID_LOCAL, 7, ary);
}

void Init_Timestamp()
{
   rb_define_method(cTimestamp, "initialize", ts_initialize, -1);
   rb_enable_super(cTimestamp, "initialize");
   rb_define_method(cTimestamp, "to_builtin", ts_to_builtin, 0);

   /* register types handled by me */
   registry_add(TYPE_REGISTRY, SQLT_TIMESTAMP, cTimestamp);
}
