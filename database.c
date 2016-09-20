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

VALUE db_quote(VALUE klass, VALUE sql)
{
   return rb_funcall(sql, rb_intern("gsub"), 2, rb_str_new2("/'/"), rb_str_new2("''"));
}

VALUE db_initialize(int argc, VALUE *argv, VALUE self)
{
   /* process arguments */
   VALUE user, pass, db;
   rb_scan_args(argc, argv, "3", &db, &user, &pass);

   /* create service, server and session handles */
   oci9_handle *svc_h;
   oci9_handle *svr_h;
   oci9_handle *ses_h;
   VALUE v_svc_h = handle_new(INT2FIX(OCI_HTYPE_SVCCTX));
   VALUE v_svr_h = handle_new(INT2FIX(OCI_HTYPE_SERVER));
   VALUE v_ses_h = handle_new(INT2FIX(OCI_HTYPE_SESSION));
   Data_Get_Struct(v_svc_h, oci9_handle, svc_h);
   Data_Get_Struct(v_svr_h, oci9_handle, svr_h);
   Data_Get_Struct(v_ses_h, oci9_handle, ses_h);

   /* attach server */
   if (OCIServerAttach(svr_h->ptr, err_h, RSTRING(db)->ptr, RSTRING(db)->len, OCI_DEFAULT))
      error_raise("Could not attach to Oracle server", "db_initialize", __FILE__, __LINE__);

   /* set the server handle in the service handle */
   if (OCIAttrSet(svc_h->ptr, OCI_HTYPE_SVCCTX, svr_h->ptr, 0, OCI_ATTR_SERVER, err_h))
      error_raise("Could not set server in service context", "db_initialize", __FILE__, __LINE__);

   /* set username & password in authentication handle */
   if (OCIAttrSet(ses_h->ptr, OCI_HTYPE_SESSION, RSTRING(user)->ptr, RSTRING(user)->len, OCI_ATTR_USERNAME, err_h))
      error_raise("Could not set session username", "db_initialize", __FILE__, __LINE__);
   if (OCIAttrSet(ses_h->ptr, OCI_HTYPE_SESSION, RSTRING(pass)->ptr, RSTRING(pass)->len, OCI_ATTR_PASSWORD, err_h))
      error_raise("Could not set session password", "db_initialize", __FILE__, __LINE__);

   /* begin session */
   if (OCISessionBegin(svc_h->ptr, err_h, ses_h->ptr, OCI_CRED_RDBMS, OCI_DEFAULT))
      error_raise("Could not begin session", "db_initialize", __FILE__, __LINE__);

   /* set the authentication handle in the service context */
   if (OCIAttrSet(svc_h->ptr, OCI_HTYPE_SVCCTX, ses_h->ptr, 0, OCI_ATTR_SESSION, err_h))
      error_raise("Could not set authentication in service context", "db_initialize", __FILE__, __LINE__);

   /* set instance variables & allow read-only access to them */
   rb_iv_set(self, "@svc_h", v_svc_h);
   rb_iv_set(self, "@svr_h", v_svr_h);
   rb_iv_set(self, "@ses_h", v_ses_h);
   //rb_define_attr(CLASS_OF(self), "svc_h", 1, 0);
   //rb_define_attr(CLASS_OF(self), "svr_h", 1, 0);
   //rb_define_attr(CLASS_OF(self), "ses_h", 1, 0);

   return self;
}

VALUE db_blocking(VALUE self)
{
   oci9_handle *svr_h;
   Data_Get_Struct(rb_iv_get(self, "@svr_h"), oci9_handle, svr_h);
   ub1 nonblocking;
   if (OCIAttrGet(svr_h->ptr, OCI_HTYPE_SERVER, &nonblocking, 0, OCI_ATTR_NONBLOCKING_MODE, err_h))
      error_raise("Could not get non-blocking mode", "db_is_nonblocking", __FILE__, __LINE__);
   if (nonblocking)
      if (OCIAttrSet(svr_h->ptr, OCI_HTYPE_SERVER, 0, 0, OCI_ATTR_NONBLOCKING_MODE, err_h))
         error_raise("Could not set non-blocking mode", "db_nonblocking", __FILE__, __LINE__);
   return self;
}

VALUE db_nonblocking(VALUE self)
{
   oci9_handle *svr_h;
   Data_Get_Struct(rb_iv_get(self, "@svr_h"), oci9_handle, svr_h);
   ub1 nonblocking;
   if (OCIAttrGet(svr_h->ptr, OCI_HTYPE_SERVER, &nonblocking, 0, OCI_ATTR_NONBLOCKING_MODE, err_h))
      error_raise("Could not get non-blocking mode", "db_is_nonblocking", __FILE__, __LINE__);
   if (!nonblocking)
      if (OCIAttrSet(svr_h->ptr, OCI_HTYPE_SERVER, 0, 0, OCI_ATTR_NONBLOCKING_MODE, err_h))
         error_raise("Could not set non-blocking mode", "db_nonblocking", __FILE__, __LINE__);
   return self;
}

VALUE db_is_nonblocking(VALUE self)
{
   oci9_handle *svr_h;
   Data_Get_Struct(rb_iv_get(self, "@svr_h"), oci9_handle, svr_h);
   ub1 nonblocking;
   if (OCIAttrGet(svr_h->ptr, OCI_HTYPE_SERVER, &nonblocking, 0, OCI_ATTR_NONBLOCKING_MODE, err_h))
      error_raise("Could not get non-blocking mode", "db_is_nonblocking", __FILE__, __LINE__);
   if (nonblocking)
      return Qtrue;
   return Qfalse;
}

