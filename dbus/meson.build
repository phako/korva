korva_dbus_sorces = gnome.gdbus_codegen('korva-dbus-interface',
                    sources : join_paths(meson.source_root(), 'data/Korva.xml'),
                    interface_prefix : 'org.jensge.Korva.',
                    namespace : 'Korva',
                )

korva_dbus_lib = static_library('korva-dbus',
    korva_dbus_sorces,
    dependencies : [config, gio, gio_unix],
)

korva_dbus = declare_dependency(include_directories : include_directories('.'), link_with : korva_dbus_lib)
