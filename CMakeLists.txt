cmake_minimum_required (VERSION 2.6)
project(plumber-rocks)

set(CMAKE_VERBOSE_MAKEFILE ON)

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

set(CFLAGS "$ENV{CFLAGS} -Wall -Werror")
set(LDFLAGS $ENV{LDFLAGS})

find_file(SERVLET_CMAKE NAMES "lib/plumber/cmake/servlet.cmake" HINTS "${PLUMBER_HINT}")
include(${SERVLET_CMAKE})

file(GLOB_RECURSE SERVLET_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
set(rocksdb_PLUMBER_LIBS proto pstd jsonschema)
FindRocksdb(rocksdb)
AddServlet(rocksdb ${SERVLET_SOURCE})

add_custom_target(distclean
	COMMAND make clean
	COMMAND rm -rf CMakeFiles CMakeCache.txt Testing cmake_install.cmake CTestTestfile.cmake Makefile tags bin build.mk
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

enable_testing()
file(GLOB scripts "${CMAKE_CURRENT_SOURCE_DIR}/test/*.pss")
foreach(script ${scripts})
	get_filename_component(test_name ${script} NAME_WE)
	file(GLOB inputs "${CMAKE_CURRENT_SOURCE_DIR}/test/data/${test_name}/*.in")
	add_test(${test_name} python ${CMAKE_CURRENT_SOURCE_DIR}/test/runtest.py ${script} ${inputs})
endforeach(script ${scripts})
