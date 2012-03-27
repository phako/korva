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
AM_PROG_CC_C_O

PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.30 gio-unix-2.0 >= 2.30])
PKG_CHECK_MODULES([GUPNP], [gupnp-1.0 >= 0.18.0 gupnp-av-1.0 >= 0.10.1])
AC_CONFIG_FILES([
Makefile
dbus/Makefile
server/Makefile
server/upnp/Makefile
client/Makefile
])
AC_OUTPUT
