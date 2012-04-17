# DBus service file
dbusservicedir = /usr/share/dbus-1/services
dbusservice_in_files = pushup.service.in
dbusservice_DATA = $(dbusservice_in_files:.service.in=.service)

icondir = /usr/share/themes/blanco/meegotouch/icons
icon_DATA = icon-l-pushup_play.png

# Rule to make the service file with bindir expanded
$(dbusservice_DATA): $(dbusservice_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST=icon.svg
