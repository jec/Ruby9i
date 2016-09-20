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

void handle_mark(void *bp)
{
}

VALUE handle_wrap(dvoid *hdl)
{
   oci9_handle *hp;
   VALUE handle = Data_Make_Struct(cHandle, oci9_handle, handle_mark, 0, hp);
   hp->ptr = hdl;
   return handle;
}

VALUE handle_new(VALUE htype)
{
   oci9_handle *hp;
   VALUE handle = Data_Make_Struct(cHandle, oci9_handle, handle_mark, 0, hp);
   rb_obj_call_init(handle, 1, &htype);
   return handle;
}

VALUE handle_initialize(VALUE self, VALUE htype)
{
   oci9_handle *hp;
   Data_Get_Struct(self, oci9_handle, hp);
   if (OCIHandleAlloc(env_h, &hp->ptr, (ub4) FIX2INT(htype), 0, 0))
      error_raise("Could not create OCI handle", "handle_initialize", __FILE__, __LINE__);
   return self;
}

void Init_Handle()
{
   rb_define_method(cHandle, "initialize", handle_initialize, 1);
}
