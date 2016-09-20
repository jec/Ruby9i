LIBNAME = ruby9i
CPPFLAGS = -I. -I/usr/lib/ruby/1.6/i586-linux-gnu
CFLAGS   = -fPIC -O2  -pipe -march=i586 -mcpu=pentiumpro  -I/opt/oracle/product/9.2.0.1.0/rdbms/demo -I/opt/oracle/product/9.2.0.1.0/rdbms/public -Wall
LIBS = $(LIBRUBY_A) -lc -L/opt/oracle/product/9.2.0.1.0/lib/ -L/opt/oracle/product/9.2.0.1.0/rdbms/lib/  -lclntsh -ldl -lm -lpthread -lnsl -ldl -lm -ldl -lcrypt -lm

OBJS = error.o \
	 handle.o \
	 database.o \
	 statement.o \
	 datatype.o \
	 rowid.o \
	 varchar.o \
	 number.o \
	 date.o \
	 basetimestamp.o \
	 timestamp.o \
	 timestamptz.o \
	 timestampltz.o \
	 baseinterval.o \
	 intervalym.o \
	 intervalds.o \
	 ruby9i.o

all: $(LIBNAME).so

clean:
	rm -f *.o *.so

.SUFFIXES: .c .o

.c.o:
	gcc $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(LIBNAME).so: $(OBJS)
	gcc -shared -o $(LIBNAME).so $(OBJS) $(LIBS)

error.o:		error.c ruby9i.h
handle.o:		handle.c ruby9i.h
database.o:		database.c ruby9i.h
statement.o:		statement.c ruby9i.h
datatype.o:		datatype.c ruby9i.h
rowid.o:		rowid.c ruby9i.h
varchar.o:		varchar.c ruby9i.h
number.o:		number.c ruby9i.h
date.o:			date.c ruby9i.h
basetimestamp.o:	basetimestamp.c ruby9i.h
timestamp.o:		timestamp.c ruby9i.h
timestamptz.o:		timestamptz.c ruby9i.h
timestampltz.o:		timestampltz.c ruby9i.h
baseinterval.o:		baseinterval.c ruby9i.h
intervalym.o:		intervalym.c ruby9i.h
intervalds.o:		intervalds.c ruby9i.h
ruby9i.o:		ruby9i.c ruby9i.h
