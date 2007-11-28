MAINTAINERCLEANFILES = Makefile.in *~

noinst_LTLIBRARIES = libdbug.la

libdbug_la_SOURCES = dbug.c dbug.h dbug_long.h ../config.h
libdbug_la_LIBADD = @LTLIBOBJS@

INCLUDES = -I$(srcdir) -I..

all: $(OBJS)
