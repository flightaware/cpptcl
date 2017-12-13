//
// Copyright (C) 2004-2006, Maciej Sobczak
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#define CPPTCL_NO_TCL_STUBS
#include "cpptcl/cpptcl.h"
#include <iostream>
#include <sstream>
#include <assert.h>

using namespace Tcl;

std::string did;
int objNum = 0;

class C {
  public:
	C() : num_(++objNum) { did = "C::C()"; }
	C(int i, std::string const &s = std::string("default")) : num_(++objNum) {
		std::ostringstream ss;
		ss << "C::C(" << i << ", " << s << ")";
		did = ss.str();
	}

	~C() {
		std::ostringstream ss;
		ss << "C::~C() on " << num_;
		did = ss.str();
		--objNum;
	}

	void fun0() {
		std::ostringstream ss;
		ss << "C::fun0() on " << num_;
		did = ss.str();
	}

	int fun1(int a, int b) {
		std::ostringstream ss;
		ss << "adding on " << num_ << ": " << a << " + " << b;
		did = ss.str();

		return a + b;
	}

  private:
	int num_;
};

C *source() { return new C; }

void use(C *p) { p->fun0(); }

void sinkFun(C *p) { delete p; }

void test1() {
	Tcl_Interp * interp = Tcl_CreateInterp();
	interpreter i(interp, true);

	i.class_<C>("C", init<int, std::string const &>());
	i.class_<C>("C2", init<int>());
	i.class_<C>("C3", init<>());
	i.class_<C>("C4").def("add", &C::fun1);

	i.def("source", source, factory("C4"));
	i.def("use", use);
	i.def("sink", sinkFun, sink(1));

	i.eval("set p [source]");
	assert(did == "C::C()");
	assert(objNum == 1);

	int res = i.eval("$p add 2 3");
	assert(res == 5);
	assert(did == "adding on 1: 2 + 3");

	i.eval("use $p");
	assert(did == "C::fun0() on 1");

	i.eval("sink $p");
	assert(did == "C::~C() on 1");
	assert(objNum == 0);

	// now, the object and its command should not exist
	try {
		// exception expected
		i.eval("$p -delete");
		assert(false);
	} catch (...) {
	}

	i.eval("set p [C 7 \"Ala ma kota\"]");
	assert(did == "C::C(7, Ala ma kota)");
	assert(objNum == 1);

	i.eval("use $p");
	assert(did == "C::fun0() on 1");

	i.eval("set p2 [C2 7]");
	assert(did == "C::C(7, default)");
	assert(objNum == 2);

	i.eval("use $p2");
	assert(did == "C::fun0() on 2");

	i.eval("set p3 [C3]");
	assert(objNum == 3);

	i.eval("use $p3");
	assert(did == "C::fun0() on 3");

	i.eval("set p4 [C4]");
	assert(objNum == 4);

	i.eval("use $p4");
	assert(did == "C::fun0() on 4");

	res = i.eval("$p4 add 3 4");
	assert(res == 7);
	assert(did == "adding on 4: 3 + 4");

	try {
		// exception expected
		i.eval("$p4 add noint noint");
		assert(false);
	} catch (...) {
	}

	i.eval("$p -delete");
	assert(did == "C::~C() on 1");
	assert(objNum == 3);

	i.eval("$p2 -delete");
	assert(did == "C::~C() on 2");
	assert(objNum == 2);

	i.eval("$p3 -delete");
	assert(did == "C::~C() on 3");
	assert(objNum == 1);

	i.eval("$p4 -delete");
	assert(did == "C::~C() on 4");
	assert(objNum == 0);
}

int main() {
	try {
		test1();
	} catch (std::exception const &e) {
		std::cerr << "Error: " << e.what() << '\n';
	}
}
