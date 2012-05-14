bin_PROGRAMS = korva-control

korva_control_SOURCES = korva-control.c
korva_control_CFLAGS = $(GIO_CFLAGS) -I $(srcdir)/../dbus $(COVERAGE_CFLAGS)
korva_control_LDFLAGS = -L $(builddir)/../dbus
korva_control_LDADD = -lkorva-dbus $(GIO_LIBS) $(COVERAGE_LIBS)

install-exec-hook:
	cd $(DESTDIR)$(bindir) && $(LN_S) -f korva-control$(EXEEXT) korva-push$(EXEEXT)
	cd $(DESTDIR)$(bindir) && $(LN_S) -f korva-control$(EXEEXT) korva-unshare$(EXEEXT)
	cd $(DESTDIR)$(bindir) && $(LN_S) -f korva-control$(EXEEXT) korva-list$(EXEEXT)
