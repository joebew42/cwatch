## Process this file with automake to produce Makefile.in
SUBDIRS = uat

TESTS = check_queue check_cwatch check_commandline
check_PROGRAMS = check_queue check_cwatch check_commandline

check_queue_SOURCES = check_queue.c $(top_builddir)/src/queue.h
check_queue_CFLAGS = @CHECK_CFLAGS@
check_queue_LDADD = $(top_builddir)/src/queue.o @CHECK_LIBS@

check_commandline_SOURCES = check_commandline.c $(top_builddir)/src/commandline.h
check_commandline_CFLAGS = @CHECK_CFLAGS@
check_commandline_LDADD = $(top_builddir)/src/commandline.o @CHECK_LIBS@

check_cwatch_SOURCES = check_cwatch.c $(top_builddir)/src/cwatch.h
check_cwatch_CFLAGS = @CHECK_CFLAGS@
check_cwatch_LDADD =  $(top_builddir)/src/bstrlib.o $(top_builddir)/src/queue.o $(top_builddir)/src/cwatch.o @CHECK_LIBS@
