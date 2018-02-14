//
// Copyright (C) 2004-2006, Maciej Sobczak
// Copyright (C) 2017-2018, FlightAware LLC
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#define CPPTCL_NO_TCL_STUBS
#include "../cpptcl/cpptcl.h"
#include <cmath>
#include <iostream>
#include <assert.h>
#include <string.h>

using namespace Tcl;

object fun(object const &o) {
	return object(static_cast<int>(o.size()));
}

void test1() {
	char buf[] = {1, 2, 3, 4};
	Tcl_Interp * interp = Tcl_CreateInterp();
	interpreter i(interp, true);

	{
		object o(true);
		assert(o.get<bool>() == true);
		assert(object(o).get<bool>() == true);
		o = false;
		assert(o.get<bool>() == false);
		assert(object(o).get<bool>() == false);

		o = 123;
		assert(o.get<int>() == 123);
	}
	{
		object o(buf, 4);
		size_t size;
		char const *b = o.get(size);
		assert(size == 4);
		assert(memcmp(b, buf, 4) == 0);
		o.resize(2);
		b = o.get(size);
		assert(size == 2);
		assert(memcmp(b, buf, 2) == 0);

		object o2(o);
		b = o2.get(size);
		assert(size == 2);
		assert(memcmp(b, buf, 2) == 0);

		o = 123;
		assert(o.get<int>() == 123);
	}
	{
		object o(3.14);
		assert(std::fabs(o.get<double>() - 3.14) < 0.001);
		object o2(o);
		assert(std::fabs(o2.get<double>() - 3.14) < 0.001);

		o = "Ala ma kota";
		assert(std::string(o.get()) == "Ala ma kota");
	}
	{
		object o(1234);
		assert(o.get<int>() == 1234);
		object o2(o);
		assert(o2.get<int>() == 1234);

		o = 123;
		assert(o.get<int>() == 123);
	}
	{
		object o(1234L);
		assert(o.get<long>() == 1234L);
		object o2(o);
		assert(o2.get<long>() == 1234L);

		o = 123;
		assert(o.get<int>() == 123);
	}
	{
		char s[] = "Ala ma kota";
		object o(s);
		assert(std::string(o.get()) == s);
		assert(std::string(o.get<char const *>()) == s);
		assert(o.get<std::string>() == s);
		object o2(o);
		assert(std::string(o2.get()) == s);
		assert(std::string(o2.get<char const *>()) == s);
		assert(o2.get<std::string>() == s);

		o = 123;
		assert(o.get<int>() == 123);
	}
	{
		std::string s = "Ala ma kota";
		object o(s);
		assert(o.get<std::string>() == s);
		object o2(o);
		assert(o2.get<std::string>() == s);

		o = 123;
		assert(o.get<int>() == 123);
	}
	{
		object o1(123);
		object o2(321);
		o1.swap(o2);
		assert(o1.get<int>() == 321);
		assert(o2.get<int>() == 123);
	}
	{
		object o(123);
		assert(o.size() == 1);
		o = "Ala ma kota";
		assert(o.size() == 3);
		assert(o.at(1).get<std::string>() == "ma");
	}
	{
		object o("Ala ma kota");
		o.append(object("na"));
		o.append(object("punkcie"));
		o.append(object("psa"));
		assert(o.size() == 6);
		assert(o.get<std::string>() == "Ala ma kota na punkcie psa");
		o.replace(2, 1, object("hopla"));
		assert(o.size() == 6);
		assert(o.get<std::string>() == "Ala ma hopla na punkcie psa");
	}
	{
		object o = i.eval("set i 17");
		assert(o.get<int>() == 17);
	}
	{
		object o("set i 18");
		int val = i.eval(o);
		assert(val == 18);
	}
	{
		object o1("malego kota");
		object o2("ma");
		o2.append(o1); // o2=="ma {malego kota}"
		object o3("Ala");
		o3.append(o2); // o3=="Ala {ma {malego kota}}"
		object o4("list");
		object o5("set");

		std::vector<object> v;
		v.push_back(o5);
		v.push_back(o4);
		v.push_back(o3);
		i.eval(v.begin(), v.end()); // "set list {Ala {ma {malego kota}}}"
		int ret = i.eval("llength $list");
		assert(ret == 2);
		std::string s = i.eval("lindex $list 0");
		assert(s == "Ala");
		s = static_cast<std::string>(i.eval("lindex $list 1"));
		assert(s == "ma {malego kota}");
		s = static_cast<std::string>(i.eval("lindex [lindex [lindex $list 1] 1] 0"));
		assert(s == "malego");
	}
}

void test2() {
	Tcl_Interp * interp = Tcl_CreateInterp();
	interpreter i(interp, true);
	i.def("fun", fun);

	object in("Ala ma kota");
	object cmd("fun");
	cmd.append(in);
	int ret = i.eval(cmd);
	assert(ret == 3);
}

int main() {
	try {
		test1();
		test2();
	} catch (std::exception const &e) {
		std::cerr << "Error: " << e.what() << '\n';
	}
}
