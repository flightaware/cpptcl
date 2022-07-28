[[prev](callpolicies.md)][[top](README.md)][[next](threads.md)]  

#### Advanced argument mapping

Besides the usual role of providing mapping to existing functions, 
cpptcl also allows C++ functions to be written that are specifically designed for interfacing TCL.

The calling conventions are communicated to cpptcl as part of the function's signature.

#### Tcl::opt

```
void print_hello(std::string & const message, Tcl::opt<int> const & ntimes) {
	for (int i = 0; i < (ntimes ? *ntimes : 1); ++i) {
		std::cout << message << "\n";
	}
}
```

This function when registered with cpptcl can be called as

```
print_hello word_up

print_hello word_up 10
```

#### Tcl::variadic

```
void print_hello(Tcl::variadic<std::string> const & messages) {
	for (auto const & m : messages) {
		std::cout << m << "\n";
	}
}
```

```
print_hello word up
print_hello
print_hello Yoh pretty ladies around the world
```

The variadic argument can only appear once and it has to be the last argument.

#### Tcl::list

```
void print_hello(Tcl::list<std::string> const & messages) {
	for (auto const & m : messages) {
		std::cout << m << "\n";
	}
}
```

```
print_hello { w o r d up }
print_hello [split "Wave your hands in the air" { }]
```

list is somewhat similar to variadic, but list can be specified multiple times and it
interprets a single argument as a list, whereas variadic maps to multiple arguments

#### Object types

tcl allows objects to be treated in much the same way as strings. When the tcl interpreter
is supplied with a function that generates a string representation of an object, the object
type can be attached to that scalar.

For these examples, the *_ops structure can be imagined as providing functinality to generate
the string representation. For each registred type the tcl interpreter will allocate a typeobj
structure that has a unique memory address.

```
Tcl::interpreter interp(Tcl_CreateInterp(), true);

interp.type<cell_ops>("cell", (Component *));
interp.type<pin_ops>("pin", (Pin *));
interp.type<net_ops>("net", (Net *));



Component * get_cell(std::string const & name) {
	return all_my_cells[name];
}
void number_of_pins(Component * c) {
	std::cout << "cell " << c->name << " has " << c->num_pins << " pins\n";
}

number_of_pins [get_cell CPU]
```

#### Tcl::any

To take a limited set of types as an argument.

```
void print_object(Tcl::any<Component, Net, Pin> const & obj) {
	if (Component  * c = obj.as<Component>()) {
		std::cout << c->name;
	} else if (Net * n = obj.as<Net>()) {
		std::cout << n->name;
	} else if (Pin * p = obj.as<Pin>()) {
		std::cout << p->name;
	}

	// or, alternatively
	obj.visit([&](Component * c) { std::cout << c->name; },
			  [&](Net       * n) { std::cout << n->name; },
			  [&](Pin       * p) { std::cout << p->name; });
}

print_obj [get_cell PCH]
print_obj [get_net GND]
```

### Tcl::getopt

This is the same as Tcl::opt, but when the procedure is called a getopt parser
is used to map tcl arguments to C++ arguments and the position of parameters
no longer maps trivially between function call and function implementation.

```
void fancy_print_hello(Tcl::getopt<bool> const & header, Tcl::getopt<std::string> const & signature, std::string const & message) {
	if (header) {
		std::cout << "Now hear this:\n";
	}
	std::cout << message << "\n";
	if (signature) {
		std::cout << "\t" << *signature << "\n";
	}
}

fancy_print_hello -signature Everybody -header "When you hear the call you got to get it underway"
```

#### Combinations

Tcl::opt and Tcl::getopt can encapsulate Tcl::any and Tcl::list.

```
void optional_hello(Tcl::opt<Tcl::any<Component, Net, Pin> > const & obj) {
	if (obj) {
		obj->visit(...);
	} else {
		std::cout << "nothing to see here\n";
	}
}
```

Tcl::list can encapsulate Tcl::any. A list of objects belonging to a type in the set.
Tcl::any can encapsulate Tcl::list. A list of objects belonging to a type in the set and all elements
in the list are of the same type.

Tcl::opt can encapsulate a Tcl::any<Tcl::list>. and so forth.

Tcl::any and Tcl::list both cannot encapsulate a Tcl::opt or Tcl::getopt. Tcl::opt cannot encapsulate Tcl::getopt.
Neither can Tcl::getopt encapsulate Tcl::opt. The result of undefined (and frankly nonsensical) combinations are
undefined. TODO: turn them into compile time errors.