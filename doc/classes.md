[[prev](freefun.md)][[top](index.md)][[next](objects.md)]  

#### Exposing Classes  

Exposing classes is a little more involved since more than just the name needs to be defined.

Let's suppose that we have the following C++ class:

```
class Person  
{  
public:  
     void setName(string const &n) { name = n; }  
     string getName() { return name; }  

private:  
     string name;  
};  
```

We can expose this class in an extension (or directly to the embedded interpreter) like this:

```
CPPTCL_MODULE(Mymodule, i)  
{  
     i.class_<Person>("Person")  
          .def("setName", &Person::setName)  
          .def("getName", &Person::getName);  
}  
```

Note: The "i" parameter to the CPPTCL_MODULE macro is a name that you later use to refer to the interpreter. You can choose whatever name you like. The rest of the code is the same as if the following object was declared in the C++ program:  

```
interpreter i;  
```

As you see, the class is defined using the class_ member function template, called on the interpreter object (it could not be named "class", since that is a reserved word in C++).

The string name that is provided as a parameter to the class_ function is a name that will be visible to the Tcl scripts; again, it does not have to be the name of the C++ class that you use in code.  

#### <a name="members"></a>Member functions

This special class_ function returns an object that can be used to define class member functions, in the same expression and in a chained way.  
The above example is written in three lines just to make it more readable, but it is a single C++ statement:  

```
i.class_<Person>("Person").def("setName", &Person::setName).def("getName", &Person::getName);  
```

The rules for defining class member functions are the same as the rules used for [Exposing Free Functions](freefun.md).  

After the class is exposed, it can be used in the following way (let's suppose that the above was compiled to the shared library named mymodule.so):  

```
% load ./mymodule.so  
% set p [Person]  
p0x807b790  
% $p setName "Maciej"  
% $p getName  
Maciej  
%  
```

As shown above, the Person command creates and returns a new Person object. This is done by invoking the constructor of the class.

It returns a name of the new object (here it is p0x807b790 - it may be different on your machine or when you run the same program twice; you should not attach any external meaning to this name) and this name is immediately available as a new command, to use with member functions.

(As noted earlier, this is very similar to the approach taken by swig. One advantage of this approach versus swig is that with cpptcl the debugger doesn't get confused across command boundaries.)

For example, the expression:

```
% $p setName "Maciej"  
```

calls the member function setName with the parameter "Maciej", on the object whose name is stored in the variable p.

Later, the expression:

```
% $p getName  
```

...calls the member function getName without any parameters and it returns the string result of that call.

#### <a name="constructors"></a>Constructors

In the above example, the class Person only has a contructor without parameters.

The Person class could be exposed also in the following way:

```
i.class_<Person>("Person", <span style="font-weight: bold;">init<>()</span>)  
     .def("setName", &Person::setName)  
     .def("getName", &Person::getName)<span style="font-family: sans-serif;">;</span>  
```

In the above example there is a special init object provided. It can carry information about the expected parameters that are needed for constructor, as in the following complete example:

```
// example4.cc  

#include "cpptcl.h"  
#include <string>  

using namespace std;  
using namespace Tcl;  

class Person  
{  
public:  
     <span style="font-weight: bold;">Person(string const &n) : name(n) {}</span>  

     void setName(string const &n) { name = n; }  
     string getName() { return name; }  

private:  
     string name;  
};  

CPPTCL_MODULE(Mymodule, i)  
{  
     i.class_<Person>("Person", <span style="font-weight: bold;">init<string const &>()</span>)  
          .def("setName", &Person::setName)  
          .def("getName", &Person::getName);  
}  
```

The init object can bring with it information about the number and the types of parameters needed by the constructor.

In this example it declares that the constructor of class Person requires one parameter of type string const &.

We no longer use this exposed class from Tcl as we did before:

```
% set p [Person]  
Too few arguments.  
%  
```

Instead we have to provide required parameters:  

```
% set p [Person "Maciej"]  
p0x807b7c0  
% $p getName  
Maciej  
%  
```

Constructors with up to nine parameters can be defined this way. (This limitation will be going away as we leverage stuff in newer versions of C++.)

Another form that may sometimes be useful is:

```
i.class_<Person>("Person", <span style="font-weight: bold;">no_init</span>)  
     .def("setName", &Person::setName)  
     .def("getName", &Person::getName);  
```

This basically means that the exposed class has no constructors at all (or, to be strict, we do not want them to be visible from Tcl).

Objects of such classes need to be created by separate factory functions.

#### <a name="destructors"></a>Destructors

There is additional "member function" that is automatically defined for each class, which is used for destroying the object:

```
% $p -delete  
```

Once this is done, the object itself is destroyed and the associated command is removed from the interpreter.

This means that the name stored in the variable p cannot be used for executing member functions any more:

```
% $p getName  
invalid command name "p0x807b790"  
%  
```

[[prev](freefun.md)][[top](index.md)][[next](objects.md)]  

* * *

Copyright © 2004-2006, Maciej Sobczak  

* * *

Copyright © 2017-2018, FlightAware LLC
