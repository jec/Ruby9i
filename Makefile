
SHELL = /bin/sh

#### Start of system configuration section. ####

srcdir = .
topdir = $(rubylibdir)/$(arch)
hdrdir = $(rubylibdir)/$(arch)
VPATH = $(srcdir)
prefix = $(DESTDIR)/usr/local
exec_prefix = $(prefix)
sitedir = $(prefix)/lib/ruby/site_ruby
rubylibdir = $(libdir)/ruby/$(ruby_version)
builddir = $(ac_builddir)
archdir = $(rubylibdir)/$(arch)
sbindir = $(exec_prefix)/sbin
compile_dir = $(DESTDIR)/home/jec/opt/ruby-1.8.0-pre3
datadir = $(prefix)/share
includedir = $(prefix)/include
infodir = $(prefix)/info
top_builddir = $(ac_top_builddir)
sysconfdir = $(prefix)/etc
mandir = $(prefix)/man
libdir = $(exec_prefix)/lib
sharedstatedir = $(prefix)/com
oldincludedir = $(DESTDIR)/usr/include
sitearchdir = $(sitelibdir)/$(sitearch)
bindir = $(exec_prefix)/bin
localstatedir = $(prefix)/var
sitelibdir = $(sitedir)/$(ruby_version)
libexecdir = $(exec_prefix)/libexec

CC = gcc
LIBRUBY = $(LIBRUBY_A)
LIBRUBY_A = lib$(RUBY_SO_NAME)-static.a
LIBRUBYARG_SHARED = 
LIBRUBYARG_STATIC = -l$(RUBY_SO_NAME)-static

CFLAGS   = -fPIC -I/opt/oracle/product/9.2.0.1.0/rdbms/demo  -I/opt/oracle/product/9.2.0.1.0/rdbms/public
CPPFLAGS = -I. -I$(topdir) -I$(hdrdir) -I$(srcdir) -DHAVE_OCI_H 
CXXFLAGS = $(CFLAGS) 
DLDFLAGS = -L/opt/oracle/product/9.2.0.1.0/lib  
LDSHARED = gcc -shared
AR = ar
EXEEXT = 

RUBY_INSTALL_NAME = ruby
RUBY_SO_NAME = $(RUBY_INSTALL_NAME)
arch = i686-linux
sitearch = i686-linux
ruby_version = 1.8
RUBY = ruby
RM = $(RUBY) -rftools -e "File::rm_f(*ARGV.map do|x|Dir[x]end.flatten.uniq)"
MAKEDIRS = $(RUBY) -r ftools -e 'File::makedirs(*ARGV)'
INSTALL_PROG = $(RUBY) -r ftools -e 'File::install(ARGV[0], ARGV[1], 0755, true)'
INSTALL_DATA = $(RUBY) -r ftools -e 'File::install(ARGV[0], ARGV[1], 0644, true)'

#### End of system configuration section. ####


LIBPATH =  -L"$(libdir)"
DEFFILE = 

CLEANFILES = 
DISTCLEANFILES = 

target_prefix = 
LOCAL_LIBS = 
LIBS =  -lclntsh  -ldl -lcrypt -lm  -lc
OBJS = date.o intervalym.o handle.o baseinterval.o varchar.o statement.o basetimestamp.o number.o datatype.o database.o intervalds.o timestamp.o error.o timestamptz.o rowid.o ruby9i.o registry.o timestampltz.o
TARGET = ruby9i
DLLIB = $(TARGET).so

RUBYCOMMONDIR = $(sitedir)$(target_prefix)
RUBYLIBDIR    = $(sitelibdir)$(target_prefix)
RUBYARCHDIR   = $(sitearchdir)$(target_prefix)

CLEANLIBS     = "$(TARGET).{lib,exp,il?,tds,map}" $(DLLIB)
CLEANOBJS     = "*.{o,a,s[ol],pdb,bak}"

all:		$(DLLIB)

clean:
		@$(RM) $(CLEANLIBS) $(CLEANOBJS) $(CLEANFILES)

distclean:	clean
		@$(RM) Makefile extconf.h conftest.* mkmf.log
		@$(RM) core ruby$(EXEEXT) *~ $(DISTCLEANFILES)

realclean:	distclean
install: $(RUBYARCHDIR)
install: $(RUBYARCHDIR)/$(DLLIB)
$(RUBYARCHDIR)/$(DLLIB): $(DLLIB) $(RUBYARCHDIR)
	@$(INSTALL_PROG) $(DLLIB) $(RUBYARCHDIR)
$(RUBYARCHDIR):
	@$(MAKEDIRS) $(RUBYARCHDIR)

site-install: install

.SUFFIXES: .c .cc .m .cxx .cpp .C .o

.cc.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.cpp.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.cxx.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.C.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $<

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

$(DLLIB): $(OBJS)
	@-$(RM) $@
	$(LDSHARED) $(DLDFLAGS) $(LIBPATH) -o $(DLLIB) $(OBJS) $(LOCAL_LIBS) $(LIBS)

