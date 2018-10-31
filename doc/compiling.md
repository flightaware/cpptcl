[[prev](errors.md)][[top](README.md)]  

#### Compiling  

The C++/Tcl library depends on a modern C++17 compiler. We test on both Clang and g++ toolchains. The C++/Tcl library is compiled using CMake. We provide simple makefiles for FreeBSD, Linux and OS X platforms.  

The C++/Tcl library consists of the following files:  

*   `cpptcl/cpptcl.h` - to be `#include`d in your own projects,  

*   `cpptcl.cc` - to be linked with your own projects,
*   additional files in the `cpptcl/details` directory - this directory should be visible on the include search path.  

The C++/Tcl library depends on the Tcl core library. We support TCL 8.6.x and higher.  

#### Threads

Note: In its current version, the C++/Tcl library is not thread-safe in the sense that it should not be used in the multithreaded environment, where many threads can register new commands or use extended commands in scripts.

#### Linking

The C++/Tcl library provides two libraries. Applications that will embed TCL can used static linking. TCL extensions require the use of dynamic linking.

##### Static Linking

The static library libcpptcl_static can be linked with `-lcpptcl_static`. TCL can be linked using the libtcl8.6.a archive file and libtclstub8.6.a files. Programs that embed TCL with static linkage must define `-DCPPTCL_NO_TCL_STUBS` to disable TCL's stub mechanism for dynamic loading.

##### Dynamic Linking

For dynamic linkage use `-lcpptcl`. The dynamic library for cpptcl will not create a dependency on the TCL dynamic library because it is compiled using the libtclstub8.6.a archive.  

The test code, in the test directory, provides the best examples of using the C++/TCL library. The tests include TCL interpreter creation and TCL extension creation.  

[[prev](errors.md)][[top](README.md)]  

* * *

Copyright © 2004-2006, Maciej Sobczak  

* * *

Copyright © 2018, FlightAware LLC
