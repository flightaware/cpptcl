// example6.cc

#include "../cpptcl.h"
#include <iostream>

using namespace std;
using namespace Tcl;

int main()
{
     interpreter i;

     int numbers[] = {5, 7, 1, 6, 3, 9, 7};
     size_t elems = sizeof(numbers) / sizeof(int);

     object tab;
     for (size_t indx = 0; indx != elems; ++indx)
     {
          tab.append(i, object(numbers[indx]));
     }

     object cmd("lsort -integer");
     cmd.append(i, tab);

     // here, cmd contains the following:
     // lsort -integer {5 7 1 6 3 9 7}

     object result = i.eval(cmd);

     cout << "unsorted: ";
     for (size_t indx = 0; indx != elems; ++indx)
     {
          cout << numbers[indx] << ' ';
     }

     cout << "\n  sorted: ";
     elems = result.length(i);
     for (size_t indx = 0; indx != elems; ++indx)
     {
          object obj(result.at(i, indx));
          int val = obj.get<int>(i);

          cout << val << ' ';
     }
     cout << '\n';
}
