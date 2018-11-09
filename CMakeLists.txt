cmake_minimum_required(VERSION 3.0)

project(cpptcl)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to Release as none was specified.")
  set(CMAKE_BUILD_TYPE Release)
endif()
message(INFO " cmake build type ${CMAKE_BUILD_TYPE}")

set(TCLSH_VERSION_STRING, "8.6")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list(APPEND OPTS -stdlib=libc++)
    list(APPEND OPTS -Wno-reserved-id-macro)
    list(APPEND OPTS -Wno-padded)
    list(APPEND OPTS -Wno-c++98-compat)
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    endif()
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

add_subdirectory(test)
add_subdirectory(examples)
