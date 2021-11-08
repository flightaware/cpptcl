//-*- mode: c++; eval: (c-set-offset 'innamespace 0); -*-
//
// Copyright (C) 2004-2006, Maciej Sobczak
// Copyright (C) 2017-2019, FlightAware LLC
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
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <type_traits>
#include <tuple>
#include <functional>
#include <cstring>
#include <typeindex>

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

class tcl_usage_message_printed {
};

// call policies

struct policies {
	policies() : variadic_(false), usage_("Too few arguments.") {}
	
	policies &factory(std::string const &name);

	// note: this is additive
	policies &sink(int index);

	policies &variadic();

	policies &usage(std::string const &message);

	policies &options(std::string const & opts);
	
	std::string factory_;
	std::vector<int> sinks_;
	bool variadic_;
	std::string usage_;
	std::string options_;
};

// syntax short-cuts
policies factory(std::string const &name);
policies sink(int index);
//policies variadic();
policies usage(std::string const &message);
policies options(std::string const & options);

class interpreter;
class object;

void post_process_policies(Tcl_Interp *interp, policies &pol, Tcl_Obj *CONST objv[], bool isMethod);

namespace details {
template <typename T> struct tcl_cast;

template <typename T> struct tcl_cast<T *> {
	static T *from(Tcl_Interp *, Tcl_Obj *obj, bool byReference) {
		std::string s(Tcl_GetString(obj));
		if (s.size() == 0) {
			throw tcl_error("Expected pointer value, got empty string.");
		}

		if (s[0] != 'p') {
			throw tcl_error("Expected pointer value.");
		}

		std::istringstream ss(s);
		char dummy;
		void *p;
		ss >> dummy >> p;

		return static_cast<T *>(p);
	}
};

// the following partial specialization is to strip reference
// (it returns a temporary object of the underlying type, which
// can be bound to the const-ref parameter of the actual function)

template <typename T> struct tcl_cast<T const &> {
	static T from(Tcl_Interp *interp, Tcl_Obj *obj, bool byReference) { return tcl_cast<T>::from(interp, obj, byReference); }
};

template <typename T> class tcl_cast_by_reference {
  public:
	static bool const value = false;
};

// the following specializations are implemented
template <> struct tcl_cast<int> { static int from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };
template <> struct tcl_cast<long> { static long from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };
template <> struct tcl_cast<bool> { static bool from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };
template <> struct tcl_cast<double> { static double from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };
template <> struct tcl_cast<float> { static float from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };
template <> struct tcl_cast<std::string> { static std::string from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };
template <> struct tcl_cast<char const *> { static char const *from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };
template <> struct tcl_cast<object> { static object from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };
template <> struct tcl_cast<std::vector<char> > { static std::vector<char> from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };

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
	void reset();
  private:
	Tcl_Interp *interp_;
};

void set_result(Tcl_Interp *interp, bool b);
void set_result(Tcl_Interp *interp, int i);
void set_result(Tcl_Interp *interp, long i);
void set_result(Tcl_Interp *interp, double d);
void set_result(Tcl_Interp *interp, std::string const &s);
void set_result(Tcl_Interp *interp, void *p);
void set_result(Tcl_Interp *interp, object const &o);

// helper for checking for required number of parameters
// (throws tcl_error when not met)
void check_params_no(int objc, int required, int maximum, const std::string &message);

// helper for gathering optional params in variadic functions
object get_var_params(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int from, policies const &pol);

// the callback_base is used to store callback handlers in a polynorphic map
class callback_base {
  public:
	virtual ~callback_base() {}

	virtual void invoke(void * dummyp, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], bool object_dot_method) = 0;
};

// base class for object command handlers
// and for class handlers
class object_cmd_base {
  public:
	// destructor not needed, but exists to shut up the compiler warnings
	virtual ~object_cmd_base() {}
	virtual void invoke(void *p, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], bool object_dot_method) = 0;
};

// base class for all class handlers, still abstract
class class_handler_base : public object_cmd_base {
  public:
	typedef std::map<std::string, policies> policies_map_type;

	class_handler_base();

	void register_method(std::string const &name, std::shared_ptr<object_cmd_base> ocb, Tcl_Interp * interp, bool managed);

	//policies &get_policies(std::string const &name);

	void install_methods(Tcl_Interp * interp, const char * prefix, void * obj);
	void uninstall_methods(Tcl_Interp * interp, const char * prefix);
protected:
	typedef std::map<std::string, std::shared_ptr<object_cmd_base> > method_map_type;

	// a map of methods for the given class
	method_map_type methods_;

	//policies_map_type policies_;
};

// class handler - responsible for executing class methods
template <class C> class class_handler : public class_handler_base {
  public:
	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], bool object_dot_method) {
		C *p = static_cast<C *>(pv);

		//if (objc < 2) {
		//	throw tcl_error(pol.usage_);
		//}

		std::string methodName(Tcl_GetString(objv[1]));
		if (objc == 2 && methodName == "-delete") {
			Tcl_DeleteCommand(interp, Tcl_GetString(objv[0]));
			delete p;
			return;
		}

		// dispatch on the method name

		method_map_type::iterator it = methods_.find(methodName);
		if (it == methods_.end()) {
			throw tcl_error("Method " + methodName + " not found.");
		}

		it->second->invoke(pv, interp, objc, objv, false);
	}
};

} // details

class interpreter;

template <typename T>
struct optional {
	T val;
	bool valid;
	operator bool() const { return valid; }
	T const & operator*() const { return val; }
	T const * operator->() const { return &val; }
	optional() : valid(false) { }
	optional(T) : valid(false) { }
	optional(T t, bool v) : val(t), valid(v) { }
};

template <typename T>
struct getopt : public optional<T> {
	using optional<T>::optional;
};

template <typename T>
using opt = optional<T>;

template <typename ...Ts>
struct any;

namespace details {
template <typename ...Ts>
struct is_any {
	static const bool value = false;
};
template <typename ...Ts>
struct is_any<any<Ts...> > {
	static const bool value = true;
};

template <typename TT, typename ...Ts>
struct is_basic_type_impl {
	static const bool value = false;
};
template <typename TT, typename T, typename ...Ts>
struct is_basic_type_impl<TT, T, Ts...> {
	static const bool value = std::is_same<TT, T>::value || is_basic_type_impl<TT, Ts...>::value;
};

template <typename T>
struct is_basic_type {
	static const bool value = is_basic_type_impl<T, int, bool, float, double, std::string, const char *, std::vector<char> >::value;
};

} // details

template <typename T>
struct list {
	interpreter * interp_;
	Tcl_Obj * lo_;
	Tcl_ObjType ** ot_;
	bool list_owner_ = false;
	
	static const bool isany = details::is_any<T>::value;

	list() : interp_(nullptr) { }
	list(interpreter * interp, Tcl_Obj * lo, Tcl_ObjType ** ot, bool list_owner = false) : interp_(interp), lo_(lo), ot_(ot), list_owner_(list_owner) {
		if (list_owner) {
			Tcl_IncrRefCount(lo);
		}
	}

	list(list const & o) : interp_(o.interp_), lo_(o.lo_), ot_(o.ot_), list_owner_(o.list_owner_) {
		if (list_owner_) {
			//std::cerr << "copy list\n";
			Tcl_IncrRefCount(lo_);
		}
	}

	~list() {
		if (list_owner_) {
			//std::cerr << "~list " << lo_->refCount << "\n";
			Tcl_DecrRefCount(lo_);
		}
	}
	
	//list(list const & other) : ot_(other.ot_), interp_(other.interp_), lo_(other.lo_) { std::cerr << '#'; }
	
	typedef typename std::conditional<isany || details::is_basic_type<T>::value, T, T *>::type return_t;

	operator bool() const {
		return interp_;
	}

	struct iterator {
		const list * l;
		std::size_t ix;
		return_t operator*() const {
			return l->at(ix);
		}
		void operator++(int) { ++ix; }
		void operator++() { ++ix; }
		bool operator!=(const iterator & other) { return ix != other.ix; }
	};
	iterator begin() const {
		return iterator{this, 0};
	}
	iterator end() const {
		return iterator{this, size()};
	}

	std::size_t size() const ;
	return_t at(std::size_t ix) const;
	return_t operator[](std::size_t ix) const { return at(ix); }
};

template <typename T>
struct variadic {
	Tcl_Obj * const * objv;
	int objc;
	interpreter * interp_;
	
	variadic() : objc(0) { }
	variadic(interpreter * i, int c, Tcl_Obj * const * v) : interp_(i), objv(v), objc(c) { }
	int size() const { return objc; }
	T at(int ix) const;
	T operator[](int ix) const { return at(ix); }
	struct iterator {
		variadic const * v;
		int ix;
		bool operator!=(const iterator & other) { return ix != other.ix; }
		void operator++() { ++ix; }
		T operator *() { return v->at(ix); }
	};
	iterator begin() const { return iterator{ this, 0 }; }
	iterator end() const { return iterator{ this, size() }; }
	operator bool() const { return objc; }
};

// allow variadic() to be used to indicate traditional variadic policies
struct variadic_compat_tag;
variadic() -> variadic<variadic_compat_tag>;
template <>
struct variadic<variadic_compat_tag> : public policies {
	variadic() {
		variadic_ = true;
	}
};

template <typename ...Ts>
struct overload {
	
};

