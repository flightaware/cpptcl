//
// Copyright (C) 2004-2006, Maciej Sobczak
// Copyright (C) 2017-2019, FlightAware LLC
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#define CPPTCL_NO_TCL_STUBS
#include "../cpptcl/cpptcl.h"
#include <iostream>
#undef NDEBUG
#include <assert.h>

using namespace Tcl;

int funv1(int, object const &o) {
	return o.size();
}

class C {
  public:
	C() {}

	C(int, object const &o) {
		len_ = o.size();
	}

	int getLen() const { return len_; }

	int len(int, object const &o) {
		return o.size();
	}

	int lenc(int, object const &o) const {
		interpreter i(o.get_interp(), false);
		return o.size(i);
	}

  private:
	int len_;
};

void test1() {
	Tcl_Interp * interp = Tcl_CreateInterp();
	interpreter i(interp, true);
	i.def("fun1", funv1, usage("Needs a usage message."));

	try {
		i.eval("fun1 1");
		assert(false);
	} catch (tcl_error const &e) {
		assert(e.what() == std::string("too few arguments: 1 given, 2 required"));//std::string("Usage: Needs a usage message."));
	}
}

int main() {
	try {
		test1();
	} catch (std::exception const &e) {
		std::cerr << "Error: " << e.what() << '\n';
		exit(-1);
	}
}
