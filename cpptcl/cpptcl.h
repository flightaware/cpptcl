//
// Copyright (C) 2004-2006, Maciej Sobczak
// Copyright (C) 2017-2018, FlightAware LLC
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#ifndef CPPTCL_INCLUDED
#define CPPTCL_INCLUDED

#ifdef _TCL
#ifndef USE_TCL_STUBS
#ifndef CPPTCL_NO_TCL_STUBS
#error "tcl.h header included before cpptcl.h.  Either set USE_TCL_STUBS or set CPPTCL_NO_TCL_STUBS if creating a TCL interpreter."
#endif
#endif
#endif

#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

//
// Using TCL stubs is the default behavior
//
#ifndef CPPTCL_NO_TCL_STUBS
    #ifndef USE_TCL_STUBS
    #define USE_TCL_STUBS
    #endif
#endif

#include "tcl.h"

// This function is not part of TCL, but is a useful helper.
extern "C" {
    Tcl_Interp * Tcl_CreateInterpWithStubs(const char *version, int exact);
};

namespace Tcl {

// exception class used for reporting all Tcl errors

class tcl_error : public std::runtime_error {
  public:
	explicit tcl_error(std::string const &msg) : std::runtime_error(msg) {}
	explicit tcl_error(Tcl_Interp *interp) : std::runtime_error(Tcl_GetString(Tcl_GetObjResult(interp))) {}
};

// call policies

struct policies {
	policies() : variadic_(false), usage_("Too few arguments.") {}

	policies &factory(std::string const &name);

	// note: this is additive
	policies &sink(int index);

	policies &variadic();

	policies &usage(std::string const &message);

	std::string factory_;
	std::vector<int> sinks_;
	bool variadic_;
	std::string usage_;
};

// syntax short-cuts
policies factory(std::string const &name);
policies sink(int index);
policies variadic();
policies usage(std::string const &message);

class interpreter;
class object;

namespace details {

// wrapper for the evaluation result
class result {
  public:
	result(Tcl_Interp *interp);

	operator bool() const;
	operator double() const;
	operator int() const;
	operator long() const;
	operator std::string() const;
	operator object() const;

  private:
	Tcl_Interp *interp_;
};

// helper functions used to set the result value

void set_result(Tcl_Interp *interp, bool b);
void set_result(Tcl_Interp *interp, int i);
void set_result(Tcl_Interp *interp, long i);
void set_result(Tcl_Interp *interp, double d);
void set_result(Tcl_Interp *interp, std::string const &s);
void set_result(Tcl_Interp *interp, void *p);
void set_result(Tcl_Interp *interp, object const &o);

// helper functor for converting Tcl objects to the given type
#include "cpptcl/details/conversions.h"

// dispatchers able to capture (or ignore) the result
#include "cpptcl/details/dispatchers.h"

// helper for checking for required number of parameters
// (throws tcl_error when not met)
void check_params_no(int objc, int required, const std::string &message);

// helper for gathering optional params in variadic functions
object get_var_params(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int from, policies const &pol);

// the callback_base is used to store callback handlers in a polynorphic map
class callback_base {
  public:
	virtual ~callback_base() {}

	virtual void invoke(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) = 0;
};

// base class for object command handlers
// and for class handlers
class object_cmd_base {
  public:
	// destructor not needed, but exists to shut up the compiler warnings
	virtual ~object_cmd_base() {}

	virtual void invoke(void *p, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) = 0;
};

// base class for all class handlers, still abstract
class class_handler_base : public object_cmd_base {
  public:
	typedef std::map<std::string, policies> policies_map_type;

	class_handler_base();

	void register_method(std::string const &name, std::shared_ptr<object_cmd_base> ocb, policies const &p);

	policies &get_policies(std::string const &name);

  protected:
	typedef std::map<std::string, std::shared_ptr<object_cmd_base> > method_map_type;

	// a map of methods for the given class
	method_map_type methods_;

