//
// Copyright (C) 2004-2006, Maciej Sobczak
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#include "cpptcl/cpptcl.h"
#include <iostream>
#include <string>

void fun0() { std::cout << "fun0\n"; }

int fun1() { return 7; }

std::string fun2() { return "Ala ma kota"; }

int add(int a, int b) { return a + b; }

int mystrlen(std::string const &s) { return s.size(); }

CPPTCL_MODULE(Test, i) {
	i.def("fun0", fun0);
	i.def("fun1", fun1);
	i.def("fun2", fun2);
	i.def("add", add);
	i.def("mystrlen", mystrlen);
}
