client = executable(
    'korva-control',
    [
        'korva-control.c'
    ],
    dependencies : [config, gio, korva_dbus],
    install : true)

symlink = join_paths(meson.source_root(), 'build-aux/meson-make-symlink.sh')
bindir = join_paths(get_option('prefix'), get_option('bindir'))

foreach link : ['push', 'list', 'unshare']
    meson.add_install_script(
        symlink,
        join_paths(bindir, 'korva-control'),
        join_paths(bindir, 'korva-' + link)
    )
endforeach
