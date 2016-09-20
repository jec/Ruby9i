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

#define MAX_BUF_SZ 1000

VALUE ERR_LOC_PTR, ERR_LOC_PTR_ESC;

VALUE error_exception(int argc, VALUE *argv, VALUE klass)
{
   VALUE err = rb_obj_alloc(eError);
   rb_obj_call_init(err, argc, argv);
   return err;
}

void error_raise(char *msg, char *mname, const char* fname, int lineno)
{
   VALUE argv[] = { rb_str_new2(msg), rb_str_new2(mname), rb_str_new2(fname), INT2FIX(lineno) };
   rb_exc_raise(error_exception(4, argv, eError));
}

void error_raise_sql(char *msg, char *mname, char *fname, int lineno, char *sql, int offset)
{
   VALUE argv[] = { rb_str_new2(msg), rb_str_new2(mname), rb_str_new2(fname), INT2FIX(lineno),
      rb_str_new2(sql), INT2FIX(offset) };
   rb_exc_raise(error_exception(6, argv, eError));
}

VALUE error_initialize(int argc, VALUE *argv, VALUE self)
{
   VALUE mesg, mname, fname, lineno, sql, offset;
   rb_scan_args(argc, argv, "06", &mesg, &mname, &fname, &lineno, &sql, &offset);

   rb_call_super(1, &mesg);
   rb_iv_set(self, "@methodname", mname);
   rb_iv_set(self, "@filename", fname);
   rb_iv_set(self, "@lineno", lineno);
   rb_iv_set(self, "@sql", sql);
   rb_iv_set(self, "@offset", offset);

   /* get Oracle error text */
   text buf[MAX_BUF_SZ];
   sb4 ora_code;
   if (OCIErrorGet(err_h, 1, NULL, &ora_code, buf, MAX_BUF_SZ, OCI_HTYPE_ERROR) == 0)
   {
      rb_iv_set(self, "@oramesg", rb_funcall(rb_str_new(buf, strlen(buf)), rb_intern("chomp!"), 0));
      rb_iv_set(self, "@oranum", INT2FIX(ora_code));
   }
   else
   {
      rb_iv_set(self, "@oramesg", Qnil);
      rb_iv_set(self, "@oranum", INT2FIX(0));
   }
   rb_define_attr(CLASS_OF(self), "oramesg", 1, 0);
   rb_define_attr(CLASS_OF(self), "oranum", 1, 0);

   return self;
}

void error_get_sql_errstr(VALUE self, VALUE *errline, VALUE *errptr)
{
   VALUE sql = rb_iv_get(self, "@sql");
   if (NIL_P(sql))
      return;
   VALUE offset = rb_iv_get(self,"@offset");
   if (NIL_P(offset))
      return;
   rb_funcall(sql, rb_intern("[]="), 3, offset, INT2FIX(0), ERR_LOC_PTR);
   VALUE not_crlf = rb_str_new2("[^\\r\\n]*");
   VALUE pattern = rb_str_dup(not_crlf);
   rb_str_concat(rb_str_concat(pattern, ERR_LOC_PTR_ESC), not_crlf);
   VALUE re = rb_funcall(rb_cRegexp, rb_intern("compile"), 1, pattern);
   if (RTEST(rb_funcall(re, rb_intern("match"), 1, sql)))
   {
      *errline = rb_funcall(rb_gv_get("$&"), rb_intern("sub"), 2, ERR_LOC_PTR_ESC, rb_str_new2(""));
      VALUE mdbegin = rb_funcall(rb_gv_get("$~"), rb_intern("begin"), 1, INT2FIX(0));
      *errptr = rb_str_concat(rb_funcall(rb_str_new2(" "), rb_intern("*"), 1, mdbegin), rb_str_new2("^"));
   }
}

VALUE error_to_s(VALUE self)
{
   VALUE v_mesg = rb_iv_get(self, "mesg");
   VALUE v_mname = rb_iv_get(self, "@methodname");
   VALUE v_fname = rb_iv_get(self, "@filename");
   VALUE v_lineno = rb_iv_get(self, "@lineno");
   VALUE v_oramesg = rb_iv_get(self, "@oramesg");
   VALUE v_errstr = Qnil;
   VALUE v_errptr = Qnil;
   error_get_sql_errstr(self, &v_errstr, &v_errptr);
   char str[MAX_BUF_SZ]="Error", mname[MAX_BUF_SZ]="", fname[MAX_BUF_SZ]="", lineno[MAX_BUF_SZ]="", oramesg[MAX_BUF_SZ]="";
   if (!NIL_P(v_mesg)) strncpy(str, RSTRING(v_mesg)->ptr, MAX_BUF_SZ);
   if (!NIL_P(v_mname)) snprintf(mname, MAX_BUF_SZ - strlen(str), " in \"%s\"", RSTRING(v_mname)->ptr);
   strncat(str, mname, MAX_BUF_SZ);
   if (!NIL_P(v_fname)) snprintf(fname, MAX_BUF_SZ - strlen(str), " in file \"%s\"", RSTRING(v_fname)->ptr);
   strncat(str, fname, MAX_BUF_SZ);
   if (!NIL_P(v_lineno)) snprintf(lineno, MAX_BUF_SZ - strlen(str), " at line %d", FIX2INT(v_lineno));
   strncat(str, lineno, MAX_BUF_SZ);
   if (RTEST(v_errstr) && RTEST(v_errptr))
   {
      strncat(str, "\n", MAX_BUF_SZ);
      strncat(str, RSTRING(v_errstr)->ptr, MAX_BUF_SZ);
      strncat(str, "\n", MAX_BUF_SZ);
      strncat(str, RSTRING(v_errptr)->ptr, MAX_BUF_SZ);
   }
   if (!NIL_P(v_oramesg)) snprintf(oramesg, MAX_BUF_SZ - strlen(str), "\n%s", RSTRING(v_oramesg)->ptr);
   strncat(str, oramesg, MAX_BUF_SZ);
   return rb_str_new2(str);
}

void Init_Error()
{
   rb_define_singleton_method(eError, "exception", error_exception, -1);
   rb_define_singleton_method(eError, "new", error_exception, -1);
   rb_define_method(eError, "initialize", error_initialize, -1);
   rb_enable_super(eError, "initialize");
   rb_define_method(eError, "message", error_to_s, 0);
   rb_define_method(eError, "to_s", error_to_s, 0);

   rb_global_variable(&ERR_LOC_PTR);
   rb_global_variable(&ERR_LOC_PTR_ESC);
   ERR_LOC_PTR = rb_str_new2("{{^}}");
   ERR_LOC_PTR_ESC = rb_funcall(rb_cRegexp, rb_intern("escape"), 1, ERR_LOC_PTR);
}
