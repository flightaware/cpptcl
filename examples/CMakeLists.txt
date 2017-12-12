cmake_minimum_required(VERSION 3.0)

if (NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux") 
    set(CMAKE_CXX_COMPILER clang++)
endif()

project(examples)

set(TCLSH_VERSION_STRING, "8.6")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list(APPEND OPTS -stdlib=libc++)
    list(APPEND OPTS -Wno-reserved-id-macro)
    list(APPEND OPTS -Wno-padded)
    list(APPEND OPTS -Wno-c++98-compat)
endif()

find_path(TCL_INCLUDE_PATH tcl.h PATHS /usr/local/include/tcl8.6 /usr/local/include /usr/include/tcl8.6 NO_DEFAULT_PATH)
find_library(TCL_LIBRARY NAMES tcl8.6 tcl86 HINTS /Users/shannonnoe/git/tcl/unix PATHS /usr/local/lib)
find_library(TCL_STUB_LIBRARY NAMES tclstub8.6 tclstub86 HINTS /Users/shannonnoe/git/tcl/unix PATHS /usr/local/lib)

message(INFO " Tcl include ${TCL_INCLUDE_PATH}")
message(INFO " Tcl library ${TCL_LIBRARY}")
message(INFO " Tcl stub library ${TCL_STUB_LIBRARY}")

if(NOT TCL_LIBRARY)
  message(FATAL_ERROR " Tcl library not found")
endif()

find_library(CPPTCL_LIBRARY cpptcl PATHS /usr/local/lib)
find_path (CPPTCL_INCLUDE_DIRS cpptcl/cpptcl.h PATHS ${CMAKE_INCLUDE_PATH})

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

include_directories(${CPPTCL_INCLUDE_DIRS} ${TCL_INCLUDE_PATH})

add_compile_options(${OPTS})

add_executable(example2 example2.cc)
target_link_libraries(example2 PUBLIC ${CPPTCL_LIBRARY} ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(example6 example6.cc)
target_link_libraries(example6 PUBLIC ${CPPTCL_LIBRARY} ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

