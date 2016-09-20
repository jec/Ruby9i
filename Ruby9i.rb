#!/usr/bin/env ruby

# Ruby9i -- a Ruby library for accessing Oracle9i through the OCI
# Copyright (C) 2003-2004 James Edwin Cain <ruby9i@jimcain.us>
# 
# This file is part of the Ruby9i library.  This Library is free software;
# you can redistribute it and/or modify it under the terms of the license
# contained in the file LICENCE.txt. If you did not receive a copy of the
# license, please contact the copyright holder.

require 'dbi'
require 'ruby9i'

module DBI
module DBD
module Ruby9i

VERSION = '0.2.1'
USED_DBD_VERSION = '0.2'

class Driver < DBI::BaseDriver

   def initialize
      super(USED_DBD_VERSION)
   end

   def connect(dbname, user, pass, attr)
      return Database.new(dbname, user, pass)
   end

end

class Database < DBI::BaseDatabase

   def initialize(dbname, user, pass)
      @db = ::Ruby9i::Database.new(dbname, user, pass)
   end

   def rollback
      @db.rollback
   end

   def commit
      @db.commit
   end

   def prepare(sql)
      return Statement.new(@db.ses_h, @db.svc_h, sql, true)
   end

   def execute(sql, *bindvars)
      stmt = Statement.new(@db.ses_h, @db.svc_h, sql, false)
      if bindvars.size > 0
         stmt.bind_params(bindvars)
      end
      stmt.execute
      return stmt
   end

   def do(sql, *bindvars)
      stmt = Statement.new(@db.ses_h, @db.svc_h, sql, false)
      if bindvars
         stmt.bind_params(bindvars)
      end
      stmt.execute
      rows = stmt.rows
      stmt.finish
      return rows
   end

   def ping
      @db.ping
   end

   def connected?(sql)
      @db.connected?(sql)
   end

   def disconnect
      @db.disconnect
   end

   def __blocking
      @db.blocking
   end

   def __nonblocking
      @db.nonblocking
   end

   def __nonblocking?
      @db.nonblocking?
   end

end

class Statement < DBI::BaseStatement

   def initialize(ses_h, svc_h, sql, yieldflag)
      @stmt = ::Ruby9i::Statement.prepare(ses_h, svc_h, sql, yieldflag)
   end

   def bind_param(param, value, attrs)
      @stmt.bind_param(param, value)
      return self # don't return the Ruby9i::Statement
   end

   def bind_params(*bindvars)
      @stmt.bind_params(bindvars)
      return self # don't return the Ruby9i::Statement
   end

   def execute
      @stmt.execute
   end

   def rows
      @stmt.rows
   end

   def fetch
      @stmt.fetch
   end

   def finish
      @stmt.finish
      return self # don't return the Ruby9i::Statement
   end

   def cancel
      @stmt.cancel
      return self # don't return the Ruby9i::Statement
   end

   def column_info
      @stmt.column_info
   end

   def __executing?
      @stmt.executing?
   end

   def __text
      @stmt.text
   end

   def __ocitype
      @stmt.ocitype
   end

   def __sqlfcode
      @stmt.sqlfcode
   end

end

end
end
end
