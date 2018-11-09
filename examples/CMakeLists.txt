cmake_minimum_required(VERSION 3.0)

add_executable(example2 example2.cc)
target_link_libraries(example2 PUBLIC cpptcl_static ${TCL_STUB_LIBRARY} ${TCL_LIBRARY})

add_executable(example6 example6.cc)
target_link_libraries(example6 PUBLIC cpptcl_static ${TCL_STUB_LIBRARY} ${TCL_LIBRARY})

add_library(libcpptcl_module_two SHARED cpptcl_module_two.cc)
target_link_libraries(libcpptcl_module_two PUBLIC cpptcl_static ${TCL_STUB_LIBRARY})

add_library(libcpptcl_module_three SHARED cpptcl_module_three.cc)
target_link_libraries(libcpptcl_module_three PUBLIC cpptcl_static ${TCL_STUB_LIBRARY})

add_library(libcpptcl_module_five SHARED cpptcl_module_five.cc)
target_link_libraries(libcpptcl_module_five PUBLIC cpptcl_static ${TCL_STUB_LIBRARY})

add_library(libcpptcl_module_six SHARED cpptcl_module_six.cc)
target_link_libraries(libcpptcl_module_six PUBLIC cpptcl_static ${TCL_STUB_LIBRARY})

# Create a packaged TCL extension
add_library(cpptcl_example_functions SHARED cpptcl_example_functions.cc)
target_link_libraries (cpptcl_example_functions cpptcl_static ${TCL_STUB_LIBRARY})

set(CPPTCL_EXAMPLE_FUNCTIONS_VERSION 0.0.1)
set_target_properties(cpptcl_example_functions PROPERTIES VERSION ${CPPTCL_EXAMPLE_FUNCTIONS_VERSION})
target_compile_options(cpptcl_example_functions PRIVATE "-DCPPTCL_EXAMPLE_FUNCTIONS_VERSION=\"${CPPTCL_EXAMPLE_FUNCTIONS_VERSION}\"")

message(INFO " Tcl tclsh ${TCL_TCLSH}")
message(INFO " Tcl include ${TCL_INCLUDE_PATH}")
message(INFO " Tcl stub library ${TCL_STUB_LIBRARY}")

set(CPPTCL_INSTALL_EXAMPLE FALSE CACHE BOOL "Install example TCL extension")
#
# To install the example TCL extension as a package
#    cd build
#    cmake -DCPPTCL_INSTALL_EXAMPLE:BOOL=TRUE ..
#    make
#    make install
#
if(CPPTCL_INSTALL_EXAMPLE)
    #
    # Install the cpptcl_example_functions as a TCL package
    #
    install(TARGETS cpptcl_example_functions LIBRARY DESTINATION lib/cpptcl_example_functions_${CPPTCL_EXAMPLE_FUNCTIONS_VERSION})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl DESTINATION lib/cpptcl_example_functions_${CPPTCL_EXAMPLE_FUNCTIONS_VERSION})
    #
    # Generate the pkgIndex.tcl
    #
    # We need to remove the unversioned library symlink to avoid double loads in pkgIndex.tcl
    #
    add_custom_command(TARGET cpptcl_example_functions
       POST_BUILD
       WORKING_DIRECTORY ${PROJECT_BIN_DIR}
       COMMAND /bin/rm ARGS "./libcpptcl_example_functions${CMAKE_SHARED_LIBRARY_SUFFIX}")
    add_custom_command(TARGET cpptcl_example_functions
       POST_BUILD
       COMMAND /bin/sh ARGS -c "echo pkg_mkIndex .|${TCL_TCLSH}")
endif(CPPTCL_INSTALL_EXAMPLE)
