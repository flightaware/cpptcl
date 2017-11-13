// example2.cc

#include "cpptcl.h"
#include <iostream>
#include <string>

using namespace std;
using namespace Tcl;

void hello() { cout << "Hello C++/Tcl!" << endl; }

int main() {
	interpreter i;
	i.def("hello", hello);

	string script = "for {set i 0} {$i != 4} {incr i} { hello }";

	i.eval(script);
}
