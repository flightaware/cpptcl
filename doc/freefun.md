[[prev](quickstart.md)][[top](README.md)][[next](classes.md)]  

#### Exposing Free Functions  

You have already seen an example of how to expose a free C++ function to the Tcl interpreter.

No matter whether you write an extension module or embed Tcl in a C++ application, the way to expose a free function is alway the same:

```
i.def("tcl_function_name", cpp_function_name);  
```

...where i is the name of the interpreter object.

In the above example, the "tcl_function_name" is a name that will be visible to Tcl scripts, and the cpp_function_name is a name of (or a pointer to) the function in the C++ code to implement it.

They do not have to be the same, although this is the usual practice, like in the [Quick Start](quickstart.md) examples:

```
i.def("hello", hello);  
```

Suppose that we want to expose the following C++ function:

```
int sum(int a, int b)  
{  
     return a + b;  
}  
```

We can define it in the interpreter like this:

```
i.def("<span style="font-weight: bold;">add</span>", sum);  
i.def("<span style="font-weight: bold;">+</span>", sum);  
```

so that we can call the sum function from Tcl like this:

```
add 3 4  
```

or:

```
+ 3 4  
```

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
*   [object](objects.md)  

In addition, the parameter of the function can be of type T const &, where T is any of the above.

This means that only <span style="font-style: italic;">input</span> parameters are supported (this may change in future versions of the libray).

[[prev](quickstart.md)][[top](README.md)][[next](classes.md)]  

* * *

Copyright © 2004-2006, Maciej Sobczak  

* * *

Copyright © 2017-2019, FlightAware LLC