	policies_map_type policies_;
};

// class handler - responsible for executing class methods
template <class C> class class_handler : public class_handler_base {
  public:
	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) {
		C *p = static_cast<C *>(pv);

		if (objc < 2) {
			throw tcl_error(pol.usage_);
		}

		std::string methodName(Tcl_GetString(objv[1]));

		if (methodName == "-delete") {
			Tcl_DeleteCommand(interp, Tcl_GetString(objv[0]));
			delete p;
			return;
		}

		// dispatch on the method name

		method_map_type::iterator it = methods_.find(methodName);
		if (it == methods_.end()) {
			throw tcl_error("Method " + methodName + " not found.");
		}

		it->second->invoke(pv, interp, objc, objv, pol);
	}
};

// factory functions for creating class objects
#include "cpptcl/details/constructors.h"

// actual callback envelopes
#include "cpptcl/details/callbacks.h"

// actual method envelopes
#include "cpptcl/details/methods.h"

// helper meta function for figuring appropriate constructor callback
#include "cpptcl/details/metahelpers.h"

// this class is used to provide the "def" interface for defining
// class member functions

template <class C> class class_definer {
  public:
	class_definer(std::shared_ptr<class_handler<C>> ch) : ch_(ch) {}

	template <typename R> class_definer &def(std::string const &name, R (C::*f)(), policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method0<C, R>(f)), p);
		return *this;
	}

	template <typename R> class_definer &def(std::string const &name, R (C::*f)() const, policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method0<C, R>(f)), p);
		return *this;
	}

	template <typename R, typename T1> class_definer &def(std::string const &name, R (C::*f)(T1), policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method1<C, R, T1>(f)), p);
		return *this;
	}

	template <typename R, typename T1> class_definer &def(std::string const &name, R (C::*f)(T1) const, policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method1<C, R, T1>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2> class_definer &def(std::string const &name, R (C::*f)(T1, T2), policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method2<C, R, T1, T2>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2> class_definer &def(std::string const &name, R (C::*f)(T1, T2) const, policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method2<C, R, T1, T2>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3), policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method3<C, R, T1, T2, T3>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3) const, policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method3<C, R, T1, T2, T3>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4), policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method4<C, R, T1, T2, T3, T4>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4) const, policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method4<C, R, T1, T2, T3, T4>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4, T5), policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method5<C, R, T1, T2, T3, T4, T5>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4, T5) const, policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method5<C, R, T1, T2, T3, T4, T5>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4, T5, T6), policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method6<C, R, T1, T2, T3, T4, T5, T6>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4, T5, T6) const, policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method6<C, R, T1, T2, T3, T4, T5, T6>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4, T5, T6, T7), policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method7<C, R, T1, T2, T3, T4, T5, T6, T7>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4, T5, T6, T7) const, policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method7<C, R, T1, T2, T3, T4, T5, T6, T7>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4, T5, T6, T7, T8), policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method8<C, R, T1, T2, T3, T4, T5, T6, T7, T8>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4, T5, T6, T7, T8) const, policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method8<C, R, T1, T2, T3, T4, T5, T6, T7, T8>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4, T5, T6, T7, T8, T9), policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method9<C, R, T1, T2, T3, T4, T5, T6, T7, T8, T9>(f)), p);
		return *this;
	}

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> class_definer &def(std::string const &name, R (C::*f)(T1, T2, T3, T4, T5, T6, T7, T8, T9) const, policies const &p = policies()) {
		ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method9<C, R, T1, T2, T3, T4, T5, T6, T7, T8, T9>(f)), p);
		return *this;
	}

  private:
	std::shared_ptr<class_handler<C>> ch_;
};

} // namespace details

// init type for defining class constructors
template <typename T1 = void, typename T2 = void, typename T3 = void, typename T4 = void, typename T5 = void, typename T6 = void, typename T7 = void, typename T8 = void, typename T9 = void> class init {};