namespace details {

template <typename T>
struct is_variadic {
	static const bool value = false;
};
template <typename T>
struct is_variadic<variadic<T> > {
	static const bool value = true;
};

template <typename ...Ts>
struct has_variadic {
	static const bool value = false;
};
template <typename T, typename ...Ts>
struct has_variadic<T, Ts...> : public has_variadic<Ts...> { };
template <typename T, typename ...Ts>
struct has_variadic<variadic<T>, Ts...> {
	static const bool value = true;
};
template <typename T, typename ...Ts>
struct has_variadic<variadic<T> const &, Ts...> {
	static const bool value = true;
};

template <typename T>
struct remove_rc {
	typedef T type;
};
template <typename T>
struct remove_rc<T const &> {
	typedef T type;
};

template <typename T>
struct is_list {
	static const bool value = false;
};
template <typename T>
struct is_list<list<T> > {
	static const bool value = true;
};

template <typename ...Ts>
struct all_lists_impl {
	static const bool value = true;
};
template <typename T, typename ...Ts>
struct all_lists_impl<list<T>, Ts...> {
	static const bool value = all_lists_impl<Ts...>::value;
};
template <typename T, typename ...Ts>
struct all_lists_impl<T, Ts...> {
	static const bool value = false;
};

template <typename T>
struct all_lists {
	static const bool value = false;
};
template <typename ...Ts>
struct all_lists<any<Ts...> > : public all_lists_impl<Ts...> {
};
	
template <typename ...Ts>
struct any_impl {
	Tcl_Obj * o_;
	int which_;
	void set_which(interpreter *, Tcl_Obj * o) {
		//throw tcl_error(std::string("type ") + (o->typePtr ? o->typePtr->name : "(none)") + " is not in any<> type list");
		which_ = -1;
	}
};
template <typename T, typename ...Ts>
struct any_impl<T, Ts...> : public any_impl<Ts...> {
	void set_which(interpreter * interp, Tcl_Obj * o);
};

template <typename TT, typename ...Ts>
struct type_at {
	static const int value = -1;
};
template <typename TT, typename T, typename ...Ts>
struct type_at<TT, T, Ts...> {
	static const int value = std::is_same<TT, T>::value ? sizeof...(Ts) : type_at<TT, Ts...>::value;
};
}

template <typename ...Ts>
struct any : public details::any_impl<Ts...> {
	typedef details::any_impl<Ts...> super_t;
	any() { super_t::which_ = -1; }
	interpreter * interp_;
	Tcl_ObjType ** ot_;
	bool list_owner_ = false;
	
	any(interpreter * i, Tcl_Obj * o, Tcl_ObjType ** ot, bool list_owner = false) : interp_(i), ot_(ot), list_owner_(list_owner) {
		//this->p_ = o->internalRep.otherValuePtr;
		this->o_ = o;
		if (o) {
			//super_t::set_which(i, o);
		}
		if (list_owner) {
			Tcl_IncrRefCount(this->o_);
		}
	}
	any(any const & o) : interp_(o.interp_), ot_(o.ot_), list_owner_(o.list_owner_) {
		this->o_ = o.o_;
		this->which_ = o.which_;
		if (list_owner_) {
			Tcl_IncrRefCount(this->o_);
		}
	}
	~any() {
		if (list_owner_) {
			Tcl_DecrRefCount(this->o_);
		}
	}
	
	operator bool() const {
		for (int i = 0; i < sizeof...(Ts); ++i) {
			if (ot_[i] == this->o_->typePtr) return true;
		}
		return false;
		//return super_t::which_ != -1;
	}

	template <typename ...Fs>
	void visit(Fs... f) const;
	
	template <typename TT>
	typename std::conditional<details::is_list<TT>::value, TT, TT *>::type as() const;
};

template <typename ...Ts> struct init { };

namespace details {

template <typename T>
struct is_optional {
	static const bool value = false;
};
template <typename T>
struct is_optional<optional<T> > {
	static const bool value = true;
};
template <typename T>
struct is_optional<optional<T> const &> {
	static const bool value = true;
};

template <typename T>
struct is_getopt {
	static const bool value = false;
};
template <typename T>
struct is_getopt<getopt<T> > {
	static const bool value = true;
};
template <typename T>
struct is_getopt<getopt<T> const &> {
	static const bool value = true;
};

template <typename T>
struct optional_unpack {
	typedef T type;
};
template <typename T>
struct optional_unpack<optional<T> > {
	typedef T type;
};
template <typename T>
struct optional_unpack<getopt<T> > {
	typedef T type;
};

template <typename T>
struct list_unpack {
	typedef T type;
};
template <typename T>
struct list_unpack<list<T> > {
	typedef T type;
};

template <typename ...Ts>
struct ppack {
};

template <typename ...Ts>
struct num_optional {
	static const int value = 0;
};
template <typename T, typename ...Ts>
struct num_optional<T, Ts...> {
	static const int value = num_optional<Ts...>::value + (is_optional<T>::value ? 1 : 0);
};

template <typename ...Ts>
struct num_getopt {
	static const int value = 0;
};
template <typename T, typename ...Ts>
struct num_getopt<T, Ts...> {
	static const int value = num_getopt<Ts...>::value + (is_getopt<T>::value ? 1 : 0);
};

template <std::size_t Ix, std::size_t Itarget, std::size_t Ipos, typename ...Ts>
struct getopt_index {
	static const int value = -1;
};
template <std::size_t Ix, std::size_t Itarget, std::size_t Ipos, typename T, typename ...Ts>
struct getopt_index<Ix, Itarget, Ipos, getopt<T>, Ts...> {
	static const int value = Ix == Itarget ? Ipos : getopt_index<Ix + 1, Itarget, Ipos + 1, Ts...>::value;
};
template <std::size_t Ix, std::size_t Itarget, std::size_t Ipos, typename T, typename ...Ts>
struct getopt_index<Ix, Itarget, Ipos, getopt<T> const &, Ts...> {
	static const int value = Ix == Itarget ? Ipos : getopt_index<Ix + 1, Itarget, Ipos + 1, Ts...>::value;
};
template <std::size_t Ix, std::size_t Itarget, std::size_t Ipos, typename T, typename ...Ts>
struct getopt_index<Ix, Itarget, Ipos, T, Ts...> {
	static const int value = getopt_index<Ix + 1, Itarget, Ipos + 1, Ts...>::value;
};

template <typename ...Fs>
struct visit_impl {
	template <typename ...Ts>
	static void invoke(any<Ts...> const &) {
	}
};

class no_argument_match { };
template <bool strip_list, typename F, typename ...Ts>
struct argument_type {
	typedef no_argument_match type;
};
template <bool strip_list, typename F, typename T, typename ...Ts>
struct argument_type<strip_list, F, T, Ts...> {
	typedef typename std::conditional<std::is_invocable<
		F, typename std::conditional<strip_list, typename list_unpack<T>::type, T>::type *
	>::value, T, typename argument_type<strip_list, F, Ts...>::type>::type type;
};

template <typename F, typename ...Fs>
struct visit_impl<F, Fs...> {
	template <typename ...Ts>
	static void invoke(any<Ts...> const & a, F f, Fs... fs) {
		typedef typename argument_type<false, F, Ts...>::type arg_t;
		typedef typename argument_type<true,  F, Ts...>::type arg_list_t;

		if constexpr (!std::is_same<arg_t, arg_list_t>::value && !std::is_same<arg_list_t, no_argument_match>::value) {
			if (arg_list_t li = a.template as<arg_list_t>()) {
				for (auto && i : li) {
					f(i);
				}
				return;
			}
		} else if constexpr (!std::is_same<arg_t, no_argument_match>::value) {
			if (arg_t * p = a.template as<arg_t>()) {
				f(p);
				return;
			}
		}
		visit_impl<Fs...>::invoke(a, fs...);
	}
};
inline std::string tcl_typename(Tcl_Obj * o) {
	return o->typePtr ? o->typePtr->name : "()";
}
}

template <typename ...Ts>
template <typename ...Fs>
void any<Ts...>::visit(Fs... f) const {
	details::visit_impl<Fs...>::invoke(*this, f...);
}

namespace details {
template <int I, typename ...Ts>
struct generate_hasarg {
	static void invoke(bool * arr, std::string *, const char *) { }
};
template <int I, typename T, typename ...Ts>
struct generate_hasarg<I, T, Ts...> {
	static void invoke(bool * arr, std::string * opts, const char * p) {
		if (!is_getopt<T>::value) {
			generate_hasarg<I, Ts...>::invoke(arr, opts, p);
		} else {
			arr[I] = !std::is_same<typename remove_rc<T>::type, getopt<bool> >::value;
			while (*p && isspace(*p)) ++p;
			if (p) {
				const char * pend = p;
				while (*pend && !isspace(*pend)) ++pend;
				opts[I] = std::string(p, pend - p);
				
				generate_hasarg<I + 1, Ts...>::invoke(arr, opts, pend);
			} else {
				std::cerr << "option error\n";
			}
		}
	}
};


template <int I>
struct no_type { };

template <typename ...Ts>
struct signature { };

template <typename T>
struct subtype_notype_tag {
	static const int value = 0;
};
template <int I>
struct subtype_notype_tag<no_type<I> > {
	static const int value = I;
};

template <int I, typename T, bool E = I != 0>
struct subtype {
	typedef no_type<I> type;
};
template <typename T>
struct subtype<0, T, false> {
	typedef T type;
};

template <int I, typename T>
struct subtype<I, list<T>, true> {
	typedef typename subtype<I, T>::type type;
};
template <typename T>
struct subtype<0, list<T>, false> {
	typedef typename subtype<0, T>::type type;
};

template <int I, typename T, typename ...Ts>
struct subtype<I, any<T, Ts...>, true> {
	typedef typename subtype<I, T>::type t1;

