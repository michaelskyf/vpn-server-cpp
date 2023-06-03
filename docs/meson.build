build_docs = get_option('docs')

if not build_docs.disabled()

sphinx = find_program('sphinx-build')
doxygen = find_program('doxygen')

in_dir = meson.current_source_dir()
out_dir = meson.current_build_dir()

Doxyfile = configure_file(
    input: 'Doxyfile.in',
    output: 'Doxyfile',
    configuration: {
        'DOXYGEN_OUTPUT_DIR': meson.current_build_dir(),
        'DOXYGEN_SOURCE_DIR': meson.project_source_root()/'src' + ' ' + meson.project_source_root()/'include',
    }
)

build_docs = custom_target('build_docs',
    input: 'meson.build',
    output: 'vpn-server-cpp',
    command: [sphinx, in_dir/'source', out_dir],
)

endif