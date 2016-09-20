#
# Ruby9i
#

require 'mkmf'

if ENV["ORACLE_HOME"] || ENV["ORA_HOME"]
   ORACLE_HOME = ENV["ORACLE_HOME"] || ENV["ORA_HOME"]
   $CFLAGS="-I#{ORACLE_HOME}/rdbms/demo  -I#{ORACLE_HOME}/rdbms/public"
   $LDFLAGS="-L#{ORACLE_HOME}/lib"
else
   STDERR.puts <<EOF
Your $ORACLE_HOME environment variable is not set.  You may either set it and
start over or manually edit the Makefile to point to the appropriate
directories.
EOF
end

dir_config("ruby9i")
have_header("oci.h")
have_library("clntsh")
create_makefile("ruby9i")