// no_init type and object - to define classes without constructors
namespace details {
struct no_init_type {};
} // namespace details
extern details::no_init_type no_init;

// interpreter wrapper
class interpreter {
  public:
	static interpreter *defaultInterpreter;

	static interpreter *getDefault() {
		if (defaultInterpreter == NULL) {
			throw tcl_error("No default interpreter available");
		}
		return defaultInterpreter;
	}

  private:
	interpreter(const interpreter &i);
	interpreter();

  public:
	interpreter(Tcl_Interp *, bool owner = false);
	~interpreter();

	void make_safe();

	Tcl_Interp *get() const { return interp_; }

	// free function definitions

	template <typename R> void def(std::string const &name, R (*f)(), policies const &p = policies()) { add_function(name, std::shared_ptr<details::callback_base>(new details::callback0<R>(f)), p); }

	template <typename R, typename T1> void def(std::string const &name, R (*f)(T1), policies const &p = policies()) { add_function(name, std::shared_ptr<details::callback_base>(new details::callback1<R, T1>(f)), p); }

	template <typename R, typename T1, typename T2> void def(std::string const &name, R (*f)(T1, T2), policies const &p = policies()) { add_function(name, std::shared_ptr<details::callback_base>(new details::callback2<R, T1, T2>(f)), p); }

	template <typename R, typename T1, typename T2, typename T3> void def(std::string const &name, R (*f)(T1, T2, T3), policies const &p = policies()) { add_function(name, std::shared_ptr<details::callback_base>(new details::callback3<R, T1, T2, T3>(f)), p); }

	template <typename R, typename T1, typename T2, typename T3, typename T4> void def(std::string const &name, R (*f)(T1, T2, T3, T4), policies const &p = policies()) { add_function(name, std::shared_ptr<details::callback_base>(new details::callback4<R, T1, T2, T3, T4>(f)), p); }

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5> void def(std::string const &name, R (*f)(T1, T2, T3, T4, T5), policies const &p = policies()) { add_function(name, std::shared_ptr<details::callback_base>(new details::callback5<R, T1, T2, T3, T4, T5>(f)), p); }

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> void def(std::string const &name, R (*f)(T1, T2, T3, T4, T5, T6), policies const &p = policies()) { add_function(name, std::shared_ptr<details::callback_base>(new details::callback6<R, T1, T2, T3, T4, T5, T6>(f)), p); }

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> void def(std::string const &name, R (*f)(T1, T2, T3, T4, T5, T6, T7), policies const &p = policies()) { add_function(name, std::shared_ptr<details::callback_base>(new details::callback7<R, T1, T2, T3, T4, T5, T6, T7>(f)), p); }

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> void def(std::string const &name, R (*f)(T1, T2, T3, T4, T5, T6, T7, T8), policies const &p = policies()) { add_function(name, std::shared_ptr<details::callback_base>(new details::callback8<R, T1, T2, T3, T4, T5, T6, T7, T8>(f)), p); }

	template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> void def(std::string const &name, R (*f)(T1, T2, T3, T4, T5, T6, T7, T8, T9), policies const &p = policies()) { add_function(name, std::shared_ptr<details::callback_base>(new details::callback9<R, T1, T2, T3, T4, T5, T6, T7, T8, T9>(f)), p); }

	// class definitions

	template <class C> details::class_definer<C> class_(std::string const &name) {
		std::shared_ptr<details::class_handler<C>> ch(new details::class_handler<C>());

		add_class(name, ch);

		add_constructor(name, ch, std::shared_ptr<details::callback_base>(new details::callback0<C *>(&details::construct<C, void, void, void, void, void, void, void, void, void>::doit)));

		return details::class_definer<C>(ch);
	}

