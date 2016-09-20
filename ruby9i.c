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

/* global OCI handles */
OCIEnv* env_h = NULL;
OCIError* err_h = NULL;

/* global class objects */
VALUE cHandle;
VALUE cDatabase;
VALUE cDatatype;
VALUE cRowid;
VALUE cVarchar;
VALUE cNumber;
VALUE cDate;
VALUE cBaseTimestamp;
VALUE cTimestamp;
VALUE cTimestampTZ;
VALUE cTimestampLocalTZ;
VALUE cBaseInterval;
VALUE cIntervalYearToMonth;
VALUE cIntervalDayToSecond;
VALUE cStatement;

/* global exception objects */
VALUE eError;

void Init_Error(void);
void Init_Handle(void);
void Init_Database(void);
void Init_Datatype(void);
void Init_Rowid(void);
void Init_Varchar(void);
void Init_Number(void);
void Init_Date(void);
void Init_BaseTimestamp(void);
void Init_Timestamp(void);
void Init_TimestampTZ(void);
void Init_TimestampLocalTZ(void);
void Init_BaseInterval(void);
void Init_IntervalYearToMonth(void);
void Init_IntervalDayToSecond(void);
void Init_Statement(void);

void Init_ruby9i()
{
   /* set module context */
   rb_require("dbi");
   VALUE mOracle9 = rb_define_module("Ruby9i");

   /* notify the gc to protect these global Ruby objects */
   rb_global_variable(&mOracle9);
   rb_global_variable(&cHandle);
   rb_global_variable(&cDatabase);
   rb_global_variable(&cDatatype);
   rb_global_variable(&cRowid);
   rb_global_variable(&cVarchar);
   rb_global_variable(&cNumber);
   rb_global_variable(&cDate);
   rb_global_variable(&cBaseTimestamp);
   rb_global_variable(&cTimestamp);
   rb_global_variable(&cTimestampTZ);
   rb_global_variable(&cTimestampLocalTZ);
   rb_global_variable(&cBaseInterval);
   rb_global_variable(&cIntervalYearToMonth);
   rb_global_variable(&cIntervalDayToSecond);
   rb_global_variable(&cStatement);
   rb_global_variable(&eError);

   /* initialize the OCI environment */
   if (env_h == NULL)
   {
      if (OCIEnvCreate(&env_h, OCI_OBJECT, 0, 0, 0, 0, 0, 0))
         error_raise("Could not initialize OCI environment", "Init_liboci9", __FILE__, __LINE__);
      if (OCIHandleAlloc(env_h, (dvoid**) &err_h, OCI_HTYPE_ERROR, 0, 0))
         error_raise("Could not allocate error handle", "Init_liboci9", __FILE__, __LINE__);
   }

   eError = rb_define_class_under(mOracle9, "Error", rb_eRuntimeError);
   cHandle = rb_define_class_under(mOracle9, "Handle", rb_cObject);
   cDatabase = rb_define_class_under(mOracle9, "Database", rb_cObject);
   cStatement = rb_define_class_under(mOracle9, "Statement", rb_cObject);
   cDatatype = rb_define_class_under(mOracle9, "Datatype", rb_cObject);
   cRowid = rb_define_class_under(mOracle9, "Rowid", cDatatype);
   cVarchar = rb_define_class_under(mOracle9, "Varchar", cDatatype);
   cNumber = rb_define_class_under(mOracle9, "Number", cDatatype);
   cDate = rb_define_class_under(mOracle9, "Date", cDatatype);
   cBaseTimestamp = rb_define_class_under(mOracle9, "BaseTimestamp", cDatatype);
   cTimestamp = rb_define_class_under(mOracle9, "Timestamp", cBaseTimestamp);
   cTimestampTZ = rb_define_class_under(mOracle9, "TimestampTZ", cBaseTimestamp);
   cTimestampLocalTZ = rb_define_class_under(mOracle9, "TimestampLocalTZ", cBaseTimestamp);
   cBaseInterval = rb_define_class_under(mOracle9, "BaseInterval", cDatatype);
   cIntervalYearToMonth = rb_define_class_under(mOracle9, "IntervalYearToMonth", cBaseInterval);
   cIntervalDayToSecond = rb_define_class_under(mOracle9, "IntervalDayToSecond", cBaseInterval);

   /* define individual classes */
   Init_Error();
   Init_Handle();
   Init_Database();
   Init_Statement();
   Init_Datatype();
   Init_Rowid();
   Init_Varchar();
   Init_Number();
   Init_Date();
   Init_BaseTimestamp();
   Init_Timestamp();
   Init_TimestampTZ();
   Init_TimestampLocalTZ();
   Init_BaseInterval();
   Init_IntervalYearToMonth();
   Init_IntervalDayToSecond();
}
