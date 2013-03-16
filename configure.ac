#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([cwatch], [1.0 experimental], [https://github.com/joebew42/cwatch/issues])
AM_INIT_AUTOMAKE(foreign)

AC_CONFIG_SRCDIR([src/list.c])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h strings.h sys/param.h syslog.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([realpath])

AC_CONFIG_FILES([
Makefile
src/Makefile
])    

AC_OUTPUT
