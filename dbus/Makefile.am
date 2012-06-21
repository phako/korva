noinst_LIBRARIES=libkorva-dbus.a
libkorva_dbus_a_SOURCES = gdbusinterface.h \
						  gdbusinterface.c \
						  gdbusobject.c \
						  gdbusobject.h \
						  gdbusinterfaceskeleton.c \
						  gdbusinterfaceskeleton.h \
						  gdbusobjectskeleton.c \
						  gdbusobjectskeleton.h \
						  korva-dbus-interface.c \
						  korva-dbus-interface.h \
						  $(BUILT_SOURCES)

libkorva_dbus_a_CFLAGS = $(GIO_CFLAGS) -I $(srcdir)/.. -w $(COVERAGE_CFLAGS)

BUILT_SOURCES = korva-marshal.c \
	korva-marshal.h

GENMARSHAL_ARGS=--prefix=korva_marshal $(srcdir)/korva-marshal.list --header
korva-marshal.c: korva-marshal.list
	$(AM_V_GEN) \
	glib-genmarshal $(GENMARSHAL_ARGS) --body >$@

korva-marshal.h: korva-marshal.list
	$(AM_V_GEN) \
	glib-genmarshal $(GENMARSHAL_ARGS) >$@

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = korva-marshal.list
