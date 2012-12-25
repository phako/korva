AC_PREREQ([2.64])
AC_INIT([korva],
        [0.0.1],
        [korva])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([server/korva.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 foreign tar-ustar no-dist-gzip dist-xz])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_LN_S
AM_PROG_CC_C_O
AC_PROG_CXX

LT_PREREQ([2.2])
LT_INIT([dlopen disable-static])

dnl Set extended warning flags for compilation. See m4/korva.m4 for the flags
KORVA_WARNING_FLAGS

PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.28])
PKG_CHECK_MODULES([GUPNP], [gupnp-1.0 >= 0.15.0])
PKG_CHECK_MODULES([DEVICEMODEL], [QtCore QtDBus QtGui])
PKG_CHECK_MODULES([SHAREUI], [share-ui-plugin mdatauri meegotouch QtCore QtGui
                   QtNetwork QtDBus])
PKG_CHECK_MODULES([TRACKER], [tracker-sparql-0.10])
PKG_CHECK_MODULES([GSTREAMER], [gstreamer-0.10])
PKG_CHECK_MODULES([UI], [QtCore QtGui QtNetwork QtDeclarative qdeclarative-boostable])

AC_ARG_ENABLE([debug],
              AS_HELP_STRING([--enable-debug],[Enable debug build]),[],
              [enable_debug=no])
AS_IF([test "x$[]enable_debug" = "xyes"],
      [CFLAGS="${CFLAGS} -O0 -g"],
      [CFLAGS="${CFLAGS} -O2"])

dnl Generate coverage data
AC_ARG_ENABLE([coverage],
  AS_HELP_STRING([--enable-coverage],
                 [Generate converage reports for unit tests]),
  [],
  [enable_coverage=no])
AM_CONDITIONAL([ENABLE_COVERAGE],[test "x$enable_coverage" != "xno"])
AS_IF([test "x$enable_coverage" != "xno"],
  [
    AC_PATH_PROG([LCOV], [lcov])
    AC_PATH_PROG([GENHTML], [genhtml])
    AC_SUBST([COVERAGE_LIBS],["-lgcov"])
    AC_SUBST([COVERAGE_CFLAGS],["-fprofile-arcs -ftest-coverage"])
    AC_SUBST(COVERAGE_VALAFLAGS,["-g"])
  ]
)

AC_PATH_PROG([XSLTPROC], [xsltproc])
AM_CONDITIONAL([HAVE_XSLTPROC], [test x"$XSLTPROC" != x])

dnl Fix compilation for included GLib components
AC_DEFINE([GIO_COMPILATION],[],[Needed for compiling included GLib components])

dnl Define variant type from 2.30
AC_DEFINE([G_VARIANT_TYPE_VARDICT],[((const GVariantType *) "a{sv}")],[Port from GLib 2.30])

dnl Force include config.h
CFLAGS="${CFLAGS} -include config.h"

AC_CONFIG_FILES([
pushup/libdevicemodel/Makefile
pushup/provider/Makefile
pushup/gui/Makefile
])

AC_CONFIG_FILES([
Makefile
dbus/Makefile
server/Makefile
server/upnp/Makefile
client/Makefile
data/Makefile
libgupnp-av/Makefile
tests/Makefile
doc/man/Makefile
])
AC_OUTPUT
