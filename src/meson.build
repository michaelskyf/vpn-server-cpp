boost_dep = dependency('boost', modules: [ 'thread', 'coroutine' ])

source_files = files()

subdir('stream')
subdir('packet')
subdir('mq')
subdir('database')
subdir('connection')

libvpn = static_library(
    'vpn',
	source_files,
	dependencies: [ boost_dep ]
)

e = executable(
    'vpn',
	'main.cpp',
	link_with: libvpn,
    install: true
)