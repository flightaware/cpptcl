cmake_minimum_required(VERSION 3.0)

project(cpptcl)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to Release as none was specified.")
  set(CMAKE_BUILD_TYPE Release)
endif()
message(INFO " cmake build type ${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include (CTest)

find_package(TCL)

if(NOT TCL_LIBRARY)
  message(FATAL_ERROR " Tcl library not found")
else()
  message(INFO " Tcl include ${TCL_INCLUDE_PATH}")
  message(INFO " Tcl library ${TCL_LIBRARY}")
  message(INFO " Tcl stub library ${TCL_STUB_LIBRARY}")
  message(INFO " Tcl shell ${TCL_TCLSH}")
endif()

include_directories(${cpptcl_SOURCE_DIR} ${cpptcl_SOURCE_DIR}/details ${TCL_INCLUDE_PATH})

add_compile_options(${OPTS})

list(APPEND SRCS ${cpptcl_SOURCE_DIR}/cpptcl.cc)
list(APPEND HDRS ${cpptcl_SOURCE_DIR}/cpptcl/cpptcl.h)
list(APPEND HDRS ${cpptcl_SOURCE_DIR}/cpptcl/cpptcl_object.h)
list(APPEND HDRS_DETAILS ${cpptcl_SOURCE_DIR}/cpptcl/details/callbacks.h)
list(APPEND HDRS_DETAILS ${cpptcl_SOURCE_DIR}/cpptcl/details/callbacks_v.h) 
list(APPEND HDRS_DETAILS ${cpptcl_SOURCE_DIR}/cpptcl/details/constructors.h)
list(APPEND HDRS_DETAILS ${cpptcl_SOURCE_DIR}/cpptcl/details/conversions.h) 
list(APPEND HDRS_DETAILS ${cpptcl_SOURCE_DIR}/cpptcl/details/dispatchers.h) 
list(APPEND HDRS_DETAILS ${cpptcl_SOURCE_DIR}/cpptcl/details/metahelpers.h) 
list(APPEND HDRS_DETAILS ${cpptcl_SOURCE_DIR}/cpptcl/details/methods.h) 
list(APPEND HDRS_DETAILS ${cpptcl_SOURCE_DIR}/cpptcl/details/methods_v.h)
list(APPEND HDRS_DETAILS ${cpptcl_SOURCE_DIR}/cpptcl/details/bind.h)

add_library(cpptcl SHARED ${SRCS} ${HDRS} ${HDRS_DETAILS})
add_library(cpptcl_static STATIC ${SRCS} ${HDRS} ${HDRS_DETAILS})
set_property(TARGET cpptcl_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(cpptcl ${TCL_STUB_LIBRARY})
install(TARGETS cpptcl LIBRARY DESTINATION lib)
install(TARGETS cpptcl_static ARCHIVE DESTINATION lib)
install(FILES ${HDRS} DESTINATION include/cpptcl)
install(FILES ${HDRS_DETAILS} DESTINATION include/cpptcl/details)

add_library(cpptcl_runtime STATIC ${cpptcl_SOURCE_DIR}/cpptcl_runtime.c)
set_property(TARGET cpptcl_runtime PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(cpptcl_runtime ${TCL_LIBRARY})

add_subdirectory(test EXCLUDE_FROM_ALL)
add_subdirectory(examples EXCLUDE_FROM_ALL)

list(APPEND LIST_OF_TESTS test1)
list(APPEND LIST_OF_TESTS test2)
list(APPEND LIST_OF_TESTS test3)
list(APPEND LIST_OF_TESTS test4)
list(APPEND LIST_OF_TESTS test5)
list(APPEND LIST_OF_TESTS test6)
list(APPEND LIST_OF_TESTS test7)
list(APPEND LIST_OF_TESTS test_main)

add_custom_target(tests DEPENDS ${LIST_OF_TESTS})

list(APPEND LIST_OF_EXAMPLES example2)
list(APPEND LIST_OF_EXAMPLES example6)
list(APPEND LIST_OF_EXAMPLES libcpptcl_module_two)
list(APPEND LIST_OF_EXAMPLES libcpptcl_module_three)
list(APPEND LIST_OF_EXAMPLES libcpptcl_module_five)
list(APPEND LIST_OF_EXAMPLES libcpptcl_module_six)
list(APPEND LIST_OF_EXAMPLES cpptcl_example_functions)

add_custom_target(examples DEPENDS ${LIST_OF_EXAMPLES})

