[[top](README.md)][[next](freefun.md)]  

#### Quick Start

C++/Tcl makes it easy to interface between C++ and Tcl.  
You can both extend the Tcl interpreter in C++ and embed a Tcl interpreter into a regular C++ program.  

#### <a name="extending"></a>Hello World (extending Tcl)  

Let's consider the following C++ file:
```
// example1.cc
#include "cpptcl.h"
#include <iostream>

using namespace std;

void hello()  
{  
     cout << "Hello C++/Tcl!" << endl;  
}  

CPPTCL_MODULE(Mymodule, i)  
{  
     i.def("hello", hello);
}  
```

After compiling (let's suppose that the resulting shared library is named `mymodule.so`), we can do this:  

```$ tclsh  
% load ./mymodule.so  
% hello
Hello C++/Tcl!  
% for {set i 0} {$i != 4} {incr i} { hello }  
Hello C++/Tcl!  
Hello C++/Tcl!  
Hello C++/Tcl!  
Hello C++/Tcl!  
%  
```

In other words, the Tcl interpreter was <span style="font-style: italic;">extended</span> with the loadable module (which is a shared library) that provides the definition of new command.  

#### <a name="embedding"></a>Hello World (embedding Tcl)  

Let's take the following C++ program:  

```
// example2.cc  

#include "cpptcl.h"  
#include <iostream>  
#include <string>  

using namespace std;  
using namespace Tcl;  

void hello()  
{  
     cout << "Hello C++/Tcl!" << endl;  
}  

int main()  
{  
     interpreter i;  
     i.def("hello", hello);  

     string script = "for {set i 0} {$i != 4} {incr i} { hello }";  

     i.eval(script);  
}  
```

After compiling, it gives the following result:  

```$ ./example2  
Hello C++/Tcl!  
Hello C++/Tcl!  
Hello C++/Tcl!  
Hello C++/Tcl!  
$  
```

Our C++ program has started up like any other C/C++ program, and the main is been invoked, i.e. Tcl didn't grab the main() and do its own thing; our normal main ran.

In our main routine we created a Tcl interpreter as a C++ object and manipulated it to create a new Tcl command and evaluate Tcl code to do things with that command.

Pretty cool.

[[top](README.md)][[next](freefun.md)]  

* * *

Copyright © 2004-2006, Maciej Sobczak

* * *
Copyright © 2018, FlightAware LLC
