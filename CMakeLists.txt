cmake_minimum_required(VERSION 3.8)

project(cpptcl)

include(cmake/version.cmake)
load_git_properties(cpptcl ${CMAKE_BINARY_DIR}/generated)

set(CPPTCL_VERSION 2.2.5)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to Release as none was specified.")
  set(CMAKE_BUILD_TYPE Release)
endif()
message(INFO " cmake build type ${CMAKE_BUILD_TYPE}")

set(TCLSH_VERSION_STRING, "8.6")

# Determine if cpptcl is built as a subproject (using add_subdirectory)
# or if it is the master project.
set(CPPTCL_MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(CPPTCL_MASTER_PROJECT ON)
endif()

option(CPPTCL_INSTALL "Generate the install target." ${CPPTCL_MASTER_PROJECT})
option(CPPTCL_TEST "Build the tests." ${CPPTCL_MASTER_PROJECT})
option(CPPTCL_EXAMPLES "Build the examples." ${CPPTCL_MASTER_PROJECT})

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

set(TCL_VERSION_MAJOR "8" CACHE STRING "TCL Major version")
set(TCL_VERSION_MINOR "6" CACHE STRING "TCL Minor version")
set(TCL_TCLSH "tclsh${TCL_VERSION_MAJOR}.${TCL_VERSION_MINOR}")
set(TCL_VER "${TCL_VERSION_MAJOR}.${TCL_VERSION_MINOR}")
set(TCL_VER_BSD "${TCL_VERSION_MAJOR}${TCL_VERSION_MINOR}")

message(INFO " TCL_VER ${TCL_VER}")

find_path(TCL_INCLUDE_PATH tcl.h PATHS /usr/local/include/tcl${TCL_VER} /usr/local/include /usr/include/tcl${TCL_VER} /usr/include NO_DEFAULT_PATH)
find_library(TCL_LIBRARY NAMES tcl${TCL_VER} tcl${TCL_VER_BSD} PATHS /usr/local/lib /usr/lib)
find_library(TCL_STUB_LIBRARY NAMES tclstub${TCL_VER} tclstub${TCL_VER_BSD} PATHS /usr/local/lib /usr/lib)

message(INFO " Tcl include ${TCL_INCLUDE_PATH}")
message(INFO " Tcl library ${TCL_LIBRARY}")
message(INFO " Tcl stub library ${TCL_STUB_LIBRARY}")
message(INFO " Tcl shell ${TCL_TCLSH}")

if(NOT TCL_LIBRARY)
  message(FATAL_ERROR " Tcl library not found")
endif()

add_compile_options(${OPTS})

set(cpptcl_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND SRCS ${cpptcl_SOURCE_DIR}/cpptcl.cc)
list(APPEND SRCS ${CMAKE_BINARY_DIR}/generated/cpptcl_version.cpp)
list(APPEND HDRS ${cpptcl_SOURCE_DIR}/cpptcl/cpptcl.h)
list(APPEND HDRS ${cpptcl_SOURCE_DIR}/cpptcl/cpptcl_object.h)
list(APPEND HDRS ${cpptcl_SOURCE_DIR}/cpptcl/version.h)
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
add_library(cpptcl::cpptcl ALIAS cpptcl)
target_compile_features(cpptcl PUBLIC cxx_std_11)
set_target_properties(cpptcl PROPERTIES
	CXX_EXTENSIONS OFF
	CXX_STANDARD_REQUIRED ON)
target_include_directories(cpptcl
	PUBLIC
		$<BUILD_INTERFACE:${cpptcl_INCLUDE_DIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	)
target_include_directories(cpptcl PUBLIC ${TCL_INCLUDE_PATH})
target_link_libraries(cpptcl PUBLIC ${TCL_STUB_LIBRARY})

add_library(cpptcl_static STATIC ${SRCS} ${HDRS} ${HDRS_DETAILS})
add_library(cpptcl::cpptcl_static ALIAS cpptcl_static)
target_compile_features(cpptcl_static PUBLIC cxx_std_11)
set_target_properties(cpptcl_static PROPERTIES
	POSITION_INDEPENDENT_CODE ON
	CXX_EXTENSIONS OFF
	CXX_STANDARD_REQUIRED ON)
target_include_directories(cpptcl_static
	PUBLIC
		$<BUILD_INTERFACE:${cpptcl_INCLUDE_DIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	)
target_include_directories(cpptcl_static PUBLIC ${TCL_INCLUDE_PATH})

add_library(cpptcl_runtime STATIC ${cpptcl_SOURCE_DIR}/cpptcl_runtime.c)
add_library(cpptcl::cpptcl_runtime ALIAS cpptcl_runtime)
target_compile_features(cpptcl_runtime PUBLIC cxx_std_11)
set_target_properties(cpptcl_runtime PROPERTIES
	POSITION_INDEPENDENT_CODE ON
	CXX_EXTENSIONS OFF
	CXX_STANDARD_REQUIRED ON)
target_include_directories(cpptcl_runtime
	PUBLIC
		$<BUILD_INTERFACE:${cpptcl_INCLUDE_DIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	)
target_include_directories(cpptcl_runtime PUBLIC ${TCL_INCLUDE_PATH})
target_link_libraries(cpptcl_runtime PUBLIC ${TCL_LIBRARY})

if (CPPTCL_TEST)
	add_subdirectory(test)
endif()

if (CPPTCL_EXAMPLES)
	add_subdirectory(examples)
endif()

if (CPPTCL_INSTALL)
	include(GNUInstallDirs)
	include(CMakePackageConfigHelpers)

	set(cpptcl_INCLUDE_DIR include/ CACHE STRING
		"cpptcl header install directory")
	set(CPPTCL_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/cpptcl CACHE STRING
		"Installation directory for cmake files, relative to ${CMAKE_INSTALL_PREFIX}.")
	set(CPPTCL_VERSION_CONFIG ${PROJECT_BINARY_DIR}/cpptcl-config-version.cmake)
	set(CPPTCL_PROJECT_CONFIG ${PROJECT_BINARY_DIR}/cpptcl-config.cmake)

	configure_package_config_file(
		"cmake/cpptcl-config.cmake.in"
		${CPPTCL_PROJECT_CONFIG}
		INSTALL_DESTINATION ${CPPTCL_CMAKE_DIR}
		PATH_VARS cpptcl_INCLUDE_DIR)

	write_basic_package_version_file(
		${CPPTCL_VERSION_CONFIG}
		VERSION ${CPPTCL_VERSION}
		COMPATIBILITY SameMajorVersion )

	install(
		FILES ${CPPTCL_PROJECT_CONFIG} ${CPPTCL_VERSION_CONFIG}
		DESTINATION ${CPPTCL_CMAKE_DIR})

	list(APPEND CPPTCL_INSTALL_TARGETS cpptcl)
	list(APPEND CPPTCL_INSTALL_TARGETS cpptcl_static)
	list(APPEND CPPTCL_INSTALL_TARGETS cpptcl_runtime)

	install(
		TARGETS ${CPPTCL_INSTALL_TARGETS}
		EXPORT CPPTCL_ALL_TARGETS
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
	install(
		DIRECTORY cpptcl/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cpptcl
		FILES_MATCHING PATTERN *.h)

	install(
		EXPORT CPPTCL_ALL_TARGETS
		NAMESPACE cpptcl::
		FILE cpptcl-targets.cmake
		DESTINATION ${CPPTCL_CMAKE_DIR} )

endif()
