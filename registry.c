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

void registry_add(VALUE registry, int type, VALUE klass)
{
   if (NIL_P(registry))
      rb_raise(rb_eLoadError, "Parent of %s not loaded first", rb_class2name(klass));
   rb_hash_aset(registry, INT2FIX(type), rb_ary_new3(2, klass, Qnil));
}

VALUE registry_get_super(VALUE registry, int type)
{
   if (NIL_P(registry))
      return Qnil;

   VALUE ary = rb_hash_aref(registry, INT2FIX(type));
   VALUE hash = rb_ary_entry(ary, 1);
   if (NIL_P(hash))
   {
      hash = rb_hash_new();
      rb_ary_store(ary, 1, hash);
   }
   return hash;
}
