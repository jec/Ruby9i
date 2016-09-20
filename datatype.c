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

dvoid *datatype_get_session_handle(VALUE self)
{
   /* use the session handle if available */
   dvoid *hdl = env_h;
   VALUE v_ses_h = rb_iv_get(self, "@ses_h");
   if (RTEST(v_ses_h))
   {
      oci9_handle *ses_h;
      Data_Get_Struct(v_ses_h, oci9_handle, ses_h);
      if (ses_h->ptr)
         hdl = ses_h->ptr;
   }
   return hdl;
}

void datatype_free(void *bp)
{
   /* free the descriptor for the OCI type if there is one */
   if (((oci9_define_buf*)bp)->desc)
      if (OCIDescriptorFree(((oci9_define_buf*)bp)->desc, ((oci9_define_buf*)bp)->dtype))
         error_raise("Could not free descriptor", "datatype_free", __FILE__, __LINE__);
}

VALUE datatype_new(int argc, VALUE *argv, VALUE klass)
{
   VALUE newclass = Qnil;
   VALUE datatype = Qnil;
   oci9_define_buf *bp;

   if (argc == 5)
      /* argv[] = { internal SQLT, size, precision, scale, ses_h }
       * used only when defining select-list in Statement#allocate_row
       *
       * create object of appropriate type based on internal type
       */
      switch ((ub2)FIX2INT(argv[0]))
      {
         case SQLT_RDD: /* rowid */
            newclass = cRowid;
            break;

         case SQLT_CHR: /* varchar2 */
         case SQLT_AFC: /* char */
            newclass = cVarchar;
            break;

         case SQLT_NUM: /* number */
         case SQLT_INT: /* integer */
         case SQLT_FLT: /* float */
            newclass = cNumber;
            break;

         case SQLT_DAT: /* date */
            newclass = cDate;
            break;

         case SQLT_TIMESTAMP: /* timestamp */
            newclass = cTimestamp;
            break;

         case SQLT_TIMESTAMP_TZ: /* timestamp with time zone */
            newclass = cTimestampTZ;
            break;

         case SQLT_TIMESTAMP_LTZ: /* timestamp with local time zone */
            newclass = cTimestampLocalTZ;
            break;

         case SQLT_INTERVAL_YM: /* interval year to month */
            newclass = cIntervalYearToMonth;
            break;

         case SQLT_INTERVAL_DS: /* interval day to second */
            newclass = cIntervalDayToSecond;
            break;

         default:
            rb_raise(rb_eArgError, "Unsupported SQL data type %d in SELECT list", (ub2) FIX2INT(argv[0]));
      }
   else
      /* called from subclass#new */
      newclass = klass;

   datatype = Data_Make_Struct(newclass, oci9_define_buf, 0, datatype_free, bp);
   bp->val = NULL;
   bp->desc = NULL;
   bp->dtype = 0;
   bp->len = 0;
   bp->ind = -1; /* initially NULL */
   bp->sqlt = SQLT_STR;
   rb_obj_call_init(datatype, argc, argv);
   return datatype;
}

VALUE datatype_initialize(int argc, VALUE *argv, VALUE self)
{
   /* set instance vars */
   if (argc == 5)
   {
      if (TYPE(argv[2]) != T_FIXNUM)
         rb_raise(rb_eArgError, "argument 3 is wrong type %s (expected Fixnum)", rb_class2name(argv[0]));
      if (TYPE(argv[3]) != T_FIXNUM)
         rb_raise(rb_eArgError, "argument 4 is wrong type %s (expected Fixnum)", rb_class2name(argv[0]));
      rb_iv_set(self, "@precision", argv[2]);
      rb_iv_set(self, "@scale", argv[3]);
      rb_iv_set(self, "@ses_h", argv[4]);
   }
   else
   {
      if (NIL_P(rb_iv_get(self, "@precision")))
         rb_iv_set(self, "@precision", INT2FIX(0));
      if (NIL_P(rb_iv_get(self, "@scale")))
         rb_iv_set(self, "@scale", INT2FIX(0));
      rb_iv_set(self, "@ses_h", Qnil);
   }

   /* make public read-only attributes */
   rb_define_attr(CLASS_OF(self), "precision", 1, 0);
   rb_define_attr(CLASS_OF(self), "scale", 1, 0);

   return self;
}

/* provide read-only access to the indicator to check truncated lengths */
VALUE datatype_indicator(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   return INT2FIX(bp->ind);
}

/* attempt to convert to a built-in type */
VALUE datatype_to_builtin(VALUE self)
{
   oci9_define_buf *bp;
   Data_Get_Struct(self, oci9_define_buf, bp);

   if (bp->ind == 0)
      /* by default just return self; subclasses should override this if necessary */
      return self;
   else
      return Qnil;
}

void Init_Datatype()
{
   rb_define_singleton_method(cDatatype, "new", datatype_new, -1);
   rb_define_method(cDatatype, "initialize", datatype_initialize, -1);
   rb_define_method(cDatatype, "indicator", datatype_indicator, 0);
   rb_define_method(cDatatype, "to_builtin", datatype_to_builtin, 0);
}
