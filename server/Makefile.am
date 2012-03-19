bin_PROGRAMS=korva

GENERATED_SOURCES=korva-dbus-interface.c korva-dbus-interface.h

korva_SOURCES=\
	$(GENERATED_SOURCES) \
	korva.c \
	korva-server.c \
	korva-server.h \
	korva-device.c \
	korva-device.h \
	korva-device-lister.c \
	korva-device-lister.h

korva_CFLAGS=$(GIO_CFLAGS)

korva_LDADD=$(GIO_LIBS)

korva-dbus-interface.c korva-dbus-interface.h: $(top_srcdir)/data/Korva.xml
	$(AM_V_GEN)$(GDBUS_CODEGEN) --generate-c-code korva-dbus-interface \
								--c-namespace Korva \
								--interface-prefix org.jensge.Korva \
								$<

EXTRA_DIST=$(GENERATED_SOURCES)
