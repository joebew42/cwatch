## Process this file with automake to produce Makefile.in
SUBDIRS = uat

TESTS = check_list check_cwatch check_commandline
check_PROGRAMS = check_list check_cwatch check_commandline

check_list_SOURCES = check_list.c $(top_builddir)/src/list.h
check_list_CFLAGS = @CHECK_CFLAGS@
check_list_LDADD = $(top_builddir)/src/list.o @CHECK_LIBS@

check_commandline_SOURCES = check_commandline.c $(top_builddir)/src/commandline.h
check_commandline_CFLAGS = @CHECK_CFLAGS@
check_commandline_LDADD = $(top_builddir)/src/commandline.o @CHECK_LIBS@

check_cwatch_SOURCES = check_cwatch.c $(top_builddir)/src/cwatch.h
check_cwatch_CFLAGS = @CHECK_CFLAGS@
check_cwatch_LDADD =  $(top_builddir)/src/bstrlib.o $(top_builddir)/src/list.o $(top_builddir)/src/cwatch.o @CHECK_LIBS@
