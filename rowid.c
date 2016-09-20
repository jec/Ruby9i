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

#define ROWID_MAX_LEN 100

VALUE rowid_initialize(int argc, VALUE *argv, VALUE self)
{
   rb_call_super(argc, argv);

   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   /* allocate a rowid descriptor */
   if (OCIDescriptorAlloc(env_h, &bp->desc, OCI_DTYPE_ROWID, 0, 0))
      error_raise("Could not allocate ROWID descriptor", "rowid_initialize", __FILE__, __LINE__);
   bp->len = sizeof(bp->desc);

   /* read/write this as an OCIRowid */
   bp->sqlt = SQLT_RDD;
   bp->val = &bp->desc;
   bp->dtype = OCI_DTYPE_ROWID;

   return self;
}

VALUE rowid_do_to_s(oci9_define_buf *bp)
{
   char buf[ROWID_MAX_LEN];
   ub2 buflen = (ub2) ROWID_MAX_LEN;
   if (OCIRowidToChar((OCIRowid*) bp->desc, (OraText*) buf, (ub2*) &buflen, (OCIError*) err_h))
      error_raise("Could not convert ROWID to text", "rowid_do_to_s", __FILE__, __LINE__);
   return rb_str_new(buf, buflen);
}

VALUE rowid_to_s(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0)
      return rb_str_new2("NULL");
   return rowid_do_to_s(bp);
}

VALUE rowid_to_builtin(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);
   if (bp->ind != 0)
      return Qnil;
   return rowid_do_to_s(bp);
}

void Init_Rowid()
{
   rb_define_method(cRowid, "initialize", rowid_initialize, -1);
   rb_enable_super(cRowid, "initialize");
   rb_define_method(cRowid, "to_s", rowid_to_s, 0);
   rb_define_method(cRowid, "to_builtin", rowid_to_builtin, 0);

   /* register types handled by me */
   registry_add(TYPE_REGISTRY, SQLT_RDD, cRowid);
}
