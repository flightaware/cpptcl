[[prev](callpolicies.md)][[top](README.md)][[next](errors.md)]  

#### Various Goodies  

This section of the documentation covers a couple of additional important features.  

#### <a name="packages"></a>Package support

It is possible to define a package name and version for the extension. Use the pkg_provide member function when defining a loadable module:  

```
CPPTCL_MODULE(Mymodule, i)  
{  
    i.pkg_provide("MyPackage", "3.4");  
    // ...  
}  
```

After loading this module inside the Tcl interpreter, the following command shows that the package is indeed provided:  

```
% package provide MyPackage  
3.4  
%  
```

#### <a name="streameval"></a>Stream evaluation

It is possible to evaluate the contents of the input stream. One of the possible uses is the evaluation of the whole file:  

```
ifstream f("somescript.tcl");  
i.eval(f);  
```

Any object compatible with std::istream is accepted.  

#### <a name="namespaces"></a>Tcl Namespaces

Tcl supports namespaces. It is possible to define classes and functions within their own namespace by just prefixing the name with the namespace name:  

```
i.def("NS::fun", fun);  
```

The above example defines a new function fun, which will be visible in the NS namespace.  
The problem is that the Tcl namespace needs to be created before any name is added to it. This can be done by simple script evaluation.  
Example:  

```
CPPTCL_MODULE(Mymodule, i)  
{  
     i.eval("namespace eval NS {}");  
     i.def("NS::fun", fun);  
}  
```

#### <a name="safe"></a>Safe Tcl Interpreters

Each Tcl interpreter can be turned into a <span style="font-style: italic;">safe</span> interpreter, where dangerous commands (which are commands related to files, sockets, process control, etc.) cannot be called.  
This feature may be very useful when user-provided scripts are to be evaluated in the embedded Tcl interpreter.  
In order to make a safe interpreter, do this:  

```
i.make_safe();  
```

Once the interpreter is made safe, it cannot be put back to its normal form.  

#### <a name="aliasing"></a>Aliasing

Aliasing is a powerful feature of Tcl. It allows to define <span style="font-style: italic;">links</span> between two interpreters, so that when one command is invoked in one interpreter, it is just forwarded and executed within the second interpreter. This means that the given command will have access to the other interpreter's state (its commands, variables, etc.).  
This feature is useful when a "firewall" interpreter is created that provides access to only selected set of commands defined in another interpreter.  
In order to make an alias from interpreter i1 to interpreter i2, do this:  

```
i1.create_alias("fun", i2, "otherFun");  
```

The above instruction creates an alias in the interpreter i1, so that whenever the fun command is invoked in it, it will forward the call to the second interpreter, to the otherFun command.  

#### <a name="usage">Usage messages</a>

Normally when you provide too few arguments to a command, you get an uninformative error message `Too few arguments.`. You can provide, alternatively,
a usage() parameter to def():

```
using namespace Tcl;
//...

i.def("slick_search", slick_search, usage("slick_search boxset lat lon"));
```

Now if you call "slick_search" with too few parameters, you will get the error "Usage: slick_search boxset lat lon".

This is implemented as a policy function, and does not interfere with other policy arguments:
```
using namespace Tcl;
//...

i.def("slick_close", slick_close, sink(1).usage("slick_close handle"));
```

[[prev](callpolicies.md)][[top](README.md)][[next](errors.md)]  

* * *

Copyright © 2004-2006, Maciej Sobczak
