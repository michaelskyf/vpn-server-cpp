test_sources = [
    'mq.cpp',
]

e = executable('vpn', test_sources,
		dependencies : [gtest_main_dep, boost_dep],
		include_directories : [inc],
		link_with : [libvpn])

test('vpn', e)