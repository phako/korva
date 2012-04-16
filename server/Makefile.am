SUBDIRS=upnp

libexec_PROGRAMS=korva

korva_SOURCES=\
	korva.c \
	korva-server.c \
	korva-server.h \
	korva-device.c \
	korva-device.h \
	korva-device-lister.c \
	korva-device-lister.h \
	korva-error.h \
	korva-error.c \
	korva-icon-cache.c \
	korva-icon-cache.h

korva_CFLAGS=$(GIO_CFLAGS) -I $(srcdir)/../dbus -DGIO_COMPILATION
korva_LDFLAGS=-L$(builddir)/upnp/ -L$(builddir)/../dbus
korva_LDADD=$(GIO_LIBS) \
	$(GUPNP_LIBS) \
	$(builddir)/upnp/libkorva-upnp-backend.a \
	$(top_builddir)/libgupnp-av/libgupnp-av-1.0.a \
	$(top_builddir)/dbus/libkorva-dbus.a \
	$(GUPNP_LIBS) \
	$(TRACKER_LIBS)