	typedef typename std::conditional<
		sizeof...(Ts) && bool(subtype_notype_tag<t1>::value),
		typename subtype<subtype_notype_tag<t1>::value - 1, any<Ts...> >::type,
		typename subtype<I, T>::type
	>::type type;
};
template <typename T, typename ...Ts>
struct subtype<0, any<T, Ts...>, false> {
	typedef typename subtype<0, T>::type t1;

	typedef typename std::conditional<
		sizeof...(Ts) && bool(subtype_notype_tag<t1>::value),
		typename subtype<subtype_notype_tag<t1>::value, any<Ts...> >::type,
		typename subtype<0, T>::type
	>::type type;
};

template <int I, typename T, typename ...Ts>
struct subtype<I, signature<T, Ts...>, true> {
	typedef typename subtype<I, T>::type t1;

	typedef typename std::conditional<
		bool(subtype_notype_tag<t1>::value),
		typename subtype<subtype_notype_tag<t1>::value, signature<Ts...> >::type,
		typename subtype<I, T>::type
	>::type type;
};

template <int I, typename T>
struct subtype<I, getopt<T>, true> {
	typedef typename subtype<I, T>::type type;
};
template <int I, typename T>
struct subtype<I, opt<T>, true > {
	typedef typename subtype<I, T>::type type;
};
template <int I, typename T>
struct subtype<I, variadic<T>, true> {
	typedef typename subtype<I, T>::type type;
};

template <typename T>
struct subtype<0, getopt<T>, false> {
	typedef typename subtype<0, T>::type type;
};
template <typename T>
struct subtype<0, opt<T>, false> {
	typedef typename subtype<0, T>::type type;
};
template <typename T>
struct subtype<0, variadic<T>, false> {
	typedef typename subtype<0, T>::type type;
};


template <typename T>
struct num_subtypes {
	static const int value = 1 + 100000 - subtype_notype_tag<typename subtype<100000, T>::type>::value;
};

template <int I, typename ...Ts>
struct generate_argtypes {
	static void invoke(interpreter *, Tcl_ObjType **) { }
	static const int length = 0;
};


template <typename T, bool Last>
struct fix_variadic_return {
	typedef typename std::decay<T>::type type;
};

template <>
struct fix_variadic_return<object const &, true> {
	typedef object type;
};
template <typename TT, bool dummy>
struct return_dummy_value {
	static typename std::decay<TT>::type doit(object const &) {
		return typename std::decay<TT>::type();
	}
	template <typename OptionalT>
	static typename std::decay<TT>::type doit(OptionalT, bool) {
		return typename std::decay<TT>::type();
	}
	static typename std::decay<TT>::type doit(Tcl_Interp *, Tcl_Obj *, Tcl_ObjType *) {
		return typename std::decay<TT>::type();
	}
};
template <typename TT> struct return_dummy_value<TT, false> {
	static TT doit(object const & o) {
		return o;
	}
	template <typename OptionalT>
	static TT doit(OptionalT o, bool v) {
		return TT(o, v);
	}
	static TT doit(interpreter * i, Tcl_Obj * o, Tcl_ObjType *ot) {
		return TT(i, o, ot);
	}
};
template <int ArgOffset, typename TT, std::size_t In, std::size_t Ii, typename ...Ts>
TT  do_cast(ppack<Ts...>, bool variadic, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[], int getoptc, Tcl_Obj * CONST getoptv[], policies const & pol);

struct no_init_type {};
struct no_class { };

template <typename C>
struct class_lambda { };

template <typename C>
struct class_lambda_unpack {
	typedef C type;
};
template <typename C>
struct class_lambda_unpack<class_lambda<C> > {
	typedef C type;
};

template <typename C>
struct is_class_lambda {
	static const bool value = !std::is_same<C, typename class_lambda_unpack<C>::type>::value;
};

template <typename Cparm, typename Fn, typename R, typename ...Ts> class callback_v : public std::conditional<std::is_same<Cparm, no_class>::value, details::callback_base, details::object_cmd_base>::type {
	typedef typename class_lambda_unpack<Cparm>::type C;

	typedef Fn functor_type;
	policies policies_;
	functor_type f_;

	static const int num_opt = num_getopt<Ts...>::value;
	std::string opts_[num_opt];

	bool has_arg_[num_opt + 1] = { false };

	static const int arg_offset = -num_opt;

	Tcl_ObjType * argument_types_all_[generate_argtypes<0, Ts...>::length + 1];
	
	interpreter * interpreter_;
 public :
	callback_v(interpreter * i, functor_type f, std::string const & name, policies const & pol) : interpreter_(i), policies_(pol), f_(f) {
		if (num_opt) {
			if (pol.options_.empty()) {
				throw tcl_error(std::string("no getopt string supplied for function \"") + name + "\" taking getopt<> arguments");
			}
			generate_hasarg<0, Ts...>::invoke(has_arg_, opts_, pol.options_.c_str());
		}
		//std::cerr << "genargtype\n";
		generate_argtypes<0, Ts...>::invoke(i, argument_types_all_);
	}
	template <bool v> struct void_return { };

	template <int ArgI>
	std::pair<Tcl_ObjType **, Tcl_ObjType **> args() {
		return generate_argtypes<0, Ts...>::template get<ArgI>(argument_types_all_);
	}
	
	// regular function (return / void)
	template <typename CC, std::size_t ...Is, typename std::enable_if<std::is_same<CC, no_class>::value, bool>::type = true>
	void do_invoke(std::index_sequence<Is...> const &, CC * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], int getoptc, Tcl_Obj * const getoptv[], policies const & pol, void_return<false>) {
		if (argc && argv) { }
		details::set_result(interp, f_(do_cast<arg_offset, typename remove_rc<Ts>::type, sizeof...(Is), Is>(interpreter_, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol)...));
	}
	template <typename CC, std::size_t ...Is, typename std::enable_if<std::is_same<CC, no_class>::value, bool>::type = true>
	void do_invoke(std::index_sequence<Is...> const &, CC * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], int getoptc, Tcl_Obj ** getoptv, policies const & pol, void_return<true>) {
		if (argc && argv && interp) { }
		f_(do_cast<arg_offset, typename remove_rc<Ts>::type, sizeof...(Is), Is>(interpreter_, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol)...);
	}


	// method implemented by a lambda (return / void)
	template <typename CC, std::size_t ...Is, typename std::enable_if<!std::is_same<CC, Cparm>::value, bool>::type = true>
	void do_invoke(std::index_sequence<Is...> const &, CC * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], int getoptc, Tcl_Obj ** getoptv, policies const & pol, void_return<false>) {
		if (argc && argv) { }
		details::set_result(interp, f_(p, do_cast<arg_offset, typename remove_rc<Ts>::type, sizeof...(Is), Is>(interpreter_, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol)...));
	}
	template <typename CC, std::size_t ...Is, typename std::enable_if<!std::is_same<CC, Cparm>::value, bool>::type = true>
	void do_invoke(std::index_sequence<Is...> const &, CC * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], int getoptc, Tcl_Obj ** getoptv, policies const & pol, void_return<true>) {
		if (argc && argv && interp) { }
		f_(p, do_cast<arg_offset, typename remove_rc<Ts>::type, sizeof...(Is), Is>(interpreter_, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol)...);
	}


	// method (return / void)
	template <typename CC, std::size_t ...Is, typename std::enable_if<std::is_same<CC, Cparm>::value && !std::is_same<CC, no_class>::value, bool>::type = true>
	void do_invoke(std::index_sequence<Is...> const &, CC * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], int getoptc, Tcl_Obj ** getoptv, policies const & pol, void_return<false>) {
		if (argc && argv) { }
		details::set_result(interp, (p->*f_)(do_cast<arg_offset, typename remove_rc<Ts>::type, sizeof...(Is), Is>(interpreter_, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol)...));
	}
	template <typename CC, std::size_t ...Is, typename std::enable_if<std::is_same<CC, Cparm>::value && !std::is_same<CC, no_class>::value, bool>::type = true>
	void do_invoke(std::index_sequence<Is...> const &, CC * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], int getoptc, Tcl_Obj ** getoptv, policies const & pol, void_return<true>) {
		if (argc && argv && interp) { }
		(p->*f_)(do_cast<arg_offset, typename remove_rc<Ts>::type, sizeof...(Is), Is>(interpreter_, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol)...);
	}

	void invoke(void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv [], bool object_dot_method);
};

