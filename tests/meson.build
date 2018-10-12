libxml = dependency('libxml-2.0')

upnp_test = executable(
    'test-upnp',
    [
        'mock-dmr/mock-dmr.c',
        'test-upnp.c'
    ],
    c_args : '-DTEST_DATA_DIR="@0@"'.format(meson.current_source_dir()),
    dependencies : [korva_upnp_backend, korva_core, libxml, gobject, gupnp, gssdp, soup])

test('upnp-test', upnp_test)
