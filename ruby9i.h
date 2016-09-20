/*
 * Ruby9i -- a Ruby library for accessing Oracle9i through the OCI
 * Copyright (C) 2003 James Edwin Cain <info@jimcain.us>
 * 
 * This file is part of the Ruby9i library.  This Library is free software;
 * you can redistribute it and/or modify it under the terms of the license
 * contained in the file LICENCE.txt. If you did not receive a copy of the
 * license, please contact the copyright holder.
 */

#include "ruby.h"
#include "oci.h"

#define ORACLE9_MAX_NUM_LEN 200

/* holds a pointer to an OCI handle */
typedef struct
{
   dvoid *ptr;
} oci9_handle;

/* buffer for raw column output; class Datatype wraps this struct */
typedef struct
{
   dvoid *val; /* value */
   dvoid *desc; /* descriptor */
   ub4 dtype;  /* descriptor type */
   sb4 len;    /* buffer length */
   sb2 ind;    /* null indicator */
   ub2 sqlt;   /* OCI external type */
} oci9_define_buf;

OCIEnv* env_h;
OCIError* err_h;

extern VALUE mOracle9;

extern VALUE cHandle;
extern VALUE cDatabase;
extern VALUE cDatatype;
extern VALUE cRowid;
extern VALUE cVarchar;
extern VALUE cNumber;
extern VALUE cDate;
extern VALUE cBaseTimestamp;
extern VALUE cTimestamp;
extern VALUE cTimestampTZ;
extern VALUE cTimestampLocalTZ;
extern VALUE cBaseInterval;
extern VALUE cIntervalYearToMonth;
extern VALUE cIntervalDayToSecond;
extern VALUE cStatement;

extern VALUE eError;

VALUE handle_new(VALUE);
VALUE stmt_prepare(VALUE, VALUE, VALUE, VALUE, VALUE);
VALUE stmt_bind_params(VALUE, VALUE);
VALUE stmt_execute(VALUE);
VALUE stmt_fetch(VALUE);
VALUE stmt_rows(VALUE);
VALUE stmt_finish(VALUE);
dvoid *datatype_get_session_handle(VALUE);
VALUE datatype_new(int, VALUE *, VALUE);
VALUE error_exception(int, VALUE *, VALUE);
void error_raise(char *, char *, char *, int);
void error_raise_sql(char *, char *, char *, int, char *, int);
