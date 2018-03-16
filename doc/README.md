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

[Quick Start](quickstart.html)  

[Extending Tcl by C++ module](quickstart.html#extending)  
[Embedding Tcl in C++ program](quickstart.html#embedding)  

[Exposing Free Functions](freefun.html)  
[Exposing Classes](classes.html)  

[Member functions](classes.html#members)  
[Constructors](classes.html#constructors)  
[Destructors](classes.html#destructors)  

[Objects and Lists](objects.html)  
[Call Policies](callpolicies.html)  

[Factories and sinks](callpolicies.html#factories)  
[Variadic functions](callpolicies.html#variadic)  

[Various Goodies](goodies.html)  

[Package support](goodies.html#packages)  
[Stream evaluation](goodies.html#streameval)  
[Tcl Namespaces](goodies.html#namespaces)  
[Safe Tcl Interpreters](goodies.html#safe)  
[Aliasing](goodies.html#aliasing)  

[Errors](errors.html)  
[Compiling](compiling.html)  

***

Copyright © 2004-2006, Maciej Sobczak
***
Copyright © 2018 FlightAware LLC
