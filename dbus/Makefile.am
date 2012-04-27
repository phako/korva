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
						  korva-dbus-interface.h
libkorva_dbus_a_CFLAGS = $(GIO_CFLAGS) -I $(srcdir)/.. -DGIO_COMPILATION -w $(COVERAGE_CFLAGS)
