// example2.cc

#include <iostream>
#include <string>


#include "cpptcl/cpptcl.h"

using namespace std;
using namespace Tcl;

void hello() { cout << "Hello C++/Tcl!" << endl; }

int main() {
	Tcl_Interp * interp = Tcl_CreateInterpWithStubs("8.6", 0);
	interpreter i(interp, true);
	i.def("hello", hello);

	string script = "for {set i 0} {$i != 4} {incr i} { hello }";

	i.eval(script);
}
