cmake_minimum_required(VERSION 3.0)

if (NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux") 
    set(CMAKE_CXX_COMPILER clang++)
endif()

project(cpptcl)

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

include (CTest)

find_path(TCL_INCLUDE_PATH tcl.h PATHS /usr/local/include/tcl8.6 /usr/local/include /usr/include/tcl8.6 NO_DEFAULT_PATH)
find_library(TCL_LIBRARY NAMES tcl8.6 tcl86 PATHS /usr/local/lib)
find_library(TCL_STUB_LIBRARY NAMES tclstub8.6 tclstub86 PATHS /usr/local/lib)

message(INFO " Tcl include ${TCL_INCLUDE_PATH}")
message(INFO " Tcl library ${TCL_LIBRARY}")
message(INFO " Tcl stub library ${TCL_STUB_LIBRARY}")

if(NOT TCL_LIBRARY)
  message(FATAL_ERROR " Tcl library not found")
endif()

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/details ${TCL_INCLUDE_PATH})

add_compile_options(${OPTS})

list(APPEND SRCS ${CMAKE_SOURCE_DIR}/cpptcl.cc)
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/cpptcl/cpptcl.h)
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/cpptcl/cpptcl_object.h)
list(APPEND HDRS_DETAILS ${CMAKE_SOURCE_DIR}/cpptcl/details/callbacks.h)
list(APPEND HDRS_DETAILS ${CMAKE_SOURCE_DIR}/cpptcl/details/callbacks_v.h) 
list(APPEND HDRS_DETAILS ${CMAKE_SOURCE_DIR}/cpptcl/details/constructors.h)
list(APPEND HDRS_DETAILS ${CMAKE_SOURCE_DIR}/cpptcl/details/conversions.h) 
list(APPEND HDRS_DETAILS ${CMAKE_SOURCE_DIR}/cpptcl/details/dispatchers.h) 
list(APPEND HDRS_DETAILS ${CMAKE_SOURCE_DIR}/cpptcl/details/metahelpers.h) 
list(APPEND HDRS_DETAILS ${CMAKE_SOURCE_DIR}/cpptcl/details/methods.h) 
list(APPEND HDRS_DETAILS ${CMAKE_SOURCE_DIR}/cpptcl/details/methods_v.h)
list(APPEND HDRS_DETAILS ${CMAKE_SOURCE_DIR}/cpptcl/details/bind.h)

add_library(cpptcl SHARED ${SRCS} ${HDRS} ${HDRS_DETAILS})
target_link_libraries(cpptcl ${TCL_STUB_LIBRARY})
install(TARGETS cpptcl LIBRARY DESTINATION lib)
install(FILES ${HDRS} DESTINATION include/cpptcl)
install(FILES ${HDRS_DETAILS} DESTINATION include/cpptcl/details)

add_executable(cpptcl_test test/test_main.cc)
add_test(test1 cpptcl_test)
target_link_libraries(cpptcl_test PUBLIC cpptcl ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})
