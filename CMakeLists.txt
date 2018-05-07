cmake_minimum_required(VERSION 2.6)
project(vespid)

if (NOT UNIX)
	message(FATAL_ERROR "VESPID only works on UNIX-like operating systems.")
endif()

set(CMAKE_CXX_STANDARD 11)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")

set(SHAREDIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}")
set(BINDIR "bin")

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fonts" DESTINATION "${SHAREDIR}")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/torchnn" DESTINATION "${SHAREDIR}")

if (INSTALL_SPLASH)
	set(SPLASH_PATH "/usr/share/plymouth/themes")
	if (NOT EXISTS ${SPLASH_PATH})
		message(FATAL_ERROR "Requested splash installation but ${SPLASH_PATH} does not exist.")
	endif()

	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/splash/vespid" DESTINATION "/usr/share/plymouth/themes")
	message(STATUS "VESPID plymouth theme will be installed to ${SPLASH_PATH}. Run `plymouth-set-default-theme vespid` after installation to change the splash screen.")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/" "/usr/local/lib/cmake/")

add_subdirectory(src)
