set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "michal.kostrzewski@protonmail.com")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "openssl")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "VPN Server")

install(FILES vpn-server.service DESTINATION /usr/lib/systemd/system)
install(FILES vpn.conf DESTINATION /etc/vpn)
install(TARGETS "${PROJECT_NAME}" DESTINATION bin)

include(CPack)
