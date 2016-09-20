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

void handle_free(void *p)
{
   /* Don't think anything is necessary here. OCI will dispose of the space
    * held by the handles when they're no longer used (e.g. disconnect).
    */
}

VALUE handle_new(VALUE htype)
{
   oci9_handle *hp;
   VALUE handle = Data_Make_Struct(cHandle, oci9_handle, 0, handle_free, hp);
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