template <typename T>
struct std_function {
	typedef void type;
};
template <typename Ret, typename C, typename ...Args>
struct std_function<Ret (C::*)(Args...) const> {
	typedef std::function<Ret(Args...)> type;
};
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

	class TclChannelStreambuf : public std::streambuf {
	private:
		Tcl_Channel channel_;
		Tcl_DString iBuffer_;
		int iOffset_;
	public:
		TclChannelStreambuf (Tcl_Channel channel)
			: channel_ (channel)
		{
			Tcl_DStringInit (&iBuffer_);
			iOffset_ = 0;
			base_ = (char *) malloc(1024);
			buf_size_ = 1024;
			my_base_ = true;
		}
		~TclChannelStreambuf ()
		{
			Tcl_DStringFree (&iBuffer_);
		}

		char * base() {
			return base_;
		}
		char * ebuf() {
			return base_ + buf_size_;
		}
		char * base_;
		bool my_base_;
		std::streamsize buf_size_;
		
		std::streambuf * setbuf(char * s, std::streamsize n) {
			if (my_base_) {
				free(base_);
				my_base_ = false;
			}
			base_ = s;
			buf_size_ = n;
			return this;
		}

		std::streamsize xsputn(const char_type * s, std::streamsize n) {
			if (base() == 0) {
				int r = Tcl_Write(channel_, s, n);
				return r;
			}
			if (pptr() < ebuf()) {
				std::streamsize nn = std::min(n, ebuf() - pptr());
				memcpy(pptr(), s, nn);
				setp(pptr() + nn, ebuf());
				return nn;
			}
			return 0;
		}
		int overflow (int c) {
			int status;

			//std::cerr << "overflow " << (char) c << "\n";
			
			// Allocate reserve area if necessary
			
			if (base() == 0) {
				char b[1];
				b[0] = c;
				status = Tcl_Write (channel_, b, 1);
				if (status != 1) return EOF;
				return 1;
			}
			
			// If there's no output buffer, allocate one.  Place it after the
			// end of the input buffer if there's input.
			
			if (!pbase ()) {
				if (egptr () > gptr ()) {
					setp (egptr (), ebuf ());
				} else {
					setp (base (), ebuf ());
				}
			}
			
			// If there's stuff in the output buffer, write it to the channel.
			
			if (pptr () > pbase ()) {
				status = Tcl_Write (channel_, pbase (), pptr () - pbase ());
				if (status < (pptr () - pbase ())) {
					return EOF;
				}
				setp (pbase (), ebuf ());
			}
			
			// Save the next character in the output buffer.  If there is none,
			// flush the channel
			
			if (c != EOF) {
				*(pptr()) = c;
				pbump (1);
				return c;
			} else {
				setp (0, 0);
				status = Tcl_Flush (channel_);
				if (status != TCL_OK) return EOF;
				return 0;
			}
		}
		
		int underflow ()
		{
			if (egptr() > gptr()) {
				return *gptr ();
			}
			
			if (pptr () > pbase ()) {
				if (overflow (EOF) == EOF) {
					return EOF;
				}
			}
			
			// Get a fresh line of input if needed
			
			if (iOffset_ >= Tcl_DStringLength (&iBuffer_)) {
				if (Tcl_Gets (channel_, &iBuffer_)) {
					return EOF;
				}
				Tcl_DStringAppend (&iBuffer_, "\n", 1);
			}
			
			// Determine how much input to transfer.  Don't fill the reserve
			// area more than half full
			
			size_t xferlen = Tcl_DStringLength (&iBuffer_);
			if ((long) xferlen > (ebuf () - base ()) / 2) {
				xferlen = (ebuf () - base ()) / 2;
			}
			
			// Copy string into the buffer, and advance pointers
			
			memcpy ((void *) base (), (void *) Tcl_DStringValue (&iBuffer_), xferlen);
			iOffset_ += xferlen;
			setg (base (), base (), base () + xferlen);
			
			// Free the input string if we're finished with it
			
			if (iOffset_ >= Tcl_DStringLength (&iBuffer_)) {
				Tcl_DStringFree (&iBuffer_);
				iOffset_ = 0;
			}
			
			// Return the first character read.
			
			return *gptr ();
		}
		
		int sync () {
			// Flush output
			
			if (overflow (EOF) == EOF) {
				return EOF;
			}
			
			// Discard input
			setg (0, 0, 0);
			
			return 0;
		}
	};
public:
	std::iostream & tin() {
		if (! tin_.rdbuf()) {
			tin_.rdbuf(new TclChannelStreambuf (Tcl_GetStdChannel (TCL_STDIN)));
		}
		return tin_;
	}
	std::iostream & tout() {
		if (! tout_.rdbuf()) {
			tout_.rdbuf(new TclChannelStreambuf (Tcl_GetStdChannel (TCL_STDOUT)));
		}
		return tout_;
	}
	std::iostream & terr() {
		if (! terr_.rdbuf()) {
			terr_.rdbuf(new TclChannelStreambuf (Tcl_GetStdChannel (TCL_STDERR)));
			terr_.rdbuf()->pubsetbuf(NULL, 0);
		}
		return terr_;
	}

	void trace(bool v) {
		trace_ = v;
	}
	bool trace() const {
		return trace_;
	}
	bool trace_ = false;

	int trace_count() const {
		return trace_count_;
	}
	void trace_count(int v) {
		trace_count_ = v;
	}

	int trace_count_ = 0;
	
	template <class C> class class_definer {
	public:
		class_definer(std::shared_ptr<details::class_handler<C>> ch, interpreter * inter, bool managed) : ch_(ch), interp_(inter), managed_(managed) {}
		
		template <typename Fn>
		class_definer & def(std::string const & name, Fn fn, policies const & p = policies()) {
			return def2<Fn>(name, fn, p);
		}

		template <typename Fn, typename R, typename ...Ts>
		class_definer & def2(std::string const & name, R (C::*f)(Ts...), policies const & p = policies()) {
			ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::callback_v<C, Fn, R, Ts...>(interp_, f, name, p)), interp_->get_interp(), managed_);
			return *this;
		}
		template <typename Fn, typename R, typename ...Ts>
		class_definer & def2(std::string const & name, R (C::*f)(Ts...) const, policies const & p = policies()) {
			ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::callback_v<C, Fn, R, Ts...>(interp_, f, name, p)), interp_->get_interp(), managed_);
			return *this;
		}
		template <typename R, typename ...Ts>
		class_definer & def2(std::string const & name, std::function<R(C *, Ts...)> fn, policies const & p = policies()) {
			ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::callback_v<details::class_lambda<C>, std::function<R(C *, Ts...)>, R, Ts...>(interp_, fn, name, p)), interp_->get_interp(), managed_);
			return *this;
		}
		
		template <typename Fn>
		class_definer & def2(std::string const & name, Fn fn, policies const & p = policies()) {
			return def2(name, typename details::std_function<decltype(&Fn::operator())>::type(fn), p);
		}

		template <typename T>
		class_definer & defvar(std::string const & name, T C::*varp);
	private:
		interpreter * interp_;
		std::shared_ptr<details::class_handler<C>> ch_;
		bool managed_;
	};

	template <typename C>
	struct function_definer {
		function_definer(C * this_p, interpreter * interp) : this_p_(this_p), interp_(interp) { }

		template <typename Fn>
		function_definer & def(std::string const & name, Fn fn, policies const & p = policies()) {
			interp_->def(name, fn, this_p_, p);
			return *this;
		}

		template <typename T>
		function_definer & defvar(std::string const & name, T C::*t);
		
		C * this_p_;
		interpreter * interp_;
	};
	
	interpreter(Tcl_Interp *, bool owner = false);
	~interpreter();

	void make_safe();

	Tcl_Interp *get() const { return interp_; }
	Tcl_Interp * get_interp() { return interp_; }
	
	// free function definitions

	template <typename R, typename ...Ts>
	void def(std::string const & name, R(*f)(Ts...), policies const & p = policies()) {
		add_function(name, new details::callback_v<details::no_class, R(*)(Ts...), R, Ts...>(this, f, name, p));
	}
	template <typename R, typename ...Ts>
	void def(std::string const & name, std::function<R(Ts...)> fn, policies const & p = policies()) {
		add_function(name, new details::callback_v<details::no_class, std::function<R(Ts...)>, R, Ts...>(this, fn, name, p));
	}

	template <typename Fn>
	void def(std::string const & name, Fn fn, policies const & p = policies()) {
		def(name, typename details::std_function<decltype(&Fn::operator())>::type(fn), p);
	}

	template <typename C, typename R, typename ...Ts>
	void def(std::string const & name, R (C::*f)(Ts...), C * this_p, policies const & p = policies()) {
		def(name, [this_p, f](Ts... args) { return (this_p->*f)(args...); }, p);
	}

	template <typename T>
	void defvar(std::string const & name, T & v);

	template <typename C>
	function_definer<C> def(C * this_p) {
		return function_definer<C>(this_p, this);
	}

	template <typename C, typename ...Ts> struct call_constructor {
		static C * doit(Ts... ts) {
			return new C(ts...);
		}
	};

	template <class C, typename ...Ts> class_definer<C> class_(std::string const &name, init<Ts...> const & = init<Ts...>(), policies const &p = policies()) {
		typedef details::callback_v<details::no_class, C * (*)(Ts...), C *, Ts...> callback_type;
		
		std::shared_ptr<details::class_handler<C>> ch(new details::class_handler<C>());

		add_class(name, ch);

		add_constructor(name, ch, new callback_type(this, &call_constructor<C, Ts...>::doit, name, p), p);

		return class_definer<C>(ch, this, false);
	}

