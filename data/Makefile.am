# DBus service file
dbusservicedir = $(datadir)/dbus-1/services
dbusservice_in_files = org.jensge.Korva.service.in
dbusservice_DATA = $(dbusservice_in_files:.service.in=.service)

icondir = $(datadir)/korva/icons
icon_DATA = $(srcdir)/icons/network-server.png \
	$(srcdir)/icons/video-display.png

# Rule to make the service file with bindir expanded
$(dbusservice_DATA): $(dbusservice_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = $(dbusservice_in_files)
CLEANFILES = $(dbusservice_DATA)
