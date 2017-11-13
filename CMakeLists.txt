cmake_minimum_required(VERSION 2.8)

if (NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux") 
    set(CMAKE_CXX_COMPILER clang++)
endif()

project(cpptcl)

include(FindTclStub)

if(NOT TCL_FOUND)
  message(FATAL_ERROR "Tcl installation not found")
endif()

set(CMAKE_BUILD_TYPE Debug)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/details ${TCL_INCLUDE_PATH})

list(APPEND OPTS -DUSE_TCL_STUBS)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    list(APPEND OPTS -stdlib=libc++)
    list(APPEND OPTS -Wno-reserved-id-macro)
    list(APPEND OPTS -Wno-padded)
    list(APPEND OPTS -Wno-c++98-compat)
endif()

add_compile_options(${OPTS})

list(APPEND SRCS ${CMAKE_SOURCE_DIR}/cpptcl.cc)
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/cpptcl.h)
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/cpptcl_object.h)
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/details/callbacks.h)
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/details/callbacks_v.h) 
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/details/constructors.h)
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/details/conversions.h) 
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/details/dispatchers.h) 
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/details/metahelpers.h) 
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/details/methods.h) 
list(APPEND HDRS ${CMAKE_SOURCE_DIR}/details/methods_v.h)

add_library(cpptcl SHARED ${SRCS} ${HDRS})
target_link_libraries(cpptcl ${TCL_STUB_LIBRARY})
install(TARGETS cpptcl LIBRARY DESTINATION lib)
install(FILES ${HDRS} DESTINATION include/cpptcl)

add_library(cpptcl_module_one SHARED modules/cpptcl_module_one.cc)
target_link_libraries(cpptcl_module_one cpptcl)

add_library(cpptcl_module_two SHARED modules/cpptcl_module_two.cc)
target_link_libraries(cpptcl_module_two cpptcl)

add_library(cpptcl_module_three SHARED modules/cpptcl_module_three.cc)
target_link_libraries(cpptcl_module_three cpptcl)

add_library(cpptcl_module_five SHARED modules/cpptcl_module_five.cc)
target_link_libraries(cpptcl_module_five cpptcl)

add_library(cpptcl_module_six SHARED modules/cpptcl_module_six.cc)
target_link_libraries(cpptcl_module_six cpptcl)

