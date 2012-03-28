# DBus service file
dbusservicedir = $(datadir)/dbus-1/services
dbusservice_in_files = org.jensge.Korva.service.in
dbusservice_DATA = $(dbusservice_in_files:.service.in=.service)

# Rule to make the service file with bindir expanded
$(dbusservice_DATA): $(dbusservice_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@
