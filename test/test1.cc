//
// Copyright (C) 2004-2006, Maciej Sobczak
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#include "cpptcl.h"
#include <iostream>

using namespace Tcl;

void fun0() {}
int fun1() { return 8; }
int fun2(int i) { return i + 2; }
int fun3(int const &i) { return i + 2; }
int fun4(std::string const &s) { return s.size(); }
int fun5(char const *s) { return std::string(s).size(); }

void test1() {
	interpreter i;
	i.make_safe();

	std::string s = i.eval("return \"ala ma kota\"");
	assert(s == "ala ma kota");

	i.eval("proc myproc {} { return 7 }");
	s = static_cast<std::string>(i.eval("myproc"));
	assert(s == "7");

	int ival = i.eval("myproc");
	assert(ival == 7);
	double dval = i.eval("myproc");
	assert(dval == 7.0);

	i.def("fun0", fun0);
	i.eval("fun0");

	i.def("fun1", fun1);
	ival = i.eval("fun1");
	assert(ival == 8);

	i.def("fun2", fun2);
	ival = i.eval("fun2 7");
	assert(ival == 9);

	i.def("fun3", fun3);
	ival = i.eval("fun3 7");
	assert(ival == 9);

	i.def("fun4", fun4);
	ival = i.eval("fun4 \"ala ma kota\"");
	assert(ival == 11);

	i.def("fun5", fun5);
	ival = i.eval("fun5 \"ala ma kotka\"");
	assert(ival == 12);

	try {
		i.eval("fun2 notaninteger");
		assert(false);
	} catch (std::exception const &) {
	}
}

int main() {
	try {
		test1();
	} catch (std::exception const &e) {
		std::cerr << "Error: " << e.what() << '\n';
	}
}
