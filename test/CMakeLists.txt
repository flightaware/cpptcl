cmake_minimum_required(VERSION 3.0)

if (NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux") 
    set(CMAKE_CXX_COMPILER clang++)
endif()

project(cpptcl_test)

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

find_path(TCL_INCLUDE_PATH tcl.h PATHS /usr/local/include/tcl8.6 /usr/include/tcl8.6 NO_DEFAULT_PATH)
find_library(TCL_LIBRARY NAMES tcl8.6 tcl86 PATHS /usr/local/lib /usr/lib)
find_library(TCL_STUB_LIBRARY NAMES tclstub8.6 tclstub86 PATHS /usr/local/lib /usr/lib)

message(INFO " Tcl include ${TCL_INCLUDE_PATH}")
message(INFO " Tcl library ${TCL_LIBRARY}")
message(INFO " Tcl stub library ${TCL_STUB_LIBRARY}")

if(NOT TCL_LIBRARY)
  message(FATAL_ERROR " Tcl library not found")
endif()

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

include_directories(${CMAKE_SOURCE_DIR}/.. ${TCL_INCLUDE_PATH})

add_compile_options(${OPTS})

add_executable(test1 test1.cc ../cpptcl.cc)
add_test(test1 test1)
target_link_libraries(test1 PUBLIC ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(test3 test3.cc ../cpptcl.cc)
add_test(test3 test3)
target_link_libraries(test3 PUBLIC ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(test4 test4.cc ../cpptcl.cc)
add_test(test4 test4)
target_link_libraries(test4 PUBLIC ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(test5 test5.cc ../cpptcl.cc)
add_test(test5 test5)
target_link_libraries(test5 PUBLIC ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(test6 test6.cc ../cpptcl.cc)
add_test(test6 test6)
target_link_libraries(test6 PUBLIC ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(test_main test_main.cc ../cpptcl.cc)
add_test(test_main test_main)
target_link_libraries(test_main PUBLIC ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_library(test2 SHARED test2.cc ../cpptcl.cc)
target_link_libraries(test2 PUBLIC ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})
add_test(NAME test2 COMMAND tclsh ../test2.tcl)

