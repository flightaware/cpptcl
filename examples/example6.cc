// example6.cc

#include <iostream>

#include "tcl.h"
#include "cpptcl/cpptcl.h"

using namespace std;
using namespace Tcl;

int main() {
	Tcl_Interp * interp = Tcl_CreateInterp();
	interpreter i(interp, true);

	int numbers[] = {5, 7, 1, 6, 3, 9, 7};
	size_t elems = sizeof(numbers) / sizeof(int);

	object tab;
	for (size_t indx = 0; indx != elems; ++indx) {
		tab.append(object(numbers[indx]));
	}

	object cmd("lsort -integer");
	cmd.append(tab);

	// here, cmd contains the following:
	// lsort -integer {5 7 1 6 3 9 7}

	object result = i.eval(cmd);

	cout << "unsorted: ";
	for (size_t indx = 0; indx != elems; ++indx) {
		cout << numbers[indx] << ' ';
	}

	cout << "\n  sorted: ";
	elems = result.size();
	for (size_t indx = 0; indx != elems; ++indx) {
		object obj(result.at(indx));
		int val = obj.get<int>();

		cout << val << ' ';
	}
	cout << '\n';
}
