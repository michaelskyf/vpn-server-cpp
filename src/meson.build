boost_dep = dependency('boost', modules: [ 'thread', 'coroutine' ])

source_files = files(
	'mq.cpp',
	'database/database.cpp',
)

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