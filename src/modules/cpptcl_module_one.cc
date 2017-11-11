#include "cpptcl.h"
#include <iostream>

using namespace std;
using namespace Tcl;

void hello();
void helloVar(objectref const &first, objectref const &last);
void helloArray(object const &name);
void helloArray2(object const &name, object const &address);

void hello()
{
     cout << "Hello C++/Tcl!" << endl;
}

void helloVar(objectref const &first, objectref const &last)
{
    cout << "Hello C++/Tcl! "
        << first.get().get() << " "
        << last.get().get() << endl;
}

void helloArray(object const &name)
{
    cout << "Hello C++/Tcl! from array "
    << name["first"].get() << " "
    << name["last"].get() << endl;
}

void helloArray2(object const &name, object const &address)
{
    cout << "Hello C++/Tcl! from array "
    << name["first"].get() << " "
    << name["last"].get() << endl;
	cout << "city exists " << address.exists("city") << endl;
    cout << address["city"].get() << " " << address["state"].get() << endl;
}

CPPTCL_MODULE(Cpptcl_module_one, i)
{
    i.def("hello", hello);
    i.def("helloVar", helloVar);
    i.def("helloArray", helloArray);
    i.def("helloArray2", helloArray2);
}
