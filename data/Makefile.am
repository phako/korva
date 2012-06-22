# DBus service file
dbusservicedir = /usr/share/dbus-1/services
dbusservice_in_files = pushup.service.in
dbusservice_DATA = $(dbusservice_in_files:.service.in=.service)


icondir = $(datadir)/korva/icons
icon_DATA = $(srcdir)/icon-l-pushup_play.png \
	$(srcdir)/icons/network-server.png \
	$(srcdir)/icons/video-display.png

themedir = /usr/share/themes/blanco/meegotouch/icons
theme_DATA = $(srcdir)/icon-m-content-pushup_play.png
	
# Rule to make the service file with bindir expanded
$(dbusservice_DATA): $(dbusservice_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = icon.svg $(dbusservice_in_files)
CLEANFILES = $(dbusservice_DATA)
