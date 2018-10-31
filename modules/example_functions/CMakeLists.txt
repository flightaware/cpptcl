cmake_minimum_required(VERSION 3.0)

if (NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux") 
    set(CMAKE_CXX_COMPILER clang++)
endif()

project(cpptcl_example_functions)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list(APPEND OPTS -stdlib=libc++)
    list(APPEND OPTS -Wno-reserved-id-macro)
    list(APPEND OPTS -Wno-padded)
    list(APPEND OPTS -Wno-c++98-compat)
endif()

set(CPPTCL_EXAMPLE_FUNCTIONS_VERSION 0.0.1)
add_compile_options("-DCPPTCL_EXAMPLE_FUNCTIONS_VERSION=\"${CPPTCL_EXAMPLE_FUNCTIONS_VERSION}\"")
add_compile_options(${OPTS})

add_library(cpptcl_example_functions SHARED cpptcl_example_functions.cc)
set_target_properties(cpptcl_example_functions PROPERTIES VERSION ${CPPTCL_EXAMPLE_FUNCTIONS_VERSION})

find_library(CPPTCL_LIBRARIES cpptcl HINTS ${CPPTCL_SOURCE_DIR}/build)
find_path (CPPTCL_INCLUDE_DIRS cpptcl/cpptcl.h HINTS "${CPPTCL_SOURCE_DIR}" PATHS ${CMAKE_INCLUDE_PATH})

find_path(TCL_EXECUTABLE_PATH tclsh PATHS /usr/local/bin /usr/bin /bin NO_DEFAULT_PATH)
find_path(TCL_INCLUDE_PATH tcl.h PATHS /usr/local/include/tcl8.6 /usr/local/include /usr/include/tcl8.6 NO_DEFAULT_PATH)
find_library(TCL_LIBRARY NAMES tcl8.6 tcl86 PATHS /usr/local/lib)
find_library(TCL_STUB_LIBRARY NAMES tclstub8.6 tclstub86 PATHS /usr/local/lib)

set(TCL_TCLSH "${TCL_EXECUTABLE_PATH}/tclsh")

message(INFO " Tcl tclsh ${TCL_TCLSH}")
message(INFO " Tcl include ${TCL_INCLUDE_PATH}")
message(INFO " Tcl library ${TCL_LIBRARY}")
message(INFO " Tcl stub library ${TCL_STUB_LIBRARY}")

include_directories(${CPPTCL_INCLUDE_DIRS} ${TCL_INCLUDE_PATH})
target_link_libraries (cpptcl_example_functions ${CPPTCL_LIBRARIES} ${TCL_STUB_LIBRARY})
install(TARGETS cpptcl_example_functions LIBRARY DESTINATION lib/cpptcl_example_functions_${CPPTCL_EXAMPLE_FUNCTIONS_VERSION})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl DESTINATION lib/cpptcl_example_functions_${CPPTCL_EXAMPLE_FUNCTIONS_VERSION})
add_custom_command(TARGET ${PROJECT_NAME}
   POST_BUILD
   COMMAND /bin/sh ARGS -c "echo pkg_mkIndex .|${TCL_TCLSH}")

set(CMAKE_BUILD_TYPE Debug)

