#include "cpptcl.h"
#include <iostream>

using namespace std;
using namespace Tcl;

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

CPPTCL_MODULE(Cpptcl_module_one, i)
{
    i.def("hello", hello);
    i.def("helloVar", helloVar);
}
