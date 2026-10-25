# cmusfm - CMakeLists.txt
# SPDX-FileCopyrightText: 2026 Arkadiusz Bokowy and contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.22)

project(cmusfm
	VERSION 0.4.2
	DESCRIPTION "Last.fm scrobbler for cmus music player"
	LANGUAGES C)

option(BUILD_TESTING "Build the testing tree." OFF)
include(CTest)

include(CheckIncludeFile)
include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

option(ENABLE_DEBUG "Enable debugging support." OFF)
option(ENABLE_LIBNOTIFY "Enable desktop notification support." OFF)
option(ENABLE_MANPAGES "Build and install the man page." OFF)

check_include_file(poll.h HAVE_POLL_H)
check_include_file(sys/inotify.h HAVE_SYS_INOTIFY_H)

if(NOT HAVE_POLL_H)
	message(FATAL_ERROR "poll.h header not found")
endif()

pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
if(CURL_VERSION VERSION_EQUAL "7.71.0")
	message(FATAL_ERROR
		"libcurl 7.71.0 contains a curl_easy_escape() bug that prevents cmusfm "
		"from working correctly. Please upgrade or downgrade libcurl. For more "
		"details see: https://github.com/curl/curl/issues/5601")
endif()

find_package(OpenSSL QUIET COMPONENTS Crypto)
if(OpenSSL_FOUND)
	set(CMAKE_REQUIRED_INCLUDES "${OPENSSL_INCLUDE_DIR}")
	check_include_file(openssl/md5.h HAVE_OPENSSL_MD5_H)
	unset(CMAKE_REQUIRED_INCLUDES)
	if(HAVE_OPENSSL_MD5_H)
		set(WITH_SYSTEM_MD5 ON)
	endif()
endif()

set(DEBUG "${ENABLE_DEBUG}")
configure_file(
	${PROJECT_SOURCE_DIR}/config.h.in
	${PROJECT_BINARY_DIR}/config.h)

add_library(
	cmusfmcore STATIC
	src/cache.c
	src/config.c
	src/libscrobbler2.c
	src/server.c
	src/utils.c)

target_link_libraries(cmusfmcore PkgConfig::CURL)

if(ENABLE_LIBNOTIFY)
	pkg_check_modules(LibNotify REQUIRED IMPORTED_TARGET libnotify>=0.7)
	target_sources(cmusfmcore PRIVATE src/notify.c)
	target_link_libraries(cmusfmcore PkgConfig::LibNotify)
endif()

if(WITH_SYSTEM_MD5)
	target_link_libraries(cmusfmcore OpenSSL::Crypto)
endif()

add_executable(cmusfm src/main.c)
target_link_libraries(cmusfm cmusfmcore)

install(TARGETS cmusfm RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

if(BUILD_TESTING)

	set(TESTS
		test-cache
		test-server-notify
		test-server-submit01
		test-server-submit02
		test-server-submit03)

	if(ENABLE_LIBNOTIFY)
		list(APPEND TESTS test-notify)
	endif()

	foreach(TEST IN ITEMS ${TESTS})
		add_executable("${TEST}" "test/${TEST}.c")
		target_link_libraries("${TEST}" PRIVATE cmusfmcore)
		add_test(NAME "${TEST}" COMMAND "${TEST}")
	endforeach()

	find_package(Threads REQUIRED)
	target_link_libraries(test-server-submit03 PRIVATE Threads::Threads)

endif()

if(ENABLE_MANPAGES)

	find_program(RST2MAN NAMES rst2man rst2man.py REQUIRED)

	configure_file(
		"${PROJECT_SOURCE_DIR}/doc/cmusfm.1.rst"
		"${PROJECT_BINARY_DIR}/doc/cmusfm.1.rst"
		@ONLY)

	set(MANPAGE "${PROJECT_BINARY_DIR}/doc/cmusfm.1")
	add_custom_command(
		OUTPUT "${MANPAGE}"
		COMMAND "${RST2MAN}" "${PROJECT_BINARY_DIR}/doc/cmusfm.1.rst" "${MANPAGE}"
		DEPENDS "${PROJECT_BINARY_DIR}/doc/cmusfm.1.rst"
		VERBATIM)

	add_custom_target(manpage ALL DEPENDS "${MANPAGE}")
	install(FILES "${MANPAGE}" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")

endif()
