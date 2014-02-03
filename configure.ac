AC_PREREQ([2.63])
AC_INIT([PayloadEvaluators],[0.0.1])
AC_CONFIG_SRCDIR([SwiftXmlPayload/SwiftXmlPayload.cpp])
AC_CONFIG_MACRO_DIR([../fintp_utils/m4])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

AM_INIT_AUTOMAKE

AC_CHECK_HEADERS([malloc.h stdlib.h string.h sys/param.h sys/timeb.h unistd.h], [], [Cannot build])

AC_CHECK_FUNCS([ftime gethostbyaddr gethostbyname gethostname memset strerror strtol strtoul uname], [], [Cannot build])

ENVIRONMENT

AX_LIB_XERCES([2.8.0])

if test $HAVE_XERCES != yes; then
	AC_MSG_ERROR([Cannot build without Xerces])
fi

AX_LIB_XALAN([1.10.0])

if test $HAVE_XALAN != yes; then
	AC_MSG_ERROR([Cannot build without Xalan])
fi

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
