bin_PROGRAMS=korva

GENERATED_SOURCES=korva-dbus-interface.c korva-dbus-interface.h

korva_SOURCES=\
	korva.c \
	$(GENERATED_SOURCES)

korva_CFLAGS=$(GIO_CFLAGS)

korva_LDADD=$(GIO_LIBS)

korva-dbus-interface.c korva-dbus-interface.h: $(top_srcdir)/data/Korva.xml
	$(AM_V_GEN)$(GDBUS_CODEGEN) --generate-c-code korva-dbus-interface \
								--c-namespace Korva \
								--interface-prefix org.jensge.Korva \
								$<

EXTRA_DIST=$(GENERATED_SOURCES)
