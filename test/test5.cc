//
// Copyright (C) 2004-2006, Maciej Sobczak
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#define CPPTCL_NO_TCL_STUBS
#include "../cpptcl/cpptcl.h"
#include <iostream>
#include <assert.h>

using namespace Tcl;

int fun() { return 7; }

void test1() {
	Tcl_Interp * interp = Tcl_CreateInterp();
	interpreter i(interp, true);

	i.eval("namespace eval NS {}");
	i.def("NS::fun", fun);

	int ret = i.eval("NS::fun");
	assert(ret == 7);
}

void test2() {
	Tcl_Interp * interp = Tcl_CreateInterp();
	interpreter i1(interp, true), i2(interp, false);       

	i1.def("fun", fun);
	i2.create_alias("fun2", i1, "fun");

	int ret = i2.eval("fun2");
	assert(ret == 7);
}

int main() {
	try {
		test1();
		test2();
	} catch (std::exception const &e) {
		std::cerr << "Error: " << e.what() << '\n';
	}
}
