// example5.cc

#include "cpptcl.h"
#include <string>

using namespace std;
using namespace Tcl;

class Person
{
public:
     Person(string const &n) : name(n) {}

     void setName(string const &n) { name = n; }
     string getName() { return name; }

private:
     string name;
};

// this is a factory function
Person * makePerson(string const &name)
{
     return new Person(name);
}

// this is a sink function
void killPerson(Person *p)
{
     delete p;
}

CPPTCL_MODULE(Mymodule, i)
{
     // note that the Person class is exposed without any constructor
     i.class_<Person>("Person", no_init)
          .def("setName", &Person::setName)
          .def("getName", &Person::getName);

     i.def("makePerson", makePerson, factory("Person"));
     i.def("killPerson", killPerson, sink(1));
}
