cmake_minimum_required(VERSION 2.8)

project(cpplib)

find_library(TCLSTUBLIB NAMES tclstub8.6 HINTS "$ENV{HOME}/git/tcl/build" PATHS "/usr/local/lib")

if(NOT TCLSTUBLIB)
  message(FATAL_ERROR "tclstub8.6 library not found")
endif()

set(CMAKE_BUILD_TYPE Debug)

include_directories(. details /usr/local/include)

add_compile_options(-DUSE_TCL_STUBS -std=c++11 -stdlib=libc++ -Weverything -Wno-c++98-compat)

list(APPEND SRCS cpptcl.cc)
list(APPEND SRCS cpptcl.h)
list(APPEND SRCS cpptcl_object.h)
list(APPEND SRCS examples/example7.cc)
list(APPEND SRCS details/callbacks.h)
list(APPEND SRCS details/callbacks_v.h) 
list(APPEND SRCS details/constructors.h)
list(APPEND SRCS details/conversions.h) 
list(APPEND SRCS details/dispatchers.h) 
list(APPEND SRCS details/metahelpers.h) 
list(APPEND SRCS details/methods.h) 
list(APPEND SRCS details/methods_v.h)

add_library(cpplib SHARED ${SRCS})
target_link_libraries(cpplib ${TCLLIB} ${TCLSTUBLIB})
