set(SERVER_NAME "${PROJECT_NAME}")

add_executable(
	${SERVER_NAME}
	main.cpp
    server.cpp
	)

find_package(OpenSSL REQUIRED)

target_link_libraries(
	${SERVER_NAME}
	${OPENSSL_LIBRARIES}
	)
