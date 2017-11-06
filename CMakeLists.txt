cmake_minimum_required(VERSION 2.8)

project(Mymodule)

find_library(TCLLIB NAMES tcl8.6 HINTS "$ENV{HOME}/git/tcl/build")
find_library(TCLSTUBLIB NAMES tclstub8.6 HINTS "$ENV{HOME}/git/tcl/build")

if(NOT TCLLIB)
  message(FATAL_ERROR "tcl8.6 library not found")
endif()

if(NOT TCLSTUBLIB)
  message(FATAL_ERROR "tclstub8.6 library not found")
endif()

set(CMAKE_BUILD_TYPE Debug)

include_directories(. details)

add_compile_options(-std=c++11 -stdlib=libc++ -Weverything)

list(APPEND SRCS cpptcl.cc)
list(APPEND SRCS cpptcl.h)
list(APPEND SRCS examples/example7.cc)
list(APPEND SRCS details/callbacks.h)
list(APPEND SRCS details/callbacks_v.h) 
list(APPEND SRCS details/constructors.h)
list(APPEND SRCS details/conversions.h) 
list(APPEND SRCS details/dispatchers.h) 
list(APPEND SRCS details/metahelpers.h) 
list(APPEND SRCS details/methods.h) 
list(APPEND SRCS details/methods_v.h)

add_library(Mymodule SHARED ${SRCS})
target_link_libraries(Mymodule ${TCLLIB} ${TCLSTUBLIB})
