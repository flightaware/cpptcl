[[prev](callpolicies.md)][[top](README.md)][[next](goodies.md)]  

#### Threads

Warning: cpptcl *does not work* with the TCL thread package.

The TCL interpreter is usually compiled with thread support by default on linux.
TCL uses two threads.
One thread is the main thread.
The other thread is the notifier which wakes up the main thread from the select system call.
Users can add more threads with the TCL threads package.

##### TCL Thread package

See https://www.tcl-lang.org/man/tcl/ThreadCmd/thread.htm for the documentation on the TCL thread package.

The important point is that every TCL thread has a separate TCL interpreter.
This interpreter is created in the TCL thread package with Tcl_CreateInterp C API.
There is no API access to the interpreter pointer in the thread created by the TCL thread package.
There is also no API to access the main Tcl_Interp *.

#### cpptcl does not work with the TCL thread package

##### cpptcl Interp wraps the Tcl_Interp * pointer

cpptcl wraps the Tcl_Interp * pointer.
One interpreter is cached in cpptcl to avoid having to pass it to every method.
For example, the get method can be passed an interpreter, but a default value is provided.
```
namespace Tcl {
class object {
...
template <typename T> T get(interpreter &i = *interpreter::defaultInterpreter) const;

```
If you created the interpreters, then one can pass the specific interpreter as needed.

Since TCL thread creates and hides the pointers to the interpreters, changes are required in TCL thread to fix this.

##### Mixing interpreter allocations will cause errors or worse

You cannot mix allocation with free in TCL interpreters.
Best case is that you get an error on Tcl_Free* on call ordering.
Worse case is memory is corrupted.