	template <class C, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> details::class_definer<C> class_(std::string const &name, init<T1, T2, T3, T4, T5, T6, T7, T8, T9> const &, policies const &p = policies()) {
		typedef typename details::get_callback_type_for_construct<C, T1, T2, T3, T4, T5, T6, T7, T8, T9>::type callback_type;

		std::shared_ptr<details::class_handler<C>> ch(new details::class_handler<C>());

		add_class(name, ch);

		add_constructor(name, ch, std::shared_ptr<details::callback_base>(new callback_type(&details::construct<C, T1, T2, T3, T4, T5, T6, T7, T8, T9>::doit)), p);

		return details::class_definer<C>(ch);
	}

	template <class C> details::class_definer<C> class_(std::string const &name, details::no_init_type const &) {
		std::shared_ptr<details::class_handler<C>> ch(new details::class_handler<C>());

		add_class(name, ch);

		return details::class_definer<C>(ch);
	}

	// free script evaluation
	details::result eval(std::string const &script);
	details::result eval(std::istream &s);

	details::result eval(object const &o);

	// the InputIterator should give object& or Tcl_Obj* when dereferenced
	template <class InputIterator> details::result eval(InputIterator first, InputIterator last);

	// Get a variable from TCL interpreter with Tcl_GetVar
	details::result getVar(std::string const &scalarTclVariable);
	details::result getVar(std::string const &arrayTclVariable, std::string const &arrayIndex);

    // check if variables exist
    bool exists(std::string const &scalarTclVariable);
    bool exists(std::string const &arrayTclVariable, std::string const &arrayIndex);

	// create alias from the *this interpreter to the target interpreter
	void create_alias(std::string const &cmd, interpreter &targetInterp, std::string const &targetCmd);

	// register a package info (useful when defining packages)
	void pkg_provide(std::string const &name, std::string const &version);

	// create a namespace
	void create_namespace(std::string const &name);

	// helper for cleaning up callbacks in non-managed interpreters
	static void clear_definitions(Tcl_Interp *);

  private:
	void operator=(const interpreter &);

	void add_function(std::string const &name, std::shared_ptr<details::callback_base> cb, policies const &p = policies());

	void add_class(std::string const &name, std::shared_ptr<details::class_handler_base> chb);

	void add_constructor(std::string const &name, std::shared_ptr<details::class_handler_base> chb, std::shared_ptr<details::callback_base> cb, policies const &p = policies());

	Tcl_Interp *interp_;
	bool owner_;
};

#include "cpptcl/cpptcl_object.h"

// the InputIterator should give object& or Tcl_Obj* when dereferenced
template <class InputIterator> details::result interpreter::eval(InputIterator first, InputIterator last) {
	std::vector<Tcl_Obj *> v;
	object::fill_vector(v, first, last);
	int cc = Tcl_EvalObjv(interp_, static_cast<int>(v.size()), v.empty() ? NULL : &v[0], 0);
	if (cc != TCL_OK) {
		throw tcl_error(interp_);
	}

	return details::result(interp_);
}

namespace details {

// additional callback envelopes for variadic functions
#include "cpptcl/details/callbacks_v.h"

// additional method envelopes for variadic methods
#include "cpptcl/details/methods_v.h"

} // namespace details

#include "cpptcl/details/bind.h"

inline std::ostream & operator<<(std::ostream &os, const object& obj)
{
    return os << obj.get<std::string>();
}

} // namespace Tcl

// macro for defining loadable module entry point
// - used for extending Tcl interpreter

#define CPPTCL_MODULE(name, i)                       \
	void name##_cpptcl_Init(Tcl::interpreter &i);    \
	extern "C" int name##_Init(Tcl_Interp *interp) { \
		Tcl_InitStubs(interp, "8.3", 0);             \
		Tcl::interpreter i(interp, false);           \
		name##_cpptcl_Init(i);                       \
		return TCL_OK;                               \
	}                                                \
	void name##_cpptcl_Init(Tcl::interpreter &i)

#endif // CPPTCL_INCLUDED
