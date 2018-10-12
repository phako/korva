xsltproc = find_program('xsltproc')

if xsltproc.found()
    custom_target('manpages',
        command :
        [
            xsltproc,
            '-o', '@OUTPUT@',
            '--nonet',
            '--stringparam', 'man.output.quietly', '1',
            '--stringparam', 'funcsynopsis.style', 'ansi',
            'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl',
            '@INPUT@'
        ],
        input : 'man/korva-control.xml',
        output : 'korva-control.1',
        install : true,
        install_dir : join_paths(get_option('mandir'), 'man1')
    )
endif
