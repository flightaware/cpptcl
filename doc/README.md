Welcome to [cpptcl](http://github.com/flightaware/cpptcl)!  

cpptcl creates a bridge between C++ and Tcl, providing deep interoperability between the two languages.

C++ developers can use Tcl to create new and powerful ways to interact with, debug, construct automated testing for, and orchestrate the high level activies of their applications.

Tcl developers can use C++ to create high performance code that leverages all of the C++ ecosystem while providing dead simple ways to bring their C++ functions into Tcl, with expected Tcl semantics and no funny business. Writing C++ extensions for Tcl using cpptcl is vastly easier and requires far less code than extending Tcl in C.

cpptcl was inspired by the [Boost.Python](http://www.boost.org/libs/python/doc/index.html) library and was designed to provide a similar interface, while taking into account the nature of Tcl.  

C++/Tcl provides the following features:  

*   Support for both extending Tcl with C++ modules and embedding Tcl in C++ applications.
*   The ability to expose C++ functions as commands in Tcl with automatic argument/data type management and conversion.
*   The ability to define classes and class member functions and make them accessible from Tcl in a manner similar to [SWIG](http://www.swig.org/) wrappers.
*   The ability to manipulate Tcl lists and objects from C++.  

[Quick Start](quickstart.md)  

[Extending Tcl by C++ module](quickstart.md#extending)  
[Embedding Tcl in C++ program](quickstart.md#embedding)  

[Exposing Free Functions](freefun.md)  
[Exposing Classes](classes.md)  

[Member functions](classes.md#members)  
[Constructors](classes.md#constructors)  
[Destructors](classes.md#destructors)  

[Objects and Lists](objects.md)  
[Call Policies](callpolicies.md)  

[Factories and sinks](callpolicies.md#factories)  
[Variadic functions](callpolicies.md#variadic)  

[Threads](threads.md)  

[Various Goodies](goodies.md)  

[Package support](goodies.md#packages)  
[Stream evaluation](goodies.md#streameval)  
[Tcl Namespaces](goodies.md#namespaces)  
[Safe Tcl Interpreters](goodies.md#safe)  
[Aliasing](goodies.md#aliasing)  

[Errors](errors.md)  
[Compiling](compiling.md)  

***

Copyright © 2004-2006, Maciej Sobczak
***
Copyright © 2017-2019, FlightAware LLC
