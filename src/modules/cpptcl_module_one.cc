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

void helloProc()
{
    Bind<int, int> expr("expr");
    Bind<double, double, double> pow("pow");

    int v = pow(2, 5);
    cout
        << "pow(2,5) == " << v
        << " pow(5,2) = " << pow(5, 2)
        << " expr {1} = " << expr(1)
        << endl;
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
    i.def("helloProc", helloProc);
    i.def("helloVar", helloVar);
    i.def("helloArray", helloArray);
    i.def("helloArray2", helloArray2);
}
