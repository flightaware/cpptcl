#include <iostream>

#define CPPTCL_NO_TCL_STUBS
#include "tcl.h"
#include "cpptcl/cpptcl.h"

int main()
{
	Tcl_Interp* interp = Tcl_CreateInterp();
	Tcl::interpreter i(interp, true);
	i.eval("puts \"hello\"");
}
