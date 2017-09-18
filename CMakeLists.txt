cmake_minimum_required (VERSION 2.6)
project(plumber-rocks)

macro(FindPlumber outvar)
	unset(${outvar}_PREFIX CACHE)
	find_path(${outvar}_PREFIX NAMES "lib/plumber/servlet.mk" HINTS "${PLUMBER_HINT}")
	if("${${outvar}_PREFIX}" STREQUAL "${outvar}_PREFIX-NOTFOUND")
		message(FATAL_ERROR "Plumber installation not found")
	else("${${outvar}_PREFIX}" STREQUAL "${outvar}_PREFIX-NOTFOUND")
		message("Found libplumber at ${${outvar}_PREFIX}")
	endif("${${outvar}_PREFIX}" STREQUAL "${outvar}_PREFIX-NOTFOUND")
endmacro(FindPlumber outvar)

macro(FindRocksdb outvar)
	unset(${outvar}_PREFIX CACHE)
	find_path(${outvar}_PREFIX NAMES "include/rocksdb/db.h")
	if("${outvar}_PREFIX-NOTFOUND" STREQUAL "${${outvar}_PREFIX}")
		message(FATAL_ERROR "Librocksdb not found!")
	else("${outvar}_PREFIX-NOTFOUND" STREQUAL "${${outvar}_PREFIX}")
		message("Found librocksdb at ${${outvar}_PREFIX}")
	endif("${outvar}_PREFIX-NOTFOUND" STREQUAL "${${outvar}_PREFIX}")
	set(${outvar}_LDFLAGS "-L${${outvar}_PREFIX}/lib -lrocksdb")
	set(${outvar}_CFLAGS "-I${${outvar}_PREFIX}/include")
endmacro(FindRocksdb outvar)

FindPlumber(PLUMBER)
FindRocksdb(ROCKSDB)

if("" STREQUAL "${CMAKE_BUILD_TYPE}")
	set(CMAKE_BUILD_TYPE DEBUG)
endif("" STREQUAL "${CMAKE_BUILD_TYPE}")


set(CFLAGS "${CFLAGS} -Wall -Werror ${CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}}")
set(LDFLAGS "${LDFLAGS} ${CMAKE_LD_FLAGS_${CMAKE_BUILD_TYPE}}")


file(GLOB_RECURSE SERVLET_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

set(SERVLET_NAME "rocksdb")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/build.mk.in"
               "${CMAKE_CURRENT_BINARY_DIR}/build.mk")

add_custom_target(rocksdb ALL 
	COMMAND make -f ${PLUMBER_PREFIX}/lib/plumber/servlet.mk
	BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/bin/lib${SERVLET_NAME}.so
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	SOURCES ${SERVLET_SOURCE})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

add_custom_target(servlet-clean
	COMMAND make -f ${PLUMBER_PREFIX}/lib/plumber/servlet.mk clean
)

add_custom_target(distclean
	COMMAND make -f ${PLUMBER_PREFIX}/lib/plumber/servlet.mk clean
	COMMAND make clean
	COMMAND rm -rf CMakeFiles CMakeCache.txt Testing cmake_install.cmake CTestTestfile.cmake Makefile tags bin build.mk
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
