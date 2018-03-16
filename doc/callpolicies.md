[[prev](objects.html)][[top](index.html)][[next](goodies.html)]  

#### Call Policies

<span style="font-weight: bold;"><a name="factories"></a>Factories and sinks</span>  

C++ has extremely rich type system. You can see this when you expose some free function or member function:  

<div style="margin-left: 40px;">`i.def("fun", fun);`  
</div>

The name of the function, `fun`, is enough for the C++/Tcl libray to discover the following information:

*   Whether the function returns anything and what exactly.
*   Whether it has any parameters, how many of them and what are their types.

Sadly, these two pieces of information are sometimes not enough. Consider the following C++ functions:  

<div style="margin-left: 40px;">`SomeClass * produce()`  
`{`  
`     return new SomeClass();`  
`}`  

`void consume(SomeClass *p)`  
`{`  
`     // ...`  
`     delete p;`  
`}`  
</div>

These two functions can be exposed this way:  

<div style="margin-left: 40px;">`i.def("produce", produce);`  
`i.def("consume", consume);`  
</div>

After doing so, you can safely use them like here:  

<div style="margin-left: 40px;">`% set object [produce]`  
`p0x807b790`  
`% consume $object`  
</div>

The problem is that the `produce` command will not register any new Tcl command that would be responsible for managing member function calls or explicit object destruction, because it has no idea to which class definition (in the Tcl sense, not the C++ sense) the returned object belongs. In other words, the object name that is returned by the `produce` command has no meaning to the Tcl interpreter.

To solve this problem, some additional information has to be provided while exposing the `produce` function:  

<div style="margin-left: 40px;">`i.def("produce", produce, factory("SomeClass"));`  
</div>

As you see, there is additional parameter that provides hints to the C++/Tcl library. Such hints are called <span style="font-style: italic;">policies</span>. The `factory` policy above provides the following information:

*   The objects returned by the `produce` function are created with the `new` expression, or by other means. In any case the pointer is returned.  

*   These objects are meant to be handled as if they were created by a regular constructor.

*   In particular, a new command has to be registered for each object returned by this function.
*   This command should work as defined for the `"SomeClass"` Tcl class.  

What about the consume function?

There is also the issue that, normally, objects are destroyed by explicit calls to their `-delete` method. This call does two things:

*   It `delete`s the objects (in the C++ sense), and
*   it unregisters the Tcl command associated with that object.

We already know that the `consume()` function does the former. It is important also to do latter, otherwise the Tcl interpreter will be polluted with commands associated with non-existent objects (any call to such a command will result in undefined behaviour, which usually means a program crash).

In other words, we have to provide a hint to the C++/Tcl library that the `consume()` function is a <span style="font-style: italic;">sink</span> for objects:  

<div style="margin-left: 40px;">`i.def("consume", consume, sink(1));`  
</div>

The `sink` policy above says:  

*   The `consume` function takes responsibility for objects that are passed by its first (1) parameter.
*   The object will be destroyed (or otherwise managed).
*   The Tcl command associated with this object should be unregistered.

The following is a complete example with the use of policies:  

<div style="margin-left: 40px;">`// example5.cc`  

`#include "cpptcl.h"`  
`#include <string>`  

`using namespace std;`  
`using namespace Tcl;`  

`class Person`  
`{`  
`public:`  
`     Person(string const &n) : name(n) {}`  

`     void setName(string const &n) { name = n; }`  
`     string getName() { return name; }`  

`private:`  
`     string name;`  
`};`  

`// this is a factory function`  
`Person * makePerson(string const &name)`  
`{`  
`     return new Person(name);`  
`}`  

`// this is a sink function`  
`void killPerson(Person *p)`  
`{`  
`     delete p;`  
`}`  

`CPPTCL_MODULE(Mymodule, i)`  
`{`  
`     // note that the Person class is exposed without any constructor`  
`     i.class_<Person>("Person", <span style="font-weight: bold;">no_init</span>)`  
`          .def("setName", &Person::setName)`  
`          .def("getName", &Person::getName);`  

`     i.def("makePerson", makePerson, <span style="font-weight: bold;">factory("Person")</span>);`  
`     i.def("killPerson", killPerson, <span style="font-weight: bold;">sink(1)</span>);`  
`}`  
</div>

Now, let's try to exercise some interactive session with this:  

<div style="margin-left: 40px;">`% load ./mymodule.so`  
`% set p [Person "Maciej"]`  
`invalid command name "Person"`  
`%`  
</div>

As you see, the Tcl class `Person` has no constructor (this is the effect of providing the `no_init` policy when the class was exposed).  
Let's try another way:  

<div style="margin-left: 40px;">`% set p [makePerson "John"]`  
`p0x807b810`  
`% $p getName`  
`John`  
`%`  
</div>

It works fine. Now:  

