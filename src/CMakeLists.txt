cmake_minimum_required(VERSION 2.6)

project(vespid)

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)

set(srcs
	camera.cc
        gpio.cc
	gui.cc
	image.cc
	main.cc
	util.cc)

add_executable(${PROJECT_NAME} ${srcs})

find_package(raspicam REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Torch REQUIRED)
find_package(pigpio REQUIRED)
find_package(LuaJIT REQUIRED)

INCLUDE(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_ttf REQUIRED SDL2_ttf)

target_link_libraries(${PROJECT_NAME}
        ${raspicam_CV_LIBS}
	${SDL2_LIBRARIES}
	${SDL2_ttf_LIBRARIES}
	${pigpiod_if2_LIBRARY}
	${LUA_LIBRARY})

include_directories(
	${raspicam_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIRS}
	${SDL2_ttf_INCLUDE_DIRS}
	${pigpio_INCLUDE_DIRS}
	${LUA_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")

install(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
