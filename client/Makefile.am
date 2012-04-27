bin_PROGRAMS = korva-control

korva_control_SOURCES = korva-control.c
korva_control_CFLAGS = $(GIO_CFLAGS) -I $(srcdir)/../dbus -DGIO_COMPILATION $(COVERAGE_CFLAGS)
korva_control_LDFLAGS = -L$(builddir)/../dbus
korva_control_LDADD = -lkorva-dbus $(GIO_LIBS) $(COVERAGE_LIBS)
