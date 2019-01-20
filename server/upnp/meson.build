korva_upnp_backend_lib = static_library(
    'korva-upnp-backend',
    [
        'korva-upnp-device.c',
        'korva-upnp-device-lister.c',
        'korva-upnp-file-post.c',
        'korva-upnp-file-server.c',
        'korva-upnp-host-data.c',
        'korva-upnp-metadata-query.c'
    ],
    include_directories : include_directories('..'),
    dependencies : [config, gio, soup, gupnp, gssdp, gupnp_av],
)

korva_upnp_backend = declare_dependency(include_directories : include_directories('.'), link_with : korva_upnp_backend_lib)

