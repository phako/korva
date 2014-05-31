AC_PREREQ([2.64])
AC_INIT([korva],
        [0.0.1],
        [korva])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([server/korva.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 tar-ustar no-dist-gzip dist-xz])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_LN_S
AM_PROG_CC_C_O

dnl Set extended warning flags for compilation. See m4/korva.m4 for the flags
KORVA_WARNING_FLAGS

PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.30 gio-unix-2.0 >= 2.30])
PKG_CHECK_MODULES([GUPNP], [gssdp-1.0 gupnp-1.0 >= 0.18.0 gupnp-av-1.0 >= 0.10.1 libsoup-2.4])

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

AC_CONFIG_FILES([
Makefile
dbus/Makefile
server/Makefile
server/upnp/Makefile
client/Makefile
data/Makefile
tests/Makefile
doc/man/Makefile
])
AC_OUTPUT