#define CPPTCL_MANAGED_CLASS
#ifdef CPPTCL_MANAGED_CLASS
	template <typename C, typename ...Ts> struct call_managed_constructor {
		interpreter * interp_;
		call_managed_constructor(interpreter * i) : interp_(i) { }

		object operator()(Ts... ts);
	};

	template <class C, typename ...Ts> class_definer<C> managed_class_(std::string const &name, init<Ts...> const & = init<Ts...>(), policies const &p = policies()) {
		typedef details::callback_v<details::no_class, call_managed_constructor<C, Ts...>, object, Ts...> callback_type;
		
		std::shared_ptr<details::class_handler<C>> ch(new details::class_handler<C>());

		this->type<type_ops<C> >(name, (C *) 0);
		
		add_class(name, ch);

		add_managed_constructor(name, ch, new callback_type(this, call_managed_constructor<C, Ts...>(this), name, p), p);

		return class_definer<C>(ch, this, true);
	}
#endif

	std::map<std::string, Tcl_ObjType *> obj_type_by_tclname_;
	std::map<std::type_index, Tcl_ObjType *> obj_type_by_cppname_;

	template <typename T>
	struct deleter {
		virtual bool free(Tcl_Obj * o) {
			delete (T *) o->internalRep.twoPtrValue.ptr1;
			return false;
		}
		virtual deleter * dup_deleter() { return this; }
		virtual void * dup(void * obj) {
			return (void *) new T(*((T *) obj));
		}
		virtual ~deleter() { }
	};

	template <typename T>
	struct dyn_deleter : public deleter<T> {
		bool free(Tcl_Obj * o) override {
			//std::cerr << "REF#: " << o->refCount << "\n";
			delete (T *) o->internalRep.twoPtrValue.ptr1;
			return true;
		}
		deleter<T> * dup_deleter() override { return new dyn_deleter(); }
	};

	template <typename T>
	struct shared_ptr_deleter : public deleter<T> {
		std::shared_ptr<T> p_;

		shared_ptr_deleter(T * p) : p_(p) { }

		template <typename DEL>
		shared_ptr_deleter(T * p, DEL del) : p_(p, del) {
			//std::cerr << "custom\n";
		}
		shared_ptr_deleter(std::shared_ptr<T> & p) : p_(p) {
			//std::cerr << "copy\n";
		}
		bool free(Tcl_Obj * o) override {
			return true;
		}
		virtual void * dup(void * obj) override { return obj; }
		deleter<T> * dup_deleter() override {
			//std::cerr << "dup\n";
			return new shared_ptr_deleter(p_);
		}
		~shared_ptr_deleter() {
			//std::cerr << "~shptr deleter\n";
		}
	};
	
	template <typename T>
	struct type_ops {
		static void dup(Tcl_Obj * src, Tcl_Obj * dst) {
			//2p dst->internalRep.otherValuePtr = src->internalRep.otherValuePtr;
			//std::cerr << "dup " << src->typePtr << " " << (src->typePtr ? src->typePtr->name : "()" ) << "\n";
			if (src->internalRep.twoPtrValue.ptr2) {
				//std::cerr << "dup osrc=" << src << "[" << src->refCount << "] odst=" << dst << "[" << dst->refCount << "]\n";// type=" << src->typePtr << " intern=" << src->internalRep.otherValuePtr << "\n";
				deleter<T> * d = (deleter<T> *) src->internalRep.twoPtrValue.ptr2;
				dst->internalRep.twoPtrValue.ptr1 = d->dup(src->internalRep.twoPtrValue.ptr1);
				dst->internalRep.twoPtrValue.ptr2 = (( deleter<T> *)src->internalRep.twoPtrValue.ptr2)->dup_deleter();
			} else {
				dst->internalRep.twoPtrValue.ptr1 = src->internalRep.twoPtrValue.ptr1;
				dst->internalRep.twoPtrValue.ptr2 = nullptr;
			}
			//src->internalRep.twoPtrValue.ptr2 = nullptr;
			dst->typePtr = src->typePtr;
		}
		static void free(Tcl_Obj * o) {
			if (o->internalRep.twoPtrValue.ptr2) {
				//std::cerr << "free " << o << "\n";
				deleter<T> * d = (deleter<T> *) o->internalRep.twoPtrValue.ptr2;
				if (d->free(o)) {
					delete d;
				}
			}
		}
		static void str(Tcl_Obj * o) {
			std::ostringstream oss;
			oss << 'p' << o->internalRep.twoPtrValue.ptr1;
			str_impl(o, oss.str());
		}
		static void str_impl(Tcl_Obj * o, std::string const & name) {
			o->bytes = Tcl_Alloc(name.size() + 1);
			strncpy(o->bytes, name.c_str(), name.size());
			o->bytes[name.size()] = 0;
			o->length = name.size();
		}
		static int set(Tcl_Interp *, Tcl_Obj *) {
			//std::cerr << "type set\n";
			return TCL_OK;
		}
	};

	template <typename TY>
	Tcl_ObjType * get_objtype() {
		//std::cerr << "get_objtype\n";
		auto it = obj_type_by_cppname_.find(std::type_index(typeid(TY)));
		if (it != obj_type_by_cppname_.end()) {
			return it->second;
		}
		return nullptr;
	}
	Tcl_ObjType * get_objtype(const char * name) {
		//std::cerr << "get_objtype\n";
		auto & o = obj_type_by_tclname_;
		auto it = o.find(name);
		return it == o.end() ? nullptr : it->second;
	}
	template <typename T>
	bool is_type(Tcl_Obj * o) {
		//std::cerr << "is_type\n";
		return o->typePtr && get_objtype<T>() == o->typePtr;
		if (o->typePtr) {
			std::type_index * tip = (std::type_index *) (o->typePtr->name + 256);
			if (std::type_index(typeid(T)) == *tip) {
				return true;
			}
		}
		return false;
	}
	template <typename T>
	T * try_type(Tcl_Obj * o) {
		//std::cerr << "try_type\n";
		if (is_type<T>(o)) {
			//2p return (T *) o->internalRep.otherValuePtr;
			return (T *) o->internalRep.twoPtrValue.ptr1;
		}
		return nullptr;
	}
	
	template <typename OPS, typename TY>
	void type(std::string const & name, TY * p) {
		auto it = obj_type_by_cppname_.find(std::type_index(typeid(TY)));
		if (it != obj_type_by_cppname_.end()) {
			std::cerr << "attempt to register type " << name << " twice\n";
			return;
		}
		auto it2 = obj_type_by_tclname_.find(name);
		if (it2 != obj_type_by_tclname_.end()) {
			std::cerr << "attempt to register type " << name << " twice\n";
		}
		Tcl_ObjType * ot = new Tcl_ObjType;
		//char * cp = new char[name.size() + 1 + sizeof(std::type_index)];
		char * cp = new char[256 + sizeof(std::type_index)];
		strcpy(cp, name.c_str());
		cp[name.size()] = 0;
		std::type_index * tip = (std::type_index *) (cp + 256);
		*tip = std::type_index(typeid(TY));
		ot->name = cp;
		ot->freeIntRepProc   = &OPS::free;
		ot->dupIntRepProc    = &OPS::dup;
		ot->updateStringProc = &OPS::str;
		ot->setFromAnyProc   = &OPS::set;
		obj_type_by_tclname_.insert(std::pair<std::string, Tcl_ObjType *>(name, ot));
		obj_type_by_cppname_.insert(std::pair<std::type_index, Tcl_ObjType *>(std::type_index(typeid(TY)), ot));
		Tcl_RegisterObjType(ot);
	}

	template <typename T, typename std::enable_if<!std::is_same<T, typename details::list_unpack<T>::type>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp *, Tcl_Obj *, bool) {
		throw;
	}
	template <typename T, typename std::enable_if<!std::is_same<T, typename details::list_unpack<T>::type>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp *, Tcl_Obj *, bool, Tcl_ObjType *) {
		throw;
	}

	template <typename T>
	T tcl_cast(Tcl_Obj * o) {
		return this->template tcl_cast<T>(this->get_interp(), o, false);
	}
	template <typename T>
	T tcl_cast(Tcl_Obj * o, Tcl_ObjType * ot) {
		return this->template tcl_cast<T>(this->get_interp(), o, false, ot);
	}

	template <typename T, typename std::enable_if<std::is_pointer<T>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp * i, Tcl_Obj * o, bool byref) {
		auto it = obj_type_by_cppname_.find(std::type_index(typeid(typename std::remove_pointer<T>::type)));
		if (it != obj_type_by_cppname_.end()) {
			//std::cerr << "castingold...\n";
			return this->template tcl_cast<T>(i, o, byref, it->second);
		}
		throw tcl_error("function argument type is not registered");
	}
	
	template <typename T, typename std::enable_if<std::is_pointer<T>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp * i, Tcl_Obj * o, bool byref, Tcl_ObjType * ot) {
		if (std::is_pointer<T>::value && o->typePtr) {
			//auto it = obj_type_by_cppname_.find(std::type_index(typeid(typename std::remove_pointer<T>::type)));
			//if (it != obj_type_by_cppname_.end()) {
			if (true) {
				// Tcl_ObjType * otp = it->second;
				Tcl_ObjType * otp = ot;
				//std::cerr << "casting... " << otp << " " << o->typePtr << " " << o->typePtr->name << "\n";

				if (otp == o->typePtr) {
					//return (typename std::conditional<std::is_pointer<T>::value, T, T*>::type) o->internalRep.otherValuePtr;
					//std::cerr << "objcast " << o << " " << o->internalRep.otherValuePtr << "\n";
					//return (T) o->internalRep.otherValuePtr;
					return (T) o->internalRep.twoPtrValue.ptr1;//otherValuePtr;
				} else if (o->typePtr && strcmp(o->typePtr->name, "list") == 0) {
					int len;
					if (Tcl_ListObjLength(i, o, &len) == TCL_OK) {
						if (len == 1) {
							Tcl_Obj *o2;
							if (Tcl_ListObjIndex(i, o, 0, &o2) == TCL_OK) {
								if (otp == o2->typePtr) {
									//std::cerr << "objcast(list=len1) " << o2 << " " << o2->internalRep.otherValuePtr << "\n";
									//2p return (T) o2->internalRep.otherValuePtr;
									return (T) o2->internalRep.twoPtrValue.ptr1;
								}
							}
						} else {
							throw tcl_error("Expected single object, got list");
						}
					} else {
						throw tcl_error("Unknown tcl error");
					}
				} else {
					throw tcl_error("function argument has wrong type");
				}
			} else {
				throw tcl_error("function argument type is not registered");
			}
		} else {
			throw tcl_error("function argument is not a tcl object type");
		}
		return nullptr;
		return details::tcl_cast<T>::from(i, o, byref);
		//return nullptr;
	}
	template <typename T, typename std::enable_if<!std::is_pointer<T>::value && std::is_same<T, typename details::list_unpack<T>::type>::value && !details::is_any<T>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp * i, Tcl_Obj * o, bool byref) {
		return details::tcl_cast<T>::from(i, o, byref);
	}
	template <typename T, typename std::enable_if<!std::is_pointer<T>::value && std::is_same<T, typename details::list_unpack<T>::type>::value && !details::is_any<T>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp * i, Tcl_Obj * o, bool byref, Tcl_ObjType *) {
		return details::tcl_cast<T>::from(i, o, byref);
	}

	template <class C> class_definer<C> class_(std::string const &name, details::no_init_type const &) {
		std::shared_ptr<details::class_handler<C>> ch(new details::class_handler<C>());

		add_class(name, ch);

		return class_definer<C>(ch, this, false);
	}

	template <class C> class_definer<C> managed_class_(std::string const &name, details::no_init_type const &) {
		std::shared_ptr<details::class_handler<C>> ch(new details::class_handler<C>());
		
		add_class(name, ch);
		
		return class_definer<C>(ch, this, true);
	}

	
	// free script evaluation
	details::result eval(std::string const &script);
	details::result eval(std::istream &s);

	details::result eval(object const &o);

	void result_reset() {
		Tcl_ResetResult(get_interp());
	}
	
	// the InputIterator should give object& or Tcl_Obj* when dereferenced
	template <class InputIterator> details::result eval(InputIterator first, InputIterator last);


	template <typename OT, typename DEL>
	object makeobj(OT * n, bool owning = false, DEL delfn = []{});
	template <typename OT>
	object makeobj(OT * n, bool owning = false);
	template <typename OT, typename DEL>
	void makeobj_inplace(OT * p, object & obj, bool owning = false, DEL delfn = []{});
	
	
	template <typename T>
	void setVar(std::string const & name, T const & value);
	void setVar(std::string const & name, object const & value);

	template <typename OT>
	void setVar(std::string const & name, object & ptr);

	void unsetVar(std::string const & name);
	
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

	void add_function(std::string const &name, details::callback_base * cb);

	void add_class(std::string const &name, std::shared_ptr<details::class_handler_base> chb);

	void add_constructor(std::string const &name, std::shared_ptr<details::class_handler_base> chb, details::callback_base * cb, policies const &p = policies());
	void add_managed_constructor(std::string const &name, std::shared_ptr<details::class_handler_base> chb, details::callback_base * cb, policies const &p = policies());

	Tcl_Interp *interp_;
	bool owner_;
	std::iostream tin_, tout_, terr_;
};

