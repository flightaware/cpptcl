cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux") 
	message(INFO "Linux link patching")
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_STANDARD_LIBRARIES "-lc++ /usr/lib/x86_64-linux-gnu/libc++abi.so.1.0")
endif()

project(cpptcl)

# Technically you should set TCL_INCLUDE_SPEC from tclConfig.sh
find_path(TCL_INCLUDE NAMES tcl.h PATHS /usr/local/include /usr/include ENV TCL_INCLUDE_SPEC) 

find_library(TCLSTUBLIB NAMES tclstub8.6 HINTS "$ENV{HOME}/git/tcl/build" PATHS "/usr/local/lib")

if(NOT TCLSTUBLIB)
  # bsd does not use the same name for stubs
  find_library(TCLSTUBLIB NAMES tclstub86 HINTS "$ENV{HOME}/git/tcl/build" PATHS "/usr/local/lib")
  if(NOT TCLSTUBLIB)
    message(FATAL_ERROR "tclstub8.6 library not found")
  endif()
endif()

set(CMAKE_BUILD_TYPE Debug)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/details ${TCL_INCLUDE})

add_compile_options(-DUSE_TCL_STUBS -std=c++11 -stdlib=libc++ -Weverything -Wno-c++98-compat -Wno-padded)

list(APPEND SRCS cpptcl.cc)
list(APPEND SRCS cpptcl.h)
list(APPEND SRCS cpptcl_object.h)
list(APPEND SRCS details/callbacks.h)
list(APPEND SRCS details/callbacks_v.h) 
list(APPEND SRCS details/constructors.h)
list(APPEND SRCS details/conversions.h) 
list(APPEND SRCS details/dispatchers.h) 
list(APPEND SRCS details/metahelpers.h) 
list(APPEND SRCS details/methods.h) 
list(APPEND SRCS details/methods_v.h)

add_library(cpptcl SHARED ${SRCS})
target_link_libraries(cpptcl ${TCLSTUBLIB})

add_library(cpptcl_module_one SHARED src/modules/cpptcl_module_one.cc)
target_link_libraries(cpptcl_module_one cpptcl ${TCLSTUBLIB})

add_library(cpptcl_module_two SHARED src/modules/cpptcl_module_two.cc)
target_link_libraries(cpptcl_module_two cpptcl ${TCLSTUBLIB})

add_library(cpptcl_module_three SHARED src/modules/cpptcl_module_three.cc)
target_link_libraries(cpptcl_module_three cpptcl ${TCLSTUBLIB})

add_library(cpptcl_module_five SHARED src/modules/cpptcl_module_five.cc)
target_link_libraries(cpptcl_module_five cpptcl ${TCLSTUBLIB})

add_library(cpptcl_module_six SHARED src/modules/cpptcl_module_six.cc)
target_link_libraries(cpptcl_module_six cpptcl ${TCLSTUBLIB})

