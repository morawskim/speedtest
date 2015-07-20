cmake_minimum_required(VERSION 3.1)
project(SpeedTestClient)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")


#set(CURL_HEADERS /usr/include/curl)
#include_directories(${CURL_HEADERS})

find_package(PkgConfig)
pkg_check_modules(XML2 REQUIRED libxml-2.0)

include_directories(${XML2_INCLUDE_DIRS})

set(CMAKE_INSTALL_PREFIX "/home/marcin/Projekty/CSpeedTestClion/build/")

add_subdirectory(libspeedtest)
set(SOURCE_FILES main.c)
add_executable(speed-test-client ${SOURCE_FILES} cli.h cli.c)
target_link_libraries(speed-test-client speedtest)
