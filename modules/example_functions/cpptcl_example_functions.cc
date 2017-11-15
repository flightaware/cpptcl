#include <iostream>

#include "cpptcl/cpptcl.h"

using namespace std;
using namespace Tcl;

void hello();
void helloVar(upvar const &first, upvar const &last);
void helloArray(object const &name);
void helloArray2(object const &name, object const &address);

void hello() { cout << "Hello C++/Tcl!" << endl; }

void helloProc() {
	Bind<int, int> expr("expr");
	Bind<double, double, double> pow("pow");

	int v = pow(2, 5);
	cout << "pow(2,5) == " << v << " pow(5,2) = " << pow(5, 2) << " expr {1} = " << expr(1) << endl;
}

void helloVar(upvar const &first, upvar const &last) { cout << "Hello C++/Tcl! " << first.get().get() << " " << last.get().get() << endl; }

void helloArray(object const &name) { cout << "Hello C++/Tcl! from array " << name("first").get() << " " << name("last").get() << endl; }

void helloArray2(object const &name, object const &address) {

	cout << "Hello C++/Tcl! from array " << name("first").get() << " " << name("last").get() << endl;
	cout << "city exists " << address.exists("city") << endl;
	cout << "city " << address("city").get() << endl;

	std::string state("state");
	// Check for exists with if
        if (/* info exists */ address(state)) {
		// C++ does not allow $ to be an operator, so use *
		cout << "state " << *address(state) << endl;
	}

	// TODO
	// address(state).set("TX");

	cout << "exists zip? " << address.exists("zip") << endl;
	cout << "exception stack expected" << endl;
	std::string zip = (*address("zip")).to_string();
	cout << zip << endl;

}

CPPTCL_MODULE(Cpptcl_example_functions, i) {
	i.pkg_provide("cpptcl_example_functions", CPPTCL_EXAMPLE_FUNCTIONS_VERSION);
	i.def("hello", hello);
	i.def("helloProc", helloProc);
	i.def("helloVar", helloVar);
	i.def("helloArray", helloArray);
	i.def("helloArray2", helloArray2);
}
