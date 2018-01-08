#include <iostream>

#define CPPTCL_NO_TCL_STUBS
#include "tcl.h"
#include "cpptcl/cpptcl.h"

using namespace std;

int main()
{
	Tcl_Interp* interp = Tcl_CreateInterp();
	Tcl::interpreter i(interp, true);
	i.eval("puts [info var]");
    
    Tcl::object tcl_version = i.getVar("tcl_version");
    cout << "tcl version " << tcl_version.asString() << endl;
    
    Tcl::object env = i.getVar("env", "HOME");
    cout << "env(HOME) = " << env.asString() << endl;
    
    Tcl::object newstring("a new string");
    newstring.bind("x");
    // Set a variable to anonymous object
    Tcl::object(42).bind("y");
    // What to the variables look like?
    i.eval("puts \"tcl x = $x y = $y\"");
    
    // Binding a variable in a specific interpreter requires creation of the object
    // first then set the interpreter of the object.
    Tcl::object interp_var("something");
    interp_var.set_interp(interp);
    interp_var.bind("var1");
}
