#include "cpptcl.h"
#include <iostream>

using namespace std;

void hello()
{
     cout << "Hello C++/Tcl!" << endl;
}

CPPTCL_MODULE(Cpptcl_module_one, i)
{
     i.def("hello", hello);
}
