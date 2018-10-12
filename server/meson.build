subdir('upnp')

korva_core_lib = static_library(
    'korva-core',
    [
        'korva-device.c',
        'korva-device-lister.c',
        'korva-error.c',
        'korva-icon-cache.c'
    ],
    dependencies : [config, gio, gupnp, gssdp],
    c_args : '-DICON_PATH="@0@"'.format(join_paths(get_option('prefix'), get_option('datadir'), 'korva/icons'))
)

korva_core = declare_dependency(link_with : korva_core_lib, include_directories : include_directories('.'))

executable(
    'korva-server',
    [
        'korva-server.c',
        'korva.c'
    ],
    dependencies : [config, korva_dbus, korva_upnp_backend, glib, gobject],
    link_with : korva_core_lib,
    install: true,
    install_dir : join_paths(get_option('libexecdir'), 'korva')
)

