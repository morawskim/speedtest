cmake_minimum_required(VERSION 3.1)

project(libspeedtest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")


find_package(PkgConfig)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(XML2 REQUIRED libxml-2.0)
pkg_check_modules(LIBCRYPTO REQUIRED libcrypto)

link_directories(${CURL_LIBRARY_DIRS} ${XML2_LIBRARY_DIRS})
include_directories(${CURL_INCLUDE_DIRS} ${XML2_INCLUDE_DIRS})

set(SOURCE_FILES speedtest.c util.c md5.c speedtest.h util.h md5.h)

add_library(speedtest SHARED ${SOURCE_FILES} speed_test_error.h speed_test_error.c)
target_link_libraries(speedtest ${CURL_LIBRARIES} ${XML2_LIBRARIES} ${LIBCRYPTO_LIBRARIES} pthread m)

install(TARGETS speedtest DESTINATION lib)
install(FILES speedtest.h DESTINATION include)