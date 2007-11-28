MAINTAINERCLEANFILES = Makefile.in *~

EXTRA_DIST = mysqlgistools.spec.in

noinst_DATA = mysqlgistools.spec

mysqlgistools.spec: mysqlgistools.spec.in

SUFFIXES = .in

.in:
	@RM@ -f $@ $@-t
	@SED@ \
	  -e 's!@''VERSION''@!@VERSION@!' \
	$< > $@-t
	@MV@ $@-t $@
