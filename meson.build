project('korva',
        ['c'],
        version : '0.0.1')

gnome = import('gnome')

glib = dependency('glib-2.0', version : '>= 2.30')
gobject = dependency('gobject-2.0', version : '>= 2.30')
gio = dependency('gio-2.0', version : '>= 2.30')
gio_unix = dependency('gio-unix-2.0', version : '>= 2.30')
gssdp = dependency('gssdp-1.0', version : '>= 1.0')
gupnp = dependency('gupnp-1.0', version : '>= 1.0')
gupnp_av = dependency('gupnp-av-1.0', version : '>= 0.10.1')
soup = dependency('libsoup-2.4')

conf = configuration_data()
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set('libexecdir', join_paths(get_option('prefix'), get_option('libexecdir')))

config_h = configure_file(output : 'config.h', configuration: conf)
config = declare_dependency(compile_args : '-DHAVE_CONFIG_H', include_directories : include_directories('.'))

subdir('dbus')
subdir('server')
subdir('client')
subdir('data')
subdir('tests')
subdir('doc')
