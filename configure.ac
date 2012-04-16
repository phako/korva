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
AM_PROG_CC_C_O
AC_PROG_CXX

LT_PREREQ([2.2])
LT_INIT([dlopen disable-static])

CFLAGS="${CFLAGS} -Wall -Wextra -Wno-unused-parameter"
CFLAGS="${CFLAGS} -Wno-missing-field-initializers -Wmissing-declarations"
CFLAGS="${CFLAGS} -Wshadow -Wredundant-decls -Wmissing-noreturn"
CFLAGS="${CFLAGS} -Wpointer-arith -Wwrite-strings -Winline -Wformat-nonliteral"
CFLAGS="${CFLAGS} -Wcast-align -Wformat-security -Wswitch-enum"
CFLAGS="${CFLAGS} -Wswitch-default"

PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.28 libffi])
PKG_CHECK_MODULES([GUPNP], [gupnp-1.0 >= 0.15.0])
PKG_CHECK_MODULES([SHAREUI], [share-ui-plugin mdatauri QtCore QtGui meegotouch])
PKG_CHECK_MODULES([TRACKER], [tracker-sparql-0.10])

AC_ARG_ENABLE([debug],
              AS_HELP_STRING([--enable-debug],[Enable debug build]),[],
              [enable_debug=no])
if test "x$enable_debug" = "xyes"; then
    CFLAGS="${CFLAGS} -O0 -g -Werror"
else
    CFLAGS="${CFLAGS} -O2"
fi

AC_CONFIG_FILES([
Makefile
dbus/Makefile
server/Makefile
server/upnp/Makefile
client/Makefile
data/Makefile
libgupnp-av/Makefile
provider/Makefile
])
AC_OUTPUT