<div style="margin-left: 40px;">`% killPerson $p`  
`% $p getName`  
`invalid command name "p0x807b810"`  
`%`  
</div>

Interestingly, a single function may need to combine several policies at once:  

<div style="margin-left: 40px;">`ClassA * create(int i, ClassB *p1, string const &s, ClassC *p2)`  
`{`  
`     // consume both p1 and p2 pointers`  
`     // and create new object:`  
`     return new ClassA();`  
`}`  
</div>

This function needs to combine three policies:  

1.  The `factory` policy, because it returns new objects.
2.  The `sink` policy on its second parameter.
3.  The `sink` policy on its fourth parameter.  

The following definition does it:  

<div style="margin-left: 40px;">`i.def("create", create, factory("ClassA").sink(2).sink(4));`  
</div>

As you see, policies can be chained. Their order is not important, apart from the fact that the `factory` policy can be related to only one class, so it is the last `factory` policy in the chain that is effective.  

<span style="font-weight: bold;"><a name="variadic"></a>Variadic functions</span>

Another thing that can be controlled from policies is whether the function is variadic (whether it can accept variable number of arguments) or not.

This feature is supported for:

*   free functions
*   constructors
*   class methods  

In order for the function (or constructor or method) to be variadic, <span style="font-style: italic;">both</span> of the following conditions must be true:

1.  The type of last (or the only) parameter must be `object const &  
    `
2.  The `variadic()` policy must be provided when defining that function.

The last formal parameter is of the generic `object` type and it allows to retrieve both single values and lists.

An example may be of help:

<div style="margin-left: 40px;">`void fun(int a, int b, <span style="font-weight: bold;">object const &c</span>);  
`

`// later:  
``i.def("fun", fun);`  

</div>

The fun() function above is defined in the interpreter as having <span style="font-style: italic;">exactly</span> three parameters.

If more arguments are provided, they are discarded. If less, an error is reported.

Now, consider the following change:

<div style="margin-left: 40px;">`void fun(int a, int b, <span style="font-weight: bold;">object const &c</span>);  

// later:  
``i.def("fun", fun, <span style="font-weight: bold;">variadic()</span>);`  
</div>

With the `variadic()` policy, the function will accept:  

*   two integer arguments - in that case, the `c` parameter will be an empty object
*   three arguments - in that case, `c` will have the value of the third argument
*   more - `c` will be a list of all arguments except the first two integer values.

Similarly, a constructor with variable list of parameters may be defined as:  

<div style="margin-left: 40px;">`class MyClass`  
`{`  
`public:`  
`    MyClass(int a, <span style="font-weight: bold;">object const &b</span>);`  
`    void fun();`  
`};`  

`// later:`  
`i.class_<MyClass>("MyClass", init<int, <span style="font-weight: bold;">object const &</span>>(), <span style="font-weight: bold;">variadic()</span>)`  
`    .def("fun", &MyClass::fun);`  
</div>

Note that the last parameter in the constructor has type `object const &` and that the `variadic()` policy was provided.

This constructor will accept 1, 2 or more arguments.

Of course, the variadic class methods can be defined in the same way:

<div style="margin-left: 40px;">`class MyClass`  
`{`  
`public:`  
`    void fun(<span style="font-weight: bold;">object const &a</span>);`  
`};`  

`// later:`  
`i.class_<MyClass>("MyClass")`  
`    .def("fun", &MyClass::fun, <span style="font-weight: bold;">variadic()</span>);`  
</div>

As you see, the special, last parameter may be the only one, or preceded by any number of "normal" parameters.

The possibility to define a variadic function is currently the only way to write commands that accept more than 9 arguments.

The `variadic()` policy may be combined (in any order) with other policies.

The following is a full example of module that defines one variadic function:

<div style="margin-left: 40px;">`#include "cpptcl.h"`  

`using namespace Tcl;`  

`int sumAll(object const &argv)`  
`{`  
`     interpreter i(argv.get_interp(), false);`  

`     int sum = 0;`  

`     size_t argc = argv.length(i);`  
`     for (size_t indx = 0; indx != argc; ++indx)`  
`     {`  
`          object o(argv.at(i, indx));`  
`          sum += o.get<int>(i);`  
`     }`  

`     return sum;`  
`}`  

`CPPTCL_MODULE(Mymodule, i)`  
`{`  
`     i.def("sum", sumAll, variadic());`  
`}`  
</div>

This module can be used from the Tcl interpreter like here:

<div style="margin-left: 40px;">`% load ./mymodule.so`  
`% sum`  
`0`  
`% sum 5`  
`5`  
`% sum 5 6 7`  
`18`  
`%`  
</div>

[[prev](objects.html)][[top](index.html)][[next](goodies.html)]  

* * *

Copyright © 2004-2006, Maciej Sobczak  

* * *

Copyright © 2017-2018, FlightAware LLC
