[[prev](classes.html)][[top](index.html)][[next](callpolicies.html)]  

#### Objects and Lists

Tcl objects are wrapped by full-fledged C++ objects in cpptcl. This wrapper provides all sorts of ways to manipulate Tcl objects that have a nice C++ feel while still letting you deal with Tcl stuff in a Tcl way, such as lists, variables, arrays, etc.

The class `object` provides the following members:

1\. Constructors

<div style="margin-left: 40px;">`object();  
explicit object(bool b);`  
`object(char const *buf, size_t size);`  
`explicit object(double b);`  
`explicit object(int i);`  

`template <class InputIterator>`  
`object(InputIterator first, InputIterator last);`  

`explicit object(long i);`  
`explicit object(char const *s);`  
`explicit object(std::string const &s);`  
</div>

The above constructors provides the means of creating Tcl objects from common C++ types.

The constructor accepting iterators is for the list creation. The provided iterator type should give either `object` or `Tcl_Obj*` type when dereferenced.

2\. Copy constructors

<div style="margin-left: 40px;">`explicit object(Tcl_Obj *o, bool shared = false);`  
`object(object const &other, bool shared = false);  
`</div>

If the `shared` flag is set, the newly created object wrapper will not duplicate the underlying Tcl object.

3\. Assignment-related members

<div style="margin-left: 40px;">`object & assign(bool b);`  
`object & resize(size_t size);                  // byte array resize`  
`object & assign(char const *buf, size_t size); // byte array assignment`  
`object & assign(double d);`  
`object & assign(int i);`  

`template <class InputIterator>`  
`object & assign(InputIterator first, InputIterator last);`  

`object & assign(long l);`  
`object & assign(char const *s);`  
`object & assign(std::string const &s);`  
`object & assign(object const &o);  
object & assign(Tcl_Obj *o);  
`  
`object & operator=(bool b);`  
`object & operator=(double d);`  
`object & operator=(int i);`  
`object & operator=(long l);`  
`object & operator=(char const *s);`  
`object & operator=(std::string const &s);`  

`object & operator=(object const &o);`  
`object & swap(object &other);`  
</div>

The `assign` member function accepting iterators is for the list assignment. The provided iterator type should give either `object` or `Tcl_Obj*` type when dereferenced.  

4\. Non-modifying accessors

<div style="margin-left: 40px;">`template <typename T>`  
`T get(interpreter &i = *interpreter::defaultInterpreter) const;`  

`char const * get() const;             // string get`  
`char const * get(size_t &size) const; // byte array get`  

`size_t size(interpreter &i = *interpreter::defaultInterpreter) const;  // returns list length`  
`object at(size_t index, interpreter &i = *interpreter::defaultInterpreter) const;`  

`Tcl_Obj * get_object() const { return obj_; }`  
</div>

The `get<T>` template is specialized for the following types:  

*   `bool`
*   `vector<char>` (for byte array queries)
*   `double`
*   `int`
*   `long`
*   `char const *`
*   `std::string`  

For ease of use in addition to the `get<T>` templates there are "asXXX()" chain methods for accessing values with type conversion: `asString(), asInt(), asBool(), asLong() and asDouble()`.  

5\. List-related modifiers  

<div style="margin-left: 40px;">`object & append(interpreter &i, object const &o);`  
`object & append_list(interpreter &i, object const &o);`  

`template <class InputIterator>`  
`object & replace(Interpreter &i, size_t index, size_t count,`  
`     InputIterator first, InputIterator last);`  

`object & replace(interpreter &i, size_t index, size_t count, object const &o);`  
`object & replace_list(interpreter &i, size_t index, size_t count, object const &o);`  
</div>

6\. Interpreter helpers  

All methods have a default interpreter parameter. TCL is a multiple interpreter platform, but many programs only use a single interpreter. The first created interpreter will be cached as the "default interpreter". This will be provided as a default parameter to get and set methods for object types.  

<div style="margin-left: 40px;">`void set_interp(Tcl_Interp *interp);`  
`Tcl_Interp * get_interp() const;`  
</div>

These functions may help to transmit the information about the "current" interpreter when the C++ function accepting `object` parameter is called from Tcl.

The `set_interp` function is automatically called by the underlying conversion logic, so that the C++ code can use the other function for accessing the interpreter.

This may be useful when the C++ code needs to invoke other functions that may change the interpreter state.

Note: If there is any need to extract the interpreter from the existing object, it may be helpful to wrap the resulting raw pointer into the `interpreter` object, which will not disrupt its normal lifetime:

<div style="margin-left: 40px;">`interpreter i(o.get_interp(), false);`  
</div>

The second parameter given to the `interpreter` constructor means that the newly created `i` object will not claim ownership to the pointer received from `get_interp()`.

In other words, the destructor of the object `i` will not free the actual interpreter.

<span style="font-weight: bold;">Example:</span>  

The following complete program creates the list of numbers, sorts it using the Tcl interpreter and prints the results on the console (note: this is not the most efficient way to sort numbers in C++!):

<div style="margin-left: 40px;">  
`#include "cpptcl/cpptcl.h"`  
`#include <iostream>`  

`using namespace std;`  
`using namespace Tcl;`  

`int main()`  
`{`  
`     interpreter i;`  

`     int numbers[] = {5, 7, 1, 6, 3, 9, 7};`  
`     size_t elems = sizeof(numbers) / sizeof(int);`  

`     object tab;`  
`     for (size_t indx = 0; indx != elems; ++indx)`  
`     {`  
`          tab.append(i, object(numbers[indx]));`  
`     }`  

`     object cmd("lsort -integer");`  
`     cmd.append(i, tab);`  

`     // here, cmd contains the following:`  
`     // lsort -integer {5 7 1 6 3 9 7}`  

`     object result = i.eval(cmd);`  

`     cout << "unsorted: ";`  
`     for (size_t indx = 0; indx != elems; ++indx)`  
`     {`  
`          cout << numbers[indx] << ' ';`  
`     }`  

`     cout << "\n  sorted: ";`  
`     elems = result.size();`  
`     for (size_t indx = 0; indx != elems; ++indx)`  
`     {`  
`          object obj(result.at(indx));`  
`          int val = obj.get<int>(); // or obj.asInt()`  

`          cout << val << ' ';`  
`     }`  
`     cout << '\n';`  
`}`  
</div>

When this program is run, it gives the following output:  

<div style="margin-left: 40px;">`$ ./example6`  
`unsorted: 5 7 1 6 3 9 7`  
`  sorted: 1 3 5 6 7 7 9`  
`<div style="margin-left: 40px;"  
</div>

In this example, an empty `tab` object is created and all numbers are appended to it to form a Tcl list of numbers.

After that, the sorting command is composed and executed (as you see, the `object` can be passed for evaluation).

The result of the command is retrieved also in the form of object wrapper, which is used to decompose the resulting list into its elements.

[[prev](classes.html)][[top](index.html)][[next](callpolicies.html)]  

* * *

Copyright © 2004-2006, Maciej Sobczak  

* * *

Copyright © 2017-2018, FlightAware LLC
