cmake_minimum_required(VERSION 3.0)

set(TCL_TCLSH tclsh8.6)

add_executable(test1 test1.cc ../cpptcl.cc)
add_test(test1 test1)
target_include_directories(test1 PRIVATE ${cpptcl_INCLUDE_DIR} ${TCL_INCLUDE_PATH})
target_link_libraries(test1 PRIVATE ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_library(test2 SHARED test2.cc ../cpptcl.cc)
target_include_directories(test2 PRIVATE ${cpptcl_INCLUDE_DIR} ${TCL_INCLUDE_PATH})
target_link_libraries(test2 PRIVATE ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})
add_test(NAME test2 COMMAND ${TCL_TCLSH} ${CMAKE_SOURCE_DIR}/test/test2.tcl)

add_executable(test3 test3.cc ../cpptcl.cc)
add_test(test3 test3)
target_include_directories(test3 PRIVATE ${cpptcl_INCLUDE_DIR} ${TCL_INCLUDE_PATH})
target_link_libraries(test3 PRIVATE ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(test4 test4.cc ../cpptcl.cc)
add_test(test4 test4)
target_include_directories(test4 PRIVATE ${cpptcl_INCLUDE_DIR} ${TCL_INCLUDE_PATH})
target_link_libraries(test4 PRIVATE ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(test5 test5.cc ../cpptcl.cc)
add_test(test5 test5)
target_include_directories(test5 PRIVATE ${cpptcl_INCLUDE_DIR} ${TCL_INCLUDE_PATH})
target_link_libraries(test5 PRIVATE ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(test6 test6.cc ../cpptcl.cc)
add_test(test6 test6)
target_include_directories(test6 PRIVATE ${cpptcl_INCLUDE_DIR} ${TCL_INCLUDE_PATH})
target_link_libraries(test6 PRIVATE ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(test7 test7.cc ../cpptcl.cc)
add_test(test7 test7)
target_include_directories(test7 PRIVATE ${cpptcl_INCLUDE_DIR} ${TCL_INCLUDE_PATH})
target_link_libraries(test7 PRIVATE ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

add_executable(test_main test_main.cc ../cpptcl.cc)
add_test(test_main test_main)
target_include_directories(test_main PRIVATE ${cpptcl_INCLUDE_DIR} ${TCL_INCLUDE_PATH})
target_link_libraries(test_main PRIVATE ${TCL_LIBRARY} ${TCL_STUB_LIBRARY})

