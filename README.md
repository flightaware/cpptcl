[![Build Status](https://travis-ci.org/flightaware/cpptcl.svg?branch=master)](https://travis-ci.org/flightaware/cpptcl)

C++/Tcl - a C++ library for interoperability between C++ and Tcl.
========

Welcome to C++/Tcl!

C++/Tcl... let's call it cpptcl... creates a bridge between C++ and Tcl, providing deep interoperability between the two languages.

C++ developers can use Tcl to create new and powerful ways to interact with, debug, construct automated testing for, and orchestrate the high level activies of their applications.

Tcl developers can use C++ to create high performance code that leverages all of the C++ ecosystem while providing dead simple ways to bring their C++ functions into Tcl, with expected Tcl semantics and no funny business. Writing C++ extensions for Tcl using cpptcl is vastly easier and requires far less code than extending Tcl in C.

Likewise 

[cpptcl documentation](https://github.com/flightaware/cpptcl/tree/master/doc)

Copyright (C) 2004-2006, Maciej Sobczak
Copyright (C) 2017-2019, FlightAware LLC

---

You should see the following directories here:

- doc      - contains a short documentation for the library
- examples - some simple examples to show C++/Tcl basics
- test     - tests used for development - use them for regression testing
             if you want to modify the library
- details  - some additional code used by the library (not for client use)

In addition, the following files may be of interest to you:

- README   - this file
- LICENSE  - explanation of the license terms
- CHANGES  - the history of changes


The C++/Tcl itself consists of these files:
- cpptcl/cpptcl.h - to be included in your own files
- cpptcl/cpptcl_object.h - C++ class that wraps Tcl objects
- cpptcl.cc - to be compiled and linked with your code

We're using cmake to generate makefiles.  This is pretty standard in the C++ world.

In order to compile the tests and the examples you may need to change the compiler options to fit your particular environment (different paths to the Tcl headers and libs, another compiler, etc.).  Tests and examples are built by default when building cpptcl (although not when included as a cmake subproject).  You can disable the tests and examples by setting `-DCPPTCL_TEST=OFF` and `-DCPPTCL_EXAMPLES=OFF` respectively.

---

cmake

The cpptcl install includes a cpptcl-config.cmake and target files for easy inclusion using cmake.  You can therefore use `find_package(cpptcl)` to add the dependency.  It will expose the `cpptcl_INCLUDE_DIR` variable with the header file location and the following targets to link against:

cpptcl::cpptcl
cpptcl::cpptcl_static
cpptcl::cpptcl_runtime

Here's a simple example of including cpptcl in your CMakeLists.txt file:

```
find_package(cpptcl REQUIRED)
find_package(TCL REQUIRED)

add_executable(myexec main.cpp)
target_include_directories(myexec PRIVATE ${cpptcl_INCLUDE_DIR})
target_include_directories(myexec PRIVATE ${TCL_INCLUDE_PATH})
target_link_libraries(myexec cpptcl::cpptcl)
target_link_libraries(myexec ${TCL_LIBRARY})
```

Anyway - have fun! :-)
