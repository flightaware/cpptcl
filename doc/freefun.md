  
[[prev](quickstart.html)][[top](index.html)][[next](classes.html)]  

#### Exposing Free Functions  

You have already seen an example of how to expose a free C++ function to the Tcl interpreter.

No matter whether you write an extension module or embed Tcl in a C++ application, the way to expose a free function is alway the same:

<div style="margin-left: 40px;">`i.def("tcl_function_name", cpp_function_name);`  
</div>

...where `i` is the name of the interpreter object.

In the above example, the `"tcl_function_name"` is a name that will be visible to Tcl scripts, and the `cpp_function_name` is a name of (or a pointer to) the function in the C++ code to implement it.

They do not have to be the same, although this is the usual practice, like in the [Quick Start](quickstart.html) examples:

<div style="margin-left: 40px;">`i.def("hello", hello);`  
</div>

Suppose that we want to expose the following C++ function:

<div style="margin-left: 40px;">`int sum(int a, int b)  
{  
     return a + b;  
}`  
</div>

We can define it in the interpreter like this:

<div style="margin-left: 40px;">`i.def("<span style="font-weight: bold;">add</span>", sum);`  
`i.def("<span style="font-weight: bold;">+</span>", sum);`  
</div>

so that we can call the `sum` function from Tcl like this:

<div style="margin-left: 40px;">`add 3 4`  
</div>

or:

<div style="margin-left: 40px;">`+ 3 4`  
</div>

As you can see, the same function can be defined in the interpreter multiple times with different Tcl names.

If you create a Tcl command that's the same name as a command that already exists, the old command is replaced.

You probably noticed that the exposed functions can have parameters and can return values.

Functions with up to 9 parameters can be exposed.

At the moment, parameters and the return value of exposed functions can have the following types:

*   std::string, char const *  

*   int,
*   long,
*   bool,
*   double,
*   pointer to arbitrary type
*   [object](objects.html)  

In addition, the parameter of the function can be of type `T const &`, where T is any of the above.

This means that only <span style="font-style: italic;">input</span> parameters are supported (this may change in future versions of the libray).

[[prev](quickstart.html)][[top](index.html)][[next](classes.html)]  

* * *

Copyright © 2004-2006, Maciej Sobczak  

* * *

Copyright © 2017-2018, FlightAware LLC