template <typename T>
std::size_t list<T>::size() const {
	if (! interp_) return 0;
	int len;
	if (Tcl_ListObjLength(interp_->get(), lo_, &len) == TCL_OK) {
		return len;
	}
	return 0;
}

template <typename T>
T variadic<T>::at(int ix) const { return interp_->template tcl_cast<T>(objv[ix]); }

template <typename T>
typename list<T>::return_t list<T>::at(std::size_t ix) const {
	Tcl_Obj *o;
	if (Tcl_ListObjIndex(interp_->get(), lo_, ix, &o) == TCL_OK) {
		if constexpr (isany) {
			return return_t(interp_, o, ot_);
		} else if constexpr (details::is_basic_type<T>::value) {
			return interp_->template tcl_cast<T>(o);
		} else {
			if (o->typePtr == ot_[0]) {
				//return (return_t) o->internalRep.otherValuePtr;
				return (return_t) o->internalRep.twoPtrValue.ptr1;
			} else {
				throw tcl_error("Unexpected type in list. Got " + details::tcl_typename(o) + " need " + ot_[0]->name);
			}
		}
	}
	return return_t();
}

template <typename ...Ts>
template <typename TT>
typename std::conditional<details::is_list<TT>::value, TT, TT *>::type any<Ts...>::as() const {
	//if (this->interp_->try_type<T>(this->o_->typePtr)) {
#if 0
	if (this->which_ == details::type_at<TT, Ts...>::value) {
		if constexpr (details::is_list<TT>::value) {
			if (this->o_->typePtr && strcmp(this->o_->typePtr->name, "list") == 0) {
				return TT(interp_, this->o_, interp_->get_objtype<typename details::list_unpack<TT>::type>());
			} else {
				return TT();
				//throw tcl_error(std::string("bad cast of ") + (this->o_->typePtr ? this->o_->typePtr->name : "(none)") + " to list");
			}
		} else {
			//return (TT *) this->o_->internalRep.otherValuePtr; //o_->internalRep.otherValuePtr;
			return (TT *) this->o_->internalRep.twoPtrValue.ptr1; //o_->internalRep.otherValuePtr;
		}
	}
	if constexpr (details::is_list<TT>::value) {
		return TT();
	} else {
		return nullptr;
	}
#else
	const int toff = sizeof...(Ts) - 1 - details::type_at<TT, Ts...>::value;

	if constexpr (details::is_list<TT>::value) {
		if (this->o_ && this->o_->typePtr && strcmp(this->o_->typePtr->name, "list") == 0) {
			Tcl_Obj * oo;
			if (Tcl_ListObjIndex(interp_->get_interp(), this->o_, 0, &oo) == TCL_OK) {
#if 0
				std::cerr << sizeof...(Ts) << " " << details::type_at<TT, Ts...>::value << " " << details::tcl_typename(oo) << "\n";
				for (int i = 0; i < sizeof...(Ts); ++i) {
					std::cerr << i << " " << this->ot_[i]->name << "\n";
				}
#endif
				if (oo->typePtr == this->ot_[toff]) {
					return TT(interp_, this->o_, this->ot_ + toff);
				}
			}
			return TT();
		} else {
			return TT();
			//throw tcl_error(std::string("bad cast of ") + (this->o_->typePtr ? this->o_->typePtr->name : "(none)") + " to list");
		}
	} else {
		//std::cerr << this->o_->typePtr << " " << this->o_->typePtr->name << " " << details::type_at<TT, Ts...>::value << " " << this->ot_ << "\n";
		if (this->o_ && this->ot_[toff] == this->o_->typePtr) {
			return (TT *) this->o_->internalRep.twoPtrValue.ptr1;
		} else {
			return nullptr;
		}
	}
#endif
}

namespace details {
template <int I, typename TArg, typename ...Ts>
struct generate_argtypes<I, TArg, Ts...> {
	template <typename WRAPT>
	struct wrap { };

	typedef typename remove_rc<TArg>::type T;
	
	template <typename TT>
	static Tcl_ObjType * lookup_type(interpreter * interp, int pos, wrap<TT> * np) {
		auto * ot = interp->get_objtype<typename std::remove_pointer<TT>::type>();
		//std::cerr << "lookup " << pos << " " << "_Z" << typeid(TT).name() << " " << ot << " " << (ot ? ot->name : "()") << "\n";
		return ot;
		//return interp->get_objtype<TT>();
	}
	template <std::size_t ...Is>
	static void invoke_helper(std::index_sequence<Is...> const &, interpreter * interp, Tcl_ObjType ** all) {
		((all[Is] = lookup_type(interp, Is, (wrap<typename subtype<Is, T>::type> *) 0)), ...);
		//p[0] = all;
		//p[1] = all + sizeof...(is);
		generate_argtypes<I + 1, Ts...>::invoke(interp, all + sizeof...(Is));
	}
	static void invoke(interpreter * interp, Tcl_ObjType ** all) {
		//std::cerr << "gat _Z" << typeid(T).name() << " " << num_subtypes<T>::value << "\n";
		invoke_helper(std::make_index_sequence<num_subtypes<T>::value>(), interp, all);
	}

	template <int ArgI>
	static std::pair<Tcl_ObjType **, Tcl_ObjType **> get(Tcl_ObjType ** p) {
		if constexpr (ArgI == I) {
			return { p, p + num_subtypes<T>::value };
		} else {
			return generate_argtypes<I + 1, Ts...>::template get<ArgI>(p + num_subtypes<T>::value);
		}
	}