VALUE db_rollback(VALUE self)
{
   oci9_handle *svc_h;
   Data_Get_Struct(rb_iv_get(self, "@svc_h"), oci9_handle, svc_h);
   if (OCITransRollback(svc_h->ptr, err_h, OCI_DEFAULT))
      error_raise("Could not roll back transactions", "db_rollback", __FILE__, __LINE__);
   return self;
}

VALUE db_commit(VALUE self)
{
   oci9_handle *svc_h;
   Data_Get_Struct(rb_iv_get(self, "@svc_h"), oci9_handle, svc_h);
   if (OCITransCommit(svc_h->ptr, err_h, OCI_DEFAULT))
      error_raise("Could not commit transactions", "db_commit", __FILE__, __LINE__);
   return self;
}

VALUE db_prepare(VALUE self, VALUE sql)
{
   return stmt_prepare(cStatement, rb_iv_get(self, "@ses_h"), rb_iv_get(self, "@svc_h"), sql, Qtrue);
}

VALUE db_execute(int argc, VALUE *argv, VALUE self)
{
   VALUE sql, bindvars;
   rb_scan_args(argc, argv, "1*", &sql, &bindvars);
   VALUE stmt = stmt_prepare(cStatement, rb_iv_get(self, "@ses_h"), rb_iv_get(self, "@svc_h"), sql, Qfalse);
   if (argc > 1)
      stmt_bind_params(stmt, bindvars);
   stmt_execute(stmt);
   return stmt;
}

VALUE db_do(int argc, VALUE *argv, VALUE self)
{
   VALUE sql, bindvars;
   rb_scan_args(argc, argv, "1*", &sql, &bindvars);
   VALUE stmt = stmt_prepare(cStatement, rb_iv_get(self, "@ses_h"), rb_iv_get(self, "@svc_h"), sql, Qfalse);
   if (argc > 1)
      stmt_bind_params(stmt, bindvars);
   stmt_execute(stmt);
   VALUE rows = stmt_rows(stmt);
   stmt_finish(stmt);
   return rows;
}

VALUE db_connected(VALUE self)
{
   ub4 status;
   oci9_handle *svr_h;
   Data_Get_Struct(rb_iv_get(self, "@svr_h"), oci9_handle, svr_h);
   if (OCIAttrGet(svr_h->ptr, OCI_HTYPE_SERVER, &status, 0, OCI_ATTR_SERVER_STATUS, err_h))
      error_raise("Could not get server status", "db_connected", __FILE__, __LINE__);
   if (status == OCI_SERVER_NORMAL)
      return Qtrue;
   return Qfalse;
}

VALUE db_ping(VALUE self)
{
   VALUE stmt = stmt_execute(stmt_prepare(cStatement, rb_iv_get(self, "@ses_h"), rb_iv_get(self, "@svc_h"),
      rb_str_new2("select 1 from dual"), Qfalse));
   stmt_fetch(stmt);
   stmt_finish(stmt);
   return Qtrue;
}

VALUE db_disconnect(VALUE self)
{
   db_rollback(self);

   /* get handles */
   oci9_handle *svc_h;
   oci9_handle *svr_h;
   oci9_handle *ses_h;
   Data_Get_Struct(rb_iv_get(self, "@svc_h"), oci9_handle, svc_h);
   Data_Get_Struct(rb_iv_get(self, "@svr_h"), oci9_handle, svr_h);
   Data_Get_Struct(rb_iv_get(self, "@ses_h"), oci9_handle, ses_h);
   
   /* end session/log off & detach server */
   if (OCISessionEnd(svc_h->ptr, err_h, ses_h->ptr, OCI_DEFAULT))
      error_raise("Could not end session", "db_disconnect", __FILE__, __LINE__);
   if (OCIServerDetach(svr_h->ptr, err_h, OCI_DEFAULT))
      error_raise("Could not detach from Oracle server", "db_disconnect", __FILE__, __LINE__);

   /* free the handles that we created */
   if (svc_h->ptr)
      OCIHandleFree(svc_h->ptr, OCI_HTYPE_SVCCTX);
   if (svr_h->ptr)
      OCIHandleFree(svr_h->ptr, OCI_HTYPE_SERVER);
   if (ses_h->ptr)
      OCIHandleFree(ses_h->ptr, OCI_HTYPE_SESSION);
   svc_h->ptr = 0;
   svr_h->ptr = 0;
   ses_h->ptr = 0;

   return self;
}

void Init_Database()
{
   rb_define_singleton_method(cDatabase, "quote", db_quote, 1);
   rb_define_method(cDatabase, "initialize", db_initialize, -1);
   rb_define_method(cDatabase, "blocking", db_blocking, 0);
   rb_define_method(cDatabase, "nonblocking", db_nonblocking, 0);
   rb_define_method(cDatabase, "nonblocking?", db_is_nonblocking, 0);
   rb_define_method(cDatabase, "rollback", db_rollback, 0);
   rb_define_method(cDatabase, "commit", db_commit, 0);
   rb_define_method(cDatabase, "prepare", db_prepare, 1);
   rb_define_method(cDatabase, "execute", db_execute, -1);
   rb_define_method(cDatabase, "do", db_do, -1);
   rb_define_method(cDatabase, "ping", db_ping, 0);
   rb_define_method(cDatabase, "connected?", db_connected, 0);
   rb_define_method(cDatabase, "disconnect", db_disconnect, 0);
}
