#include <iostream>

#define CPPTCL_NO_TCL_STUBS
#include "cpptcl/cpptcl.h"
#include "tcl.h"

using namespace std;

int main() {
	Tcl_Interp *interp = Tcl_CreateInterp();
	Tcl::interpreter i(interp, true);
	i.eval("puts [info var]");

	Tcl::object tcl_version = i.getVar("tcl_version");
	cout << "tcl version " << tcl_version.asString() << endl;

	cout << "tcl_version exists 1 == " << i.exists("tcl_version") << endl;
	cout << "env(HOME) exists 1 == " << i.exists("env(HOME)") << endl;
	cout << "env HOME exists 1 == " << i.exists("env", "HOME") << endl;
	cout << "env NOT_HOME exists 0 == " << i.exists("env", "NOT_HOME") << endl;
	cout << "tikle exists 0 == " << i.exists("tikle") << endl;

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

	Tcl::object(99.0).bind("env", "ninety-nine");
	i.eval("puts \"bound array value to $env(ninety-nine)\"");

	i.eval("proc string1 {arg} { return $arg }");
	Tcl::Bind<string, string> string1("string1");
	string val = string1("arg1 arg2");
	cout << "string1 arg1 arg2 = " << val << endl;

	i.eval(R"(proc string2 {arg1 arg2} { return "$arg1+$arg2" })");
	Tcl::Bind<string, string, string> string2("string2");
	string val2 = string2("arg1.1 arg1.2", "arg2.1 arg2.2");
	cout << "string2 \"arg1.1 arg1.2\" \"arg2.1 arg2.2\" = " << val2 << endl;
}