	static const int length = num_subtypes<T>::value + generate_argtypes<I + 1, Ts...>::length;
};


template <typename T, typename ...Ts>
inline void any_impl<T, Ts...>::set_which(interpreter * i, Tcl_Obj * o) {
	if constexpr (is_list<T>::value) {
		if (o->typePtr && strcmp(o->typePtr->name, "list") == 0) {
			Tcl_Obj *oo;
			if (Tcl_ListObjIndex(i->get(), o, 0, &oo) == TCL_OK) {
				if (i->is_type<typename list_unpack<T>::type>(oo)) {
					this->which_ = sizeof...(Ts);
					return;
				}
			}
		} else {
			if (i->is_type<typename list_unpack<T>::type>(o)) {
				this->which_ = sizeof...(Ts);
				return;
			}
		}
		any_impl<Ts...>::set_which(i, o);
	} else {
		if (i->is_type<T>(o)) {
			this->which_ = sizeof...(Ts);
		} else {
			any_impl<Ts...>::set_which(i, o);
		}
	}
}

template <typename ...Ts>
struct back {
	typedef void type;
};

template <typename T, typename ...Ts>
struct back<T, Ts...> {
	using type = typename std::conditional<sizeof...(Ts) == 0, T, typename back<Ts...>::type>::type;
};

template <typename Cparm, typename Fn, typename R, typename ...Ts>
void callback_v<Cparm, Fn, R, Ts...>::invoke(void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv [], bool object_dot_method) {
	static const int num_gopt = num_getopt<Ts...>::value;
	static const int num_opt    = num_optional<Ts...>::value;
	Tcl_Obj * getopt_argv[num_gopt + 1] = { nullptr };
	Tcl_Obj boolean_dummy_object;
	std::size_t ai = std::is_same<C, no_class>::value || object_dot_method ? 1 : 2;
	static const bool has_variadic_ = has_variadic<Ts...>::value;
	
	if (interpreter_->trace()) {
		for (int i = 0; i < argc; ++i) {
			std::cerr << " ";
			if (argv[i]->typePtr && strcmp(argv[i]->typePtr->name, "list") == 0) {
				std::cerr << '{' << Tcl_GetString(argv[i]) << '}';
			} else {
				std::cerr << Tcl_GetString(argv[i]);
			}
		}
		std::cerr << "\n";
	}
	++interpreter_->trace_count_;
	
	if (argc == ai + 1 && strcmp(Tcl_GetString(argv[ai]), "-help") == 0 && std::find(opts_, opts_ + num_gopt, "help") == opts_ + num_gopt) {
		if (num_gopt) {
			std::cerr << Tcl_GetString(argv[0]);
			for (int oi = 0; oi < num_gopt; ++oi) {
				interpreter_->terr() << " -" << opts_[oi] << (has_arg_[oi] ? " <arg>" : "");
			}
			
			if (policies_.variadic_) {
				interpreter_->terr() << " objects...";
			}
			interpreter_->terr() << "\n";
		}
		return;
	}
	
	if (num_gopt) {
		for (; ai < argc; ++ai) {
			//if (argv[ai]->typePtr) break;
			char * s = Tcl_GetString(argv[ai]);
			//std::cerr << "look at: " << s << "\n";
			if (s[0] == '-') {
				if (s[1] == '-' && s[2] == 0) {
					++ai;
					break;
				}
				bool found = false;
				for (int oi = 0; oi < num_gopt; ++oi) {
					if (opts_[oi].compare(s + 1) == 0) {
						found = true;
						//std::cerr << s << " at " << ai << " mapped to " << oi << " has_arg " << has_arg_[oi] << "\n";
						if (has_arg_[oi]) {
							++ai;
							if (ai >= argc) {
								throw tcl_error(std::string("option requires an argument: ") + s);
							}
							getopt_argv[oi] = argv[ai];
						} else {
							getopt_argv[oi] = &boolean_dummy_object;
						}
					}
				}
				if (!found) {
					throw tcl_error(std::string("unknown option: ") + s + " at index " + std::to_string(ai));
				}
			} else {
				break;
			}
		}
	}
	check_params_no(argc - ai, sizeof...(Ts) - (has_variadic_ || policies_.variadic_ ? 1 : 0) - num_gopt - num_opt, has_variadic_ || policies_.variadic_ ? -1 : sizeof...(Ts) - num_gopt, policies_.usage_);
	try {
		do_invoke(std::make_index_sequence<sizeof... (Ts)>(), (C *) pv, interp, argc - ai, argv + ai, num_gopt, getopt_argv, policies_, void_return<std::is_same<R, void>::value>());
	} catch (tcl_usage_message_printed & e) {
	} catch (std::exception & e) {
		for (int i = 0; i < argc; ++i) {
			if (i) { std::cerr << " "; }
			if (argv[i]->typePtr && strcmp(argv[i]->typePtr->name, "list") == 0) {
				std::cerr << '{' << Tcl_GetString(argv[i]) << '}';
			} else {
				std::cerr << Tcl_GetString(argv[i]);
			}
		}
		std::cerr << ": " << e.what() << "\n";
		return;
	} catch (...) {//std::exception & e) {
		throw;
	}
	post_process_policies(interp, policies_, argv + ai, false);
}
}

#include "cpptcl/cpptcl_object.h"

#ifdef CPPTCL_MANAGED_CLASS
template <typename C, typename ...Ts>
object interpreter::call_managed_constructor<C, Ts...>::operator()(Ts... ts) {
	object ret = interp_->makeobj(new C(ts...), true, [this](C * p) {
									  std::ostringstream oss;
									  oss << "p" << p << ".";
									  Tcl_DeleteCommand(interp_->get_interp(), oss.str().c_str());
									  //std::cerr << oss.str() << " removed\n";
									  delete p;
								  });
	if (false) {
		std::cerr << "managed_create " << ret.get_object()
				  << " " << ret.get_object()->internalRep.twoPtrValue.ptr1
				  << " " << ret.get_object()->internalRep.twoPtrValue.ptr2
				  << "\n";
	}
	return ret;
}
#endif

