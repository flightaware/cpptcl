// example7.cc

#include "../cpptcl.h"

using namespace Tcl;

int sumAll(object const &argv)
{
     int sum = 0;

     size_t argc = argv.size();
     for (size_t indx = 0; indx != argc; ++indx)
     {
          object o(argv.at(indx));
          sum += o.get<int>();
     }

     return sum;
}

CPPTCL_MODULE(Cpplib, i)
{
     i.def("sum", sumAll, variadic());
}
