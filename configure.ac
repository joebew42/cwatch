#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([cwatch], [1.2 experimental], [https://github.com/joebew42/cwatch/issues])

AC_CONFIG_SRCDIR([src/bstrlib.c])

# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([build-aux])

# Compiler options
AM_INIT_AUTOMAKE(foreign)

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# This macro is defined in check.m4 and tests if check.h and
# libcheck.a are installed in your system. It sets CHECK_CFLAGS and
# CHECK_LIBS accordingly.
PKG_CHECK_MODULES([CHECK], [check >= 0.9.0])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h strings.h sys/param.h syslog.h limits.h stddef.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset realpath])

AC_CONFIG_FILES([
Makefile
src/Makefile
tests/Makefile
])

AC_CONFIG_MACRO_DIR([m4])

AC_OUTPUT
