## Process this file with automake to produce Makefile.in

TESTS = check_list
check_PROGRAMS = check_list
check_list_SOURCES = check_list.c $(top_builddir)/src/list.h
check_list_CFLAGS = @CHECK_CFLAGS@
check_list_LDADD = $(top_builddir)/src/list.o @CHECK_LIBS@
