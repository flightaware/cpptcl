#include "cpptcl.h"
#include <string>

using namespace std;
using namespace Tcl;

class Person {
  public:
	void setName(string const &n) { name = n; }
	string getName() { return name; }

  private:
	string name;
};

CPPTCL_MODULE(Cpptcl_module_two, i) {
	i.class_<Person>("Person").def("setName", &Person::setName).def("getName", &Person::getName);

	// or, alternatively:

	// i.class_<Person>("Person", init<>())
	//     .def("setName", &Person::setName)
	//     .def("getName", &Person::getName);
}
