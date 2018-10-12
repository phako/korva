configure_file(input: 'org.jensge.Korva.service.in',
               output : 'org.jgensge.Korva.service',
               install: true,
               install_dir : join_paths(get_option('datadir'), 'dbus-1/services'),
               configuration: conf
           )

install_subdir('icons', install_dir : join_paths(get_option('datadir'), 'korva'))
