MAINTAINERCLEANFILES = Makefile.in *~

bin_PROGRAMS = \
	mysqlgisimport

mysqlgisimport_SOURCES = mysqlgisimport.c
mysqlgisimport_LDADD = ../libmysqlgistools.la ../dbug/libdbug.la

INCLUDES = -I$(srcdir) -I$(srcdir)/..

all: $(OBJS)
