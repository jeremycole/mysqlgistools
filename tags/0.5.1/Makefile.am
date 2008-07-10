SUBDIRS = dbug . mysqlgisimport 

MAINTAINERCLEANFILES = \
  aclocal.m4 configure config.status config.log config.h \
  config.h.in Makefile.in *~

lib_LTLIBRARIES = libmysqlgistools.la

libmysqlgistools_la_LDFLAGS = -version-info 0:0:0

libmysqlgistools_la_SOURCES = \
  mysqlgistools.c 

INCLUDES = -I$(srcdir) -I$(srcdir)/..

AM_CFLAGS = @CFLAGS@
