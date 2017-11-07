// example7.cc

#include "../cpptcl.h"

using namespace Tcl;

int sumAll(object const &argv)
{
     interpreter i(argv.get_interp(), false);

     int sum = 0;

     size_t argc = argv.length(i);
     for (size_t indx = 0; indx != argc; ++indx)
     {
          object o(argv.at(i, indx));
          sum += o.get<int>(i);
     }

     return sum;
}

CPPTCL_MODULE(Cpplib, i)
{
     i.def("sum", sumAll, variadic());
}