namespace details {
template <int ArgOffset, typename TTArg, std::size_t In, std::size_t Ii, typename ...Ts>
typename details::fix_variadic_return<TTArg, Ii + 1 == In>::type do_cast(interpreter * tcli, std::pair<Tcl_ObjType **, Tcl_ObjType **> objecttypes, ppack<Ts...> pack, bool variadic, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[], int getoptc, Tcl_Obj * CONST getoptv[], policies const & pol) {
	typedef typename remove_rc<TTArg>::type TT;

	static const bool is_variadic_arg = is_variadic<TT>::value;
	static const bool is_optional_arg = is_optional<TT>::value;
	static const bool is_getopt_arg   = is_getopt<TT>::value;

	static const bool is_old_variadic_arg = Ii + 1 == In && std::is_same<typename back<Ts...>::type, object const &>::value;

	typedef typename list_unpack<TT>::type list_unpack_t;
	static const bool is_list_arg = !std::is_same<TT, list_unpack_t>::value;
	static const bool cpptcl_byref = false;

	static const bool is_any_arg = is_any<TT>::value;
	static const bool is_any_list_arg = is_any_arg && all_lists<TT>::value;
	
	typedef typename optional_unpack<TT>::type opt_unpack_t;

	//std::cerr << "do_cast argi=" << Ii << "/" << In << " argoff=" << ArgOffset << " objc=" << objc << " " << typeid(TTArg).name() << "\n";
	if constexpr (is_old_variadic_arg) {
		if (pol.variadic_) {
			return details::get_var_params(tcli->get(), objc, objv, Ii + ArgOffset, pol);
		}
	}
		
	if constexpr (is_optional_arg || is_getopt_arg) {
		if constexpr (is_getopt_arg) {
			static const int getopti = getopt_index<0, Ii, 0, Ts...>::value;
			//std::cerr << "getopti " << getopti << " " << getoptv[getopti] << "\n";

			if (getoptv[getopti]) {
				if constexpr (std::is_same<TT, getopt<bool> >::value) {
					//std::cerr << "truebool\n";
					return TT(true, true);
				} else {
					//std::cerr << "optional\n";
					return TT(do_cast<0, opt_unpack_t, In, Ii>(tcli, objecttypes, pack, variadic, interp, getoptc, getoptv, getoptc, getoptv, pol), true);
					//return TT(tcli->template tcl_cast<opt_unpack_t>(interp, getoptv[getopti], cpptcl_byref), true);
				}
			} else {
				//std::cerr << "falsebool\n";
				return TT(opt_unpack_t(), false);
			}				
		} else {
			if (objc > Ii + ArgOffset) {
				return TT(do_cast<ArgOffset, opt_unpack_t, In, Ii>(tcli, objecttypes, pack, variadic, interp, objc, objv, getoptc, getoptv, pol), true);
				//return { tcli->template tcl_cast<opt_unpack_t>(interp, objv[Ii + ArgOffset], cpptcl_byref), true };
			} else {
				return { typename std::decay<opt_unpack_t>::type(), false };
			}
		}
	} else if constexpr (is_list_arg) {
		if (objv[Ii + ArgOffset]->typePtr) {
			Tcl_ObjType * ot = tcli->get_objtype<list_unpack_t>();
			if (strcmp(objv[Ii + ArgOffset]->typePtr->name, "list") == 0) {
				return TT(tcli, objv[Ii + ArgOffset], objecttypes.first);
				//ot return TT(tcli, objv[Ii + ArgOffset], ot);
				//return return_dummy_value<TT, !is_list_arg>::doit(tcli, objv[Ii + ArgOffset], ot);
			} else {
				Tcl_Obj * lo = Tcl_NewListObj(1, objv + Ii + ArgOffset);
				//std::cerr << "create listone " << Tcl_GetString(objv[Ii + ArgOffset]) << " " << (ot ? ot->name : "") << "\n";
				return TT(tcli, lo, objecttypes.first, true);
				//ot return TT(tcli, lo, ot, true);
				//return return_dummy_value<TT, !is_list_arg>::doit(tcli, lo, ot);
			}
		} else {
			return TT(nullptr, nullptr, nullptr);
			//throw tcl_error("expecting object at index" + std::to_string(Ii + ArgOffset));
		}
	} else if constexpr (is_any_arg) {
		if (objv[Ii + ArgOffset]->typePtr) {
			if (strcmp(objv[Ii + ArgOffset]->typePtr->name, "list") == 0) {
				if constexpr (is_any_list_arg) {
					int len;
					if (Tcl_ListObjLength(interp, objv[Ii + ArgOffset], &len) == TCL_OK) {
						if (len == 0) {
							return TT(nullptr, nullptr, nullptr);
						} else {
							return TT(tcli, objv[Ii + ArgOffset], objecttypes.first);
						}
					} else {
						throw tcl_error("cannot query list length");
					}
				} else {
					int len;
					if (Tcl_ListObjLength(interp, objv[Ii + ArgOffset], &len) == TCL_OK) {
						if (len == 1) {
							Tcl_Obj * oo;
							if (Tcl_ListObjIndex(interp, objv[Ii + ArgOffset], 0, &oo) == TCL_OK) {
								return TT(tcli, oo, objecttypes.first);
							} else throw tcl_error("cannot get list element");
						} else throw tcl_error("expecting object or list of size one, got list of size " + std::to_string(len));
					} else throw tcl_error("cannot get list size");
					throw tcl_error("cannot convert list");
				}
			} else {
				if (is_any_list_arg) {
					Tcl_Obj * lo = Tcl_NewListObj(1, objv + Ii + ArgOffset);
					return TT(tcli, lo, objecttypes.first, true);
				} else {
					return TT(tcli, objv[Ii + ArgOffset], objecttypes.first);
				}
			}
			//return return_dummy_value<TT, !is_any_arg>::doit(ret.set(tcli, objv[Ii + ArgOffset]));
		} else {
			if constexpr (is_any_list_arg) {
				return TT(nullptr, nullptr, nullptr);
			} else {
				throw tcl_error("any<> argument does not map to an object");
			}
			//return return_dummy_value<TT, !is_any_arg>::doit(interp, nullptr);
			//return TT(nullptr, nullptr);
		}
	} else {
		if constexpr (is_variadic_arg) {
			//return details::get_var_params(interp, objc, objv, In + ArgOffset - 1, pol);
			//return details::return_dummy_value<TT, !is_variadic_arg>::doit(details::get_var_params(interp, objc, objv, In + ArgOffset - 1, pol));
			return TT(tcli, objc, objv);
		} else {
			//} else {
			//return details::tcl_cast<TT>::from(interp, objv[Ii + ArgOffset], cpptcl_byref);
			//}
			//std::cerr << "regular arg " << Tcl_GetString(objv[Ii + ArgOffset]) << " " << objv[Ii + ArgOffset] << " " << Ii << " " << ArgOffset << " " << objv[Ii + ArgOffset]->typePtr << "[" << (objv[Ii + ArgOffset]->typePtr ? objv[Ii + ArgOffset]->typePtr->name : "") << "]\n";
			//return details::tcl_cast<TT>::from(interp, objv[Ii + ArgOffset], false);
			if constexpr (std::is_pointer<opt_unpack_t>::value && std::is_class<typename std::remove_pointer<opt_unpack_t>::type>::value) {
				return tcli->template tcl_cast<typename std::remove_cv<typename std::remove_reference<opt_unpack_t>::type>::type>(interp, objv[Ii + ArgOffset], false, *objecttypes.first);
			} else {
				return tcli->template tcl_cast<typename std::remove_cv<typename std::remove_reference<opt_unpack_t>::type>::type>(interp, objv[Ii + ArgOffset], false);
			}
		}
	}
}
}

template <typename T>
void interpreter::defvar(std::string const & name, T & v) {
	def(name, [&v] (opt<T> const & arg) -> T {
			if (arg) {
				v = *arg;
			}
			return v;
		});
}

template <typename C>
template <typename T>
interpreter::function_definer<C> & interpreter::function_definer<C>::defvar(std::string const & name, T C::*t) {
	return def(name, [t] (opt<T> const & arg) -> T {
				   if (arg) {
					   *t = arg;
				   }
				   return *t;
			   });
}

template <typename C>
template <typename T>
interpreter::class_definer<C> & interpreter::class_definer<C>::defvar(std::string const & name, T C::*varp) {
	return this->def(name, [varp] (C * this_p, opt<T> const & val) -> T {
						 if (val) this_p->*varp = *val;
						 return this_p->*varp;
					 });
}

inline void interpreter::setVar(std::string const & name, object const & value) {
	object name_obj(name);

#if 0
	Tcl_Obj * ro2 = Tcl_ObjGetVar2(interp_, name_obj.get_object(), nullptr, 0);
	Tcl_Obj * ro;
	{
		object value2 = value.duplicate();
		std::cerr << "sharedo1: " << value.shared() << " " << name_obj.shared() << " " << value.get_object()->refCount << " " << value2.get_object() << " " << ro2 << " " << (ro2 ? ro2->refCount : -22) << "\n";
		ro = Tcl_ObjSetVar2(interp_, name_obj.get_object(), nullptr, value2.get_object(), 0);
	}

	std::cerr << "sharedo2: " << value.shared() << " " << name_obj.shared() << " " << ro << " " << ro2 << " " << value.get_object() << " " << ro->refCount << " " << value.get_object()->refCount << "\n";
#else
	Tcl_ObjSetVar2(interp_, name_obj.get_object(), nullptr, value.get_object(), 0);
#endif
}

template <typename T>
void interpreter::setVar(std::string const & name, T const & value) {
	object val_obj(value);
	object name_obj(name);

	std::cerr << "shared1: " << val_obj.shared() << " " << name_obj.shared() << "\n";
	Tcl_ObjSetVar2(interp_, name_obj.get_object(), nullptr, val_obj.get_object(), 0);
	std::cerr << "shared2: " << val_obj.shared() << " " << name_obj.shared() << "\n";
}

template <typename OT, typename DEL>
void interpreter::makeobj_inplace(OT * n, object & obj, bool owning, DEL delfn) {
	Tcl_Obj * o = obj.get_object(); //Tcl_NewObj();
	//Tcl_Obj * o = Tcl_NewObj();

	if (Tcl_IsShared(o)) {
		throw tcl_error("cannot modify shared object in-place");
	}
	
	o->internalRep.twoPtrValue.ptr1 = (void *) n;
	if (owning) {
		if constexpr (std::is_same<DEL, void *>::value) {
			if (delfn) { }
			o->internalRep.twoPtrValue.ptr2 = new interpreter::shared_ptr_deleter<OT>(n); //new interpreter::dyn_deleter<OT>(); //nullptr;
		} else {
			o->internalRep.twoPtrValue.ptr2 = new interpreter::shared_ptr_deleter<OT>(n, delfn); //new interpreter::dyn_deleter<OT>(); //nullptr;
		}
		//o->internalRep.twoPtrValue.ptr2 = new interpreter::dyn_deleter<OT>(); //nullptr;
	} else {
		o->internalRep.twoPtrValue.ptr2 = nullptr;
	}
	
	auto it = this->obj_type_by_cppname_.find(std::type_index(typeid(OT)));
	bool ok = it != this->obj_type_by_cppname_.end();
	if (ok) {
		o->typePtr = it->second;
	} else {
		throw tcl_error("type lookup failed");
	}
	Tcl_InvalidateStringRep(o);
	//return object(o, true);
}
template <typename OT, typename DELFN>
object interpreter::makeobj(OT * p, bool owning, DELFN delfn) {
	object obj = object().duplicate();
	makeobj_inplace(p, obj, owning, delfn);
	return obj;
}

template <typename OT>
object interpreter::makeobj(OT * n, bool owning) {
	return makeobj(n, owning, (void *) 0 );
}
template <typename OT>
void interpreter::setVar(std::string const & name, object & obj) {
	object name_obj(name);
	Tcl_ObjSetVar2(interp_, name_obj.get_object(), nullptr, obj.get_object(), 0);
}


template <typename R, typename ...Ts>
struct Bind {
	object cmd_;

	template <typename ...TTs>
	void dummy(TTs...) {
	}
	
	Bind(std::string const & cmd) : cmd_(object(cmd)) { }
	R operator()(Ts... args) {
		object e(cmd_);

		dummy(e.append(object(args))...);
		return (R) (interpreter::getDefault()->eval(e));
	}
};

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
