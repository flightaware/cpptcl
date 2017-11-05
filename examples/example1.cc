// example1.cc

#include "cpptcl.h"
#include <iostream>

using namespace std;

void hello()
{
     cout << "Hello C++/Tcl!" << endl;
}

CPPTCL_MODULE(Mymodule, i)
{
     i.def("hello", hello);
}
