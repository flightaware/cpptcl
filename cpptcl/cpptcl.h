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

#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
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
#include <optional>

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
	std::string msg_;
	void * backtrace_[16];
	size_t backtrace_size_;
public:
	explicit tcl_error(std::string const &msg); //: std::runtime_error(msg) {}
	explicit tcl_error(Tcl_Interp *interp);
	const char * what() const throw();
};

class tcl_aborted : public std::exception {
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
	bool has_postprocess() {
		return !factory_.empty() || ! sinks_.empty();
	}
};

// syntax short-cuts
policies factory(std::string const &name);
policies sink(int index);
policies usage(std::string const &message);
policies options(std::string const & options);

class interpreter;
class object;

struct named_pointer_result {
	std::string name;
	void * p;
	template <typename U>
	named_pointer_result(std::string const & n, U * u) : name(n), p((void *) u) { }
};

namespace details {
template <typename T> struct tcl_cast;

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
template <> struct tcl_cast<unsigned int> { static unsigned int from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };
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
	//result(interpreter * interp);
	
	operator bool() const;
	operator double() const;
	operator int() const;
	operator long() const;
	operator std::string() const;
	operator object() const;
	void reset();
  private:
	Tcl_Interp *interp_;
	//interpreter * interp_;
};

void set_result(Tcl_Interp *interp, bool b);
void set_result(Tcl_Interp *interp, int i);
void set_result(Tcl_Interp *interp, long i);
void set_result(Tcl_Interp *interp, double d);
void set_result(Tcl_Interp *interp, std::string const &s);
void set_result(Tcl_Interp *interp, void *p);
void set_result(Tcl_Interp *interp, object const &o);
void set_result(Tcl_Interp *interp, named_pointer_result);

template <typename Iter, typename std::enable_if<!std::is_same<typename std::iterator_traits<Iter>::value_type, void>::value, bool>::type = true>
void set_result(Tcl_Interp * interp, std::pair<Iter, Iter> it);

template <typename T>
void set_result(Tcl_Interp * interp, std::vector<T> const & v) {
	set_result(interp, std::pair(v.begin(), v.end()));
}

// helper for checking for required number of parameters
// (throws tcl_error when not met)
void check_params_no(int objc, int required, int maximum, const std::string &message);

// helper for gathering optional params in variadic functions
object get_var_params(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int from, policies const &pol);

// the callback_base is used to store callback handlers in a polynorphic map
class callback_base {
  public:
	virtual ~callback_base() {}

	virtual void invoke(interpreter *, void * p, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], bool object_dot_method) = 0;
	virtual void install(interpreter *) = 0;
	virtual void uninstall(interpreter *) = 0;
};

// base class for object command handlers
// and for class handlers
class object_cmd_base : public callback_base {
  public:
	// destructor not needed, but exists to shut up the compiler warnings
	//virtual ~object_cmd_base() {}
	//virtual void invoke(void *p, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], bool object_dot_method) = 0;
};

// base class for all class handlers, still abstract
class class_handler_base : public object_cmd_base {
  public:
	typedef std::map<std::string, policies> policies_map_type;

	class_handler_base();

	void register_method(std::string const &name, callback_base * ocb, Tcl_Interp * interp, bool managed);

	//policies &get_policies(std::string const &name);

	void install_methods(interpreter * tcli, Tcl_Interp * interp, const char * prefix, void * obj);
	void uninstall_methods(interpreter * tcli, Tcl_Interp * interp, const char * prefix);
	~class_handler_base() {
		for (auto & r : methods_) {
			delete r.second;
			delete[] r.first;
		}
	}
protected:
	struct str_less {
		bool operator()(const char * a, const char * b) const {
			return strcmp(a, b) < 0;
		}
	};

	typedef std::map<const char *, callback_base *, str_less> method_map_type;

	// a map of methods for the given class
	method_map_type methods_;

	//policies_map_type policies_;
};

// class handler - responsible for executing class methods
template <class C>
class class_handler : public class_handler_base {
  public:
	virtual void invoke(interpreter * tcli, void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], bool object_dot_method) {
		C * p = static_cast<C *>(pv);

		char * methodName = Tcl_GetString(objv[1]);
		if (objc == 2 && strcmp(methodName, "-delete") == 0) {
			Tcl_DeleteCommand(interp, Tcl_GetString(objv[0]));
			delete p;
			return;
		}

		method_map_type::iterator it = methods_.find(methodName);
		if (it == methods_.end()) {
			throw tcl_error("Method " + std::string(methodName) + " not found.");
		}

		it->second->invoke(tcli, pv, interp, objc, objv, false);
	}
	void install(interpreter *) { }
	void uninstall(interpreter *) { }
};
} // details

class interpreter;

template <typename T>
struct objref {
	T * val_;
	Tcl_Obj * obj_;
	T * operator->() {
		return &val_;
	}
	T const * operator->() const {
		return &val_;
	}
	T & operator*() {
		return val_;
	}
	T const & operator*() const {
		return val_;
	}
};

template <typename T>
struct optional {
	T val;
	bool valid;
	bool is_reference_;

	bool is_reference() const { return is_reference_; }
	operator bool() const { return valid; }
	T const & operator*() const {
		if (!valid) {
			throw tcl_error("retrieving value of unspecified optional attempted");
		}
		return val;
	}
	T const * operator->() const {
		if (!valid) {
			throw tcl_error("retrieving value of unspecified optional attempted");
		}
		return &val;
	}
	optional() : valid(false) { }
	optional(T) : valid(false) { }
	optional(T t, bool v, bool ref) : val(t), valid(v), is_reference_(ref) { }
	operator std::optional<T>() { return valid ? std::optional<T>(val) : std::optional<T>(); }
};

template <typename T>
struct getopt : public optional<T> {
	using optional<T>::optional;
	using optional<T>::operator std::optional<T>;
};

template <typename T>
struct getopt_d : public optional<T> {
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

template <typename ...Ts>
struct overloaded {
	std::tuple<Ts...> ts;
};

template <typename ...Ts>
overloaded<Ts...> overload(Ts... ts) {
	return overloaded<Ts...>{ { ts... } };
};

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
			Tcl_IncrRefCount(lo_);
		}
	}

	~list() {
		if (list_owner_) {
			Tcl_DecrRefCount(lo_);
		}
	}
	
	//list(list const & other) : ot_(other.ot_), interp_(other.interp_), lo_(other.lo_) { std::cerr << '#'; }
	
	typedef typename std::conditional<isany || details::is_basic_type<T>::value || std::is_same<T, object>::value, T, T *>::type return_t;

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
	return_t at(std::size_t ix) const {
		return with_obj_at(ix).first;
	}
	return_t operator[](std::size_t ix) const { return at(ix); }
	bool is_reference(std::size_t ix) const {
		Tcl_Obj * o = obj_at(ix);
		if (o->typePtr == ot_[0]) { return true; }
		else if (o->typePtr == ot_[1]) { return false; }
	}
	std::pair<return_t, Tcl_Obj *> with_obj_at(std::size_t ix) const;
	Tcl_Obj * obj_at(std::size_t ix) const;
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

#if 0
template <typename ...Ts>
struct any_impl {
	Tcl_Obj * o_;
};
template <typename T, typename ...Ts>
struct any_impl<T, Ts...> : public any_impl<Ts...> {
};
#endif

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
struct any { //: public details::any_impl<Ts...> {
	//typedef details::any_impl<Ts...> super_t;
	//any() { super_t::which_ = -1; }
	interpreter * interp_;
	Tcl_ObjType ** ot_;
	bool list_owner_ = false;
	Tcl_Obj * o_;
	
	any(interpreter * i, Tcl_Obj * o, Tcl_ObjType ** ot, bool list_owner = false) : interp_(i), ot_(ot), list_owner_(list_owner), o_(o) {
		if (list_owner) {
			Tcl_IncrRefCount(this->o_);
		}
	}
	any(any const & o) : interp_(o.interp_), ot_(o.ot_), list_owner_(o.list_owner_), o_(o.o_) {
		if (list_owner_) {
			Tcl_IncrRefCount(this->o_);
		}
	}
	any() : o_(nullptr) { }
	~any() {
		if (list_owner_) {
			Tcl_DecrRefCount(this->o_);
		}
	}

	object as_object();
	
	bool is_reference() const {
		for (int i = 0; i < sizeof...(Ts); ++i) {
			if (ot_[i] == this->o_->typePtr) {
				return true;
			} else if (ot_[i] + 1 == this->o_->typePtr) {
				return false;
			}
		}
		throw("is_reference called on invalid any<> object");
	}
	operator bool() const {
		if (! o_) return false;
		for (int i = 0; i < sizeof...(Ts); ++i) {
			if (ot_[i] == this->o_->typePtr || ot_[i] + 1 == this->o_->typePtr) return true;
		}
		return false;
	}

	template <typename ...Fs>
	bool visit(Fs... f) const;
	
	template <typename TT>
	typename std::conditional<details::is_list<TT>::value, TT, TT *>::type as() const;
};

namespace details { struct init_default_tag { }; }

template <typename ...Ts> struct init {
	bool default_;
	init() : default_(false) { }
	init(details::init_default_tag) : default_(true) { }
};

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
struct is_getopt_d {
	static const bool value = false;
};
template <typename T>
struct is_getopt_d<getopt_d<T> > {
	static const bool value = true;
};
template <typename T>
struct is_getopt_d<getopt_d<T> const &> {
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
struct optional_unpack<getopt_d<T> > {
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
template <typename ...Ts>
struct num_getopt_d {
	static const int value = 0;
};
template <typename T, typename ...Ts>
struct num_getopt_d<T, Ts...> {
	static const int value = num_getopt_d<Ts...>::value + (is_getopt_d<T>::value ? 1 : 0);
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
	static bool invoke(any<Ts...> const &) {
		return false;
	}
};

class no_argument_match { };
template <bool strip_list, typename F, typename ...Ts>
struct argument_type {
	typedef no_argument_match type;
};
template <bool strip_list, typename F, typename T, typename ...Ts>
struct argument_type<strip_list, F, T, Ts...> {
	typedef typename std::conditional<strip_list, typename list_unpack<T>::type, T>::type arg1_t;

	typedef typename std::conditional<
		std::is_invocable<F, arg1_t *>::value || std::is_invocable<F, arg1_t *, Tcl_Obj *>::value,
		T,
		typename argument_type<strip_list, F, Ts...>::type
	>::type type;
};

template <typename F, typename ...Fs>
struct visit_impl<F, Fs...> {
	template <typename ...Ts>
	static bool invoke(any<Ts...> const & a, F f, Fs... fs) {
		typedef typename argument_type<false, F, Ts...>::type arg_t;
		typedef typename argument_type<true,  F, Ts...>::type arg_list_t;

		if constexpr (! std::is_same<arg_t, no_argument_match>::value || ! std::is_same<arg_list_t, no_argument_match>::value) {
			if constexpr (!std::is_same<arg_t, arg_list_t>::value) {
				if (arg_list_t li = a.template as<arg_list_t>()) {
					if constexpr (std::is_invocable<F, decltype(li.at(0))>::value) {
						for (auto && i : li) {
							if constexpr (std::is_same<typename std::invoke_result<F, decltype(li.at(0))>::type, bool>::value) {
								if (! f(i)) break;
							} else {
								f(i);
							}
						}
					} else if constexpr (std::is_invocable<F, decltype(li.at(0)), Tcl_Obj *>::value) {
						for (std::size_t i = 0; i < li.size(); ++i) {
							auto both = li.with_obj_at(i);
							if constexpr (std::is_same<typename std::invoke_result<F, decltype(li.at(0)), Tcl_Obj *>::type, bool>::value) {
								if (! f(both.first, both.second)) break;
							} else {
								f(both.first, both.second);
							}
						}
					}
					return true;
				}
			} else {
				if (arg_t * p = a.template as<arg_t>()) {
					if constexpr (std::is_invocable<F, arg_t *>::value) {
						f(p);
					} else if constexpr (std::is_invocable<F, arg_t *, Tcl_Obj *>::value) {
						f(p, nullptr);
					}
					return true;
				}
			}
		}
		return visit_impl<Fs...>::invoke(a, fs...);
	}
};

inline std::string tcl_typename(Tcl_ObjType const * ot) {
	std::ostringstream oss;
	oss << ot;
	return std::string(ot ? ot->name : "()") + "@" + oss.str();
}
inline std::string tcl_typename(Tcl_Obj const * o) {
	return tcl_typename(o->typePtr);
}
}

template <typename ...Ts>
template <typename ...Fs>
bool any<Ts...>::visit(Fs... f) const {
	return details::visit_impl<Fs...>::invoke(*this, f...);
}

namespace details {
template <int I, typename ...Ts>
struct generate_hasarg {
	static void invoke(bool * arr, std::string *, std::string *, const char *, bool) { }
};
template <int I, typename T, typename ...Ts>
struct generate_hasarg<I, T, Ts...> {
	static void invoke(bool * arr, std::string * opts, std::string * defaults, const char * p, bool may_have_defaults) {
		if constexpr (is_getopt_d<T>::value) {
			while (*p && isspace(*p)) ++p;
			if (p) {
				const char * pend = p;
				while (*pend && !isspace(*pend) && *pend != '=') ++pend;
			}
			generate_hasarg<I, Ts...>::invoke(arr, opts, defaults, p, may_have_defaults);
		} else if constexpr (!is_getopt<T>::value) {
			generate_hasarg<I, Ts...>::invoke(arr, opts, defaults, p, may_have_defaults);
		} else {
			arr[I] = !std::is_same<typename remove_rc<T>::type, getopt<bool> >::value;
			while (*p && isspace(*p)) ++p;
			if (p) {
				const char * pend = p;
				while (*pend && !isspace(*pend) && *pend != '=') ++pend;
				opts[I] = std::string(p, pend - p);
				if (*pend == '=') {
					if (!may_have_defaults) {
						throw tcl_error("function is not allowed to have defaults");
					}
					++pend; p = pend;
					while (*pend && !isspace(*pend)) ++pend;
					defaults[I] = std::string(p, pend - p);
				}
				generate_hasarg<I + 1, Ts...>::invoke(arr, opts, defaults, pend, may_have_defaults);
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
	//typedef typename std::decay<T>::type type;
	typedef T type;
};
template <typename T, bool Last>
struct fix_variadic_return<opt<T> const &, Last> {
	typedef opt<T> type;
};
template <typename T, bool Last>
struct fix_variadic_return<getopt<T> const &, Last> {
	typedef getopt<T> type;
};
template <typename T, bool Last>
struct fix_variadic_return<getopt_d<T> const &, Last> {
	typedef getopt_d<T> type;
};

template <typename T, bool Last>
struct fix_variadic_return<list<T> const &, Last> {
	typedef list<T> type;
};
template <typename ...Ts, bool Last>
struct fix_variadic_return<any<Ts...> const &, Last> {
	typedef any<Ts...> type;
};
template <typename T, bool Last>
struct fix_variadic_return<variadic<T> const &, Last> {
	typedef variadic<T> type;
};
template <bool Last>
struct fix_variadic_return<std::string const &, Last> {
	typedef std::string type;
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
struct no_class_lambda { };

template <typename C>
struct class_lambda { };

template <typename ...Convs>
struct conversions {
};

template <int ...Ixs>
struct conversion_ix { };
template <typename ...Ts>
struct conversion_t { };

template <int I, int N, typename Ix, typename T>
struct make_conversions {
};

struct default_conversion {
};

struct conversions_end {
};

struct conversion_error {
};

template <int N, int ...Ixs, typename ...Ts>
struct make_conversions<N, N, conversion_ix<Ixs...>, conversion_t<Ts...> > {
	typedef conversions_end type;
	typedef conversions_end recurse;
};

template <int I, int N, int Ix, int ...Ixs, typename T, typename ...Ts>
struct make_conversions<I, N, conversion_ix<Ix, Ixs...>, conversion_t<T, Ts...> > {
	typedef typename std::conditional<Ix == I, T, default_conversion>::type type;
	typedef typename std::conditional
	<Ix == I,
	 make_conversions<I + 1, N, conversion_ix<Ixs...>, conversion_t<Ts...> >,
	 make_conversions<I + 1, N, conversion_ix<I, Ixs...>, conversion_t<T, Ts...> > >::type recurse;
};

template <int I, typename Conv>
struct conversion_at {
	typedef typename std::conditional
	<I == 0, typename Conv::type, typename conversion_at<I + 1, typename Conv::recurse>::type>::type type;
};

template <int I>
struct conversion_at<I, conversions_end> {
	typedef conversion_error type;
};

struct callback_empty_base { };
template <typename T, bool P>
struct callback_no_baseclass { }; // policy tag

template <typename T>
struct callback_base_type {
	typedef object_cmd_base type;
};
template <>
struct callback_base_type<no_class> {
	typedef callback_base type;
};
template <typename T>
struct callback_free_method { }; // policy tag

template <typename T, bool P>
struct callback_postproc { }; // policy tag

template <typename T, typename A>
struct callback_attributes { };

template <typename T>
struct callback_base_type<callback_no_baseclass<T, true> > {
	typedef callback_empty_base type;
};

template <typename T>
struct callback_pass_interpreter { }; // policy tag

template <bool cold_ = false>
struct attributes {
	static const bool cold = cold_;
};
template <typename T>
struct is_attribute {
	static const bool value = false;
};

template <bool cold>
struct is_attribute<attributes<cold> > {
	static const bool value = true;
};

template <typename T>
struct callback_policy_unpack {
	typedef T type;
	static const bool no_base      = false;
	static const bool free_method  = false;
	static const bool class_lambda = false;
	static const bool postproc     = true;
	static const bool pass_interp  = false;
	typedef attributes<>             attr;
};

template <typename T, bool P>
struct callback_policy_unpack<callback_no_baseclass<T, P> > : public callback_policy_unpack<T> {
	static const bool no_base = P;
};
template <typename T, bool P>
struct callback_policy_unpack<callback_postproc<T, P> > : public callback_policy_unpack<T> {
	static const bool postproc = P;
};

template <typename T, typename A>
struct callback_policy_unpack<callback_attributes<T, A> > : public callback_policy_unpack<T> {
	typedef A attr;
	//typedef typename attributes_merge<A...>::type attr;
};

template <typename T>
struct callback_policy_unpack<callback_free_method<T> > : public callback_policy_unpack<T> {
	static const bool free_method = true;
};
template <typename T>
struct callback_policy_unpack<class_lambda<T> > : public callback_policy_unpack<T> {
	static const bool class_lambda = true;
};

template <typename T>
struct callback_policy_unpack<callback_pass_interpreter<T> > : public callback_policy_unpack<T> {
	static const bool pass_interp = true;
};

template <typename Cparm, typename Conv, typename Fn, typename R, typename ...Ts>
class callback_v : public callback_base_type<Cparm>::type {
	typedef callback_v this_t;

	struct client_data {
		this_t * this_p;
		interpreter * interp;
	};
	static void cleanup_client_data(ClientData cd) {
		client_data * p = (client_data *) cd;
		delete p;
	}

	typedef callback_policy_unpack<Cparm> pol_t;
	typedef typename pol_t::type C;

	static const bool may_have_defaults = false;

	typedef Fn functor_type;
	policies policies_;
	functor_type f_;

	static const int num_opt = num_getopt<Ts...>::value + num_getopt_d<Ts...>::value;
	std::string opts_[num_opt], defaults_[num_opt];

	bool has_arg_[num_opt + 1] = { false };

	static const int arg_offset = -num_opt;

	Tcl_ObjType * argument_types_all_[generate_argtypes<0, Ts...>::length + 1];
	
	//interpreter * interpreter_;
	Conv conv_;

	std::string name_;

	static const bool cold = pol_t::attr::cold;
public :
	static const int num_arguments = (std::is_same<C, no_class>::value || pol_t::free_method ? 0 : 1) + sizeof...(Ts);

#ifdef COLD
#error fixme
#else
#define COLD __attribute__((optimize("s"))) __attribute__((cold))
#endif
	
	callback_v(interpreter * i, functor_type f, std::string const & name, policies const & pol = policies(), Conv conv = Conv()) COLD : policies_(pol), f_(f), conv_(conv), name_(name) {
		if (num_opt) {
			if (pol.options_.empty()) {
				throw tcl_error(std::string("no getopt string supplied for function \"") + name + "\" taking getopt<> arguments");
			}
			generate_hasarg<0, Ts...>::invoke(has_arg_, opts_, defaults_, pol.options_.c_str(), may_have_defaults);
		}
		generate_argtypes<0, Ts...>::invoke(i, argument_types_all_);
	}
	void install(interpreter *) __attribute__((optimize("s")));
	void uninstall(interpreter *) __attribute__((optimize("s")));
	void invoke(interpreter * tcli, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv [], bool object_dot_method) {
		checked_invoke_impl(tcli, pv, interp, argc, argv, object_dot_method);
	}
	void invoke_impl(interpreter *, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv [], bool object_dot_method) COLD;

	~callback_v() COLD { }
private :
	template <bool v> struct void_return { };

	template <int ArgI>
	std::pair<Tcl_ObjType **, Tcl_ObjType **> args() {
		return generate_argtypes<0, Ts...>::template get<ArgI>(argument_types_all_);
	}

	template <typename CC, std::size_t ...Is>
	void do_invoke(std::index_sequence<Is...> const &, interpreter *, CC * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], int getoptc, Tcl_Obj * const getoptv[], policies const & pol, void_return<false>) __attribute__((optimize(cold ? "s" : "3")));
	template <typename CC, std::size_t ...Is>
	void do_invoke(std::index_sequence<Is...> const &, interpreter *, CC * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], int getoptc, Tcl_Obj ** getoptv, policies const & pol, void_return<true>) __attribute__((optimize(cold ? "s" : "3")));

	int checked_invoke_impl(interpreter *, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv [], bool object_dot_method) __attribute__((optimize(cold ? "s" : "3")));

	static int callback_handler(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) __attribute__((optimize(cold ? "s" : "3")));

};

template <typename Cparm, typename Conv, typename Fn, typename R, typename ...Ts>
template <typename CC, std::size_t ...Is>
void callback_v<Cparm, Conv, Fn, R, Ts...>::do_invoke(std::index_sequence<Is...> const &, interpreter * tcli, CC * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], int getoptc, Tcl_Obj * const getoptv[], policies const & pol, void_return<false>) {
	if (argc && argv) { }

	if constexpr (pol_t::pass_interp) {
		if constexpr (pol_t::class_lambda) {
			details::set_result(interp, f_(tcli, p, do_cast<arg_offset, 1, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...));
		} else if constexpr (std::is_same<C, no_class>::value || pol_t::free_method) {
			details::set_result(interp, f_(tcli, do_cast<arg_offset, 0, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...));
		} else {
			details::set_result(interp, (p->*f_)(tcli, do_cast<arg_offset, 0, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...));
		}
	} else {
		if constexpr (pol_t::class_lambda) {
			details::set_result(interp, f_(p, do_cast<arg_offset, 1, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...));
		} else if constexpr (std::is_same<C, no_class>::value || pol_t::free_method) {
			details::set_result(interp, f_(do_cast<arg_offset, 0, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...));
		} else {
			details::set_result(interp, (p->*f_)(do_cast<arg_offset, 0, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...));
		}
	}
}

template <typename Cparm, typename Conv, typename Fn, typename R, typename ...Ts>
template <typename CC, std::size_t ...Is>
void callback_v<Cparm, Conv, Fn, R, Ts...>::do_invoke(std::index_sequence<Is...> const &, interpreter * tcli, CC * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], int getoptc, Tcl_Obj ** getoptv, policies const & pol, void_return<true>) {
	if (argc && argv && interp) { }

	if constexpr (pol_t::pass_interp) {
		if constexpr (pol_t::class_lambda) {
			f_(tcli, p, do_cast<arg_offset, 1, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...);
		} else if constexpr (std::is_same<C, no_class>::value || pol_t::free_method) {
			f_(tcli, do_cast<arg_offset, 0, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...);
		} else {
			(p->*f_)(tcli, do_cast<arg_offset, 0, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...);
		}
	} else {
		if constexpr (pol_t::class_lambda) {
			f_(p, do_cast<arg_offset, 1, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...);
		} else if constexpr (std::is_same<C, no_class>::value || pol_t::free_method) {
			f_(do_cast<arg_offset, 0, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...);
		} else {
			(p->*f_)(do_cast<arg_offset, 0, Ts, sizeof...(Is), Is>(tcli, args<Is>(), ppack<Ts...>(), pol.variadic_, interp, argc, argv, getoptc, getoptv, pol, conv_, p)...);
		}
	}
}


template <typename Cparm, typename Conv, typename Fn, typename Fn2, bool postproc, bool no_base = false, typename Attr = attributes<>, typename E = void>
struct callback_expand_f {
};

template <typename Cparm, typename Conv, typename Fn, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f<Cparm, Conv, Fn, R (*)(Ts...), postproc, no_base, Attr, typename std::enable_if<!std::is_class<Fn>::value>::type> {
	typedef callback_v<callback_free_method<callback_postproc<callback_no_baseclass<callback_attributes<Cparm, Attr>, no_base>, postproc> >, Conv, Fn, R, Ts...> type;
};
template <typename Cparm, typename Conv, typename Fn, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f<Cparm, Conv, Fn, R (*)(interpreter *, Ts...), postproc, no_base, Attr, typename std::enable_if<!std::is_class<Fn>::value>::type> {
	typedef callback_v<callback_free_method<callback_postproc<callback_no_baseclass<callback_attributes<callback_pass_interpreter<Cparm>, Attr>, no_base>, postproc> >, Conv, Fn, R, Ts...> type;
};

template <typename Cparm, typename Conv, typename Fn, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f<Cparm, Conv, Fn, R (Cparm::*)(Ts...), postproc, no_base, Attr, typename std::enable_if<!std::is_class<Fn>::value>::type> {
	typedef callback_v<callback_postproc<callback_no_baseclass<callback_attributes<Cparm, Attr>, no_base>, postproc>, Conv, Fn, R, Ts...> type;
};
template <typename Cparm, typename Conv, typename Fn, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f<Cparm, Conv, Fn, R (Cparm::*)(interpreter *, Ts...), postproc, no_base, Attr, typename std::enable_if<!std::is_class<Fn>::value>::type> {
	typedef callback_v<callback_postproc<callback_no_baseclass<callback_attributes<callback_pass_interpreter<Cparm>, Attr>, no_base>, postproc>, Conv, Fn, R, Ts...> type;
};

template <typename Cparm, typename Conv, typename Fn, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f<Cparm, Conv, Fn, std::function<R (Ts...)>, postproc, no_base, Attr> {
	typedef callback_v<callback_postproc<callback_no_baseclass<callback_attributes<Cparm, Attr>, no_base>, postproc>, Conv, Fn, R, Ts...> type;
};
template <typename Cparm, typename Conv, typename Fn, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f<Cparm, Conv, Fn, std::function<R (interpreter *, Ts...)>, postproc, no_base, Attr> {
	typedef callback_v<callback_postproc<callback_no_baseclass<callback_attributes<callback_pass_interpreter<Cparm>, Attr>, no_base>, postproc>, Conv, Fn, R, Ts...> type;
};

template <typename Cparm, typename T>
struct senti { };

template <typename Cparm, typename Conv, typename Fn, typename Fn2, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2 {
	typedef typename senti<Cparm, Fn2>::type type;
};

// 6 definitions without pass_interpreter
template <typename Cparm, typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<Cparm, Conv, Fn, R (LC::*)(Cparm *, Ts...), postproc, no_base, Attr> {
	typedef callback_v<class_lambda<callback_postproc<callback_no_baseclass<callback_attributes<Cparm, Attr>, no_base>, postproc> >, Conv, Fn, R, Ts...> type;
};
template <typename Cparm, typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<Cparm, Conv, Fn, R (LC::*)(Cparm *, Ts...) const, postproc, no_base, Attr> {
	typedef callback_v<class_lambda<callback_postproc<callback_no_baseclass<callback_attributes<Cparm, Attr>, no_base>, postproc> >, Conv, Fn, R, Ts...> type;
};
template <typename Cparm, typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<Cparm, Conv, Fn, R (LC::*)(Ts...), postproc, no_base, Attr> {
	typedef callback_v<callback_free_method<callback_postproc<callback_no_baseclass<callback_attributes<Cparm, Attr>, no_base>, postproc> >, Conv, Fn, R, Ts...> type;
};
template <typename Cparm, typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<Cparm, Conv, Fn, R (LC::*)(Ts...) const, postproc, no_base, Attr> {
	typedef callback_v<callback_free_method<callback_postproc<callback_no_baseclass<callback_attributes<Cparm, Attr>, no_base>, postproc> >, Conv, Fn, R, Ts...> type;
};
template <typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<no_class, Conv, Fn, R (LC::*)(Ts...), postproc, no_base, Attr> {
	typedef callback_v<callback_postproc<callback_no_baseclass<callback_attributes<no_class, Attr>, no_base>, postproc>, Conv, Fn, R, Ts...> type;
};
template <typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<no_class, Conv, Fn, R (LC::*)(Ts...) const, postproc, no_base, Attr> {
	typedef callback_v<callback_postproc<callback_no_baseclass<callback_attributes<no_class, Attr>, no_base>, postproc>, Conv, Fn, R, Ts...> type;
};

// same 6 definitions with pass_interpreter
template <typename Cparm, typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<Cparm, Conv, Fn, R (LC::*)(interpreter *, Cparm *, Ts...), postproc, no_base, Attr> {
	typedef callback_v<class_lambda<callback_postproc<callback_no_baseclass<callback_attributes<callback_pass_interpreter<Cparm>, Attr>, no_base>, postproc> >, Conv, Fn, R, Ts...> type;
};
template <typename Cparm, typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<Cparm, Conv, Fn, R (LC::*)(interpreter *, Cparm *, Ts...) const, postproc, no_base, Attr> {
	typedef callback_v<class_lambda<callback_postproc<callback_no_baseclass<callback_attributes<callback_pass_interpreter<Cparm>, Attr>, no_base>, postproc> >, Conv, Fn, R, Ts...> type;
};
template <typename Cparm, typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<Cparm, Conv, Fn, R (LC::*)(interpreter *, Ts...), postproc, no_base, Attr> {
	typedef callback_v<callback_free_method<callback_postproc<callback_no_baseclass<callback_attributes<callback_pass_interpreter<Cparm>, Attr>, no_base>, postproc> >, Conv, Fn, R, Ts...> type;
};
template <typename Cparm, typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<Cparm, Conv, Fn, R (LC::*)(interpreter *, Ts...) const, postproc, no_base, Attr> {
	typedef callback_v<callback_free_method<callback_postproc<callback_no_baseclass<callback_attributes<callback_pass_interpreter<Cparm>, Attr>, no_base>, postproc> >, Conv, Fn, R, Ts...> type;
};
template <typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<no_class, Conv, Fn, R (LC::*)(interpreter *, Ts...), postproc, no_base, Attr> {
	typedef callback_v<callback_postproc<callback_no_baseclass<callback_attributes<callback_pass_interpreter<no_class>, Attr>, no_base>, postproc>, Conv, Fn, R, Ts...> type;
};
template <typename Conv, typename Fn, typename LC, typename R, typename ...Ts, bool postproc, bool no_base, typename Attr>
struct callback_expand_f2<no_class, Conv, Fn, R (LC::*)(interpreter *, Ts...) const, postproc, no_base, Attr> {
	typedef callback_v<callback_postproc<callback_no_baseclass<callback_attributes<callback_pass_interpreter<no_class>, Attr>, no_base>, postproc>, Conv, Fn, R, Ts...> type;
};








template <typename F>
struct is_std_function {
	static const bool value = false;
};
template <typename R, typename ...Ts>
struct is_std_function<std::function<R(Ts...)> > {
	static const bool value = true;
};

template <typename Cparm, typename Conv, typename Fn, bool postproc, bool no_base, typename Attr>
struct callback_expand_f<Cparm, Conv, Fn, Fn, postproc, no_base, Attr, typename std::enable_if<std::is_class<Fn>::value && !is_std_function<Fn>::value>::type> {
	typedef typename callback_expand_f2<Cparm, Conv, Fn, decltype(&Fn::operator()), postproc, no_base, Attr>::type type;
};

template <typename Fn>
struct num_arguments {
	static const int value = -1;
};

template <typename R, typename ...Ts>
struct num_arguments<R (*)(Ts...)> {
	static const int value = sizeof...(Ts);
};

template <int Ix, typename Fn, typename Tup>
static void overload_enumerate(Fn fn, Tup const & tup) {
	fn(&std::get<Ix>(tup));
	if constexpr (Ix < std::tuple_size<Tup>::value - 1) {
		overload_enumerate<Ix + 1>(fn, tup);
	}
}

template <typename Cparm, typename Conv, typename ...Over>
struct overloaded_callback : public callback_base_type<Cparm>::type {
	typedef overloaded_callback this_t;
	typedef callback_policy_unpack<Cparm> pol_t;
	typedef typename pol_t::type Cparm_out;

	static const bool cold = pol_t::attr::cold;

	struct client_data {
		this_t * this_p;
		interpreter * interp;
	};
	static void cleanup_client_data(ClientData cd) {
		client_data * p = (client_data *) cd;
		delete p;
	}
	
	//interpreter * interpreter_;
	std::string name_;

	std::tuple<typename callback_expand_f<Cparm_out, Conv, Over, Over, true, true, attributes<>>::type...> callbacks_;

	overloaded_callback(interpreter * i, overloaded<Over...> f, std::string const & name, policies const & pol, Conv conv)
		: overloaded_callback(std::make_index_sequence<sizeof...(Over)>(), i, f, name, pol, conv) {
	}
	
	template <std::size_t ...Is>
	overloaded_callback(std::index_sequence<Is...> const &, interpreter * i, overloaded<Over...> f, std::string const & name, policies const & pol, Conv conv)
		: name_(name), callbacks_(typename callback_expand_f<Cparm_out, Conv, Over, Over, true, true, attributes<>>::type(i, std::get<Is>(f.ts), name, pol, conv)...) { }
	
	template <int ti>
	void do_invoke(interpreter * tcli, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], bool object_dot_method) __attribute__((optimize(cold ? "s" : "3")));

	static int callback_handler(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) __attribute__((optimize(cold ? "s" : "3")));
	int checked_invoke_impl(interpreter * tcli, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], bool object_dot_method) __attribute__((optimize(cold ? "s" : "3")));
	void invoke_impl(interpreter * tcli, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], bool object_dot_method) __attribute__((optimize(cold ? "s" : "3")));
	void invoke(interpreter * tcli, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], bool object_dot_method) {
		checked_invoke_impl(tcli, pv, interp, argc, argv, object_dot_method);
	}
	void install(interpreter * tcli) COLD;
	void uninstall(interpreter * tcli) COLD;
};

template <typename Cparm, typename Conv, typename ...Over>
template <int ti>
void overloaded_callback<Cparm, Conv, Over...>::do_invoke(interpreter * tcli, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], bool object_dot_method) {
	if (std::tuple_element<ti, decltype(callbacks_)>::type::num_arguments + 1 == argc) {
		std::get<ti>(callbacks_).invoke_impl(tcli, pv, interp, argc, argv, object_dot_method);
		return;
	}
	if constexpr (ti + 1 < std::tuple_size<decltype(callbacks_)>::value) {
		do_invoke<ti + 1>(tcli, pv, interp, argc, argv, object_dot_method);
	} else {
		std::cerr << "wrong number of arguments, overload accepts any of:";
		overload_enumerate<0>([&] (auto * o) {
								  std::cerr << " " << std::remove_pointer<decltype(o)>::type::num_arguments;
							  }, callbacks_);
		std::cerr << " arguments\n";
		
	}
}

template <typename T>
struct std_function {
	typedef void type;
};
template <typename Ret, typename C, typename ...Args>
struct std_function<Ret (C::*)(Args...) const> {
	typedef std::function<Ret(Args...)> type;
};
}

template <typename Fn>
struct lambda_wrapper { Fn fn; lambda_wrapper(Fn f) : fn(f) { } };

namespace details {
template <typename T>
struct is_lambda_wrapper {
	static const bool value = false;
};
template <typename T>
struct is_lambda_wrapper<lambda_wrapper<T> > {
	static const bool value = true;
};

inline policies default_policies;

template <typename E, typename ...Extra>
inline policies const & extra_args_policies(E const & e, Extra... extra) {
	if constexpr (std::is_same<typename std::decay<E>::type, policies>::value) {
		return e;
	}
	return default_policies;
}

inline policies const & extra_args_policies() {
	return default_policies;
}

inline auto extra_args_conversions() {
	return std::tuple<>();
}

template <typename ...Extra>
struct has_policies {
	static const bool value = false;
};

template <typename E, typename ...Extra>
struct has_policies<E, Extra...> {
	static const bool value = std::is_same<E, policies>::value ? true : has_policies<Extra...>::value;
};

template <typename E, typename ...Extra>
inline auto extra_args_conversions(E e, Extra... extra) {
	if constexpr (std::is_same<typename std::decay<E>::type, policies>::value) {
		return extra_args_conversions(extra...);
	} else if constexpr (is_lambda_wrapper<E>::value) {
		return extra_args_conversions(extra...);
	} else if constexpr (is_attribute<E>::value) {
		return extra_args_conversions(extra...);
	} else {
		return std::tuple(e, extra...);
	}
}

template <typename Fn>
inline auto apply_extra_args_wrappers(Fn fn) {
	return fn;
}

template <typename Fn, typename E, typename ...Extra>
inline auto apply_extra_args_wrappers(Fn fn, E e, Extra... extra) {
	if constexpr (std::is_same<typename std::decay<E>::type, policies>::value) {
		return apply_extra_args_wrappers(fn, extra...);
	} else if constexpr (is_lambda_wrapper<E>::value) {
		return e.fn(fn);
	} else {
		return fn;
	}
}
template <typename ...Extra>
struct attributes_merge {
	typedef attributes<> type;
};

template <typename E, typename ...Extra>
struct attributes_merge<E, Extra...> {
	typedef typename std::conditional<is_attribute<E>::value, E, typename attributes_merge<Extra...>::type>::type type;
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
	void post_process_policies(Tcl_Interp *interp, policies &pol, Tcl_Obj *CONST objv[], bool isMethod);
	bool want_abort_ = false;
	void want_abort() {
		want_abort_ = true;
	}

	static std::function<void(interpreter *)> capture_callback;
private:
	interpreter(const interpreter &i);
	interpreter();

	static bool static_initialized_;

	static int capture_interpreter_init(Tcl_Interp * interp) {
		if (capture_callback) {
			interpreter * tcli = new interpreter(interp, false);
			capture_callback(tcli);
		}
		return TCL_OK;
	}
	
	static void static_initialize() {
		if (! static_initialized_) {
			Tcl_StaticPackage(nullptr, "Capture_interpreter", capture_interpreter_init, capture_interpreter_init);
			static_initialized_ = true;
		}
	}

	
	
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

	typedef std::map<std::string, std::shared_ptr<details::callback_base> > all_callbacks_t;
	all_callbacks_t all_callbacks_;
	typedef std::map<std::string, std::shared_ptr<details::class_handler_base> > all_classes_t;
	all_classes_t all_classes_;

	struct all_classes_by_objtype_value {
		std::shared_ptr<details::class_handler_base> class_handler;
		bool is_inline;
	};
	
	typedef std::map<Tcl_ObjType const *, all_classes_by_objtype_value> all_classes_by_objtype_t;
	all_classes_by_objtype_t all_classes_by_objtype_;
	
	static int dot_call_handler(ClientData cd, Tcl_Interp * interp, int objc, Tcl_Obj * CONST * objv);
	
	std::shared_ptr<details::callback_base> find_callback(std::string const & name) {
		auto it = all_callbacks_.find(name);
		if (it != all_callbacks_.end()) {
			return std::shared_ptr<details::callback_base>(it->second);
		}
		return std::shared_ptr<details::callback_base>(nullptr);
	}
	std::shared_ptr<details::class_handler_base> find_class(std::string const & name) {
		auto it = all_classes_.find(name);
		if (it != all_classes_.end()) {
			return std::shared_ptr<details::class_handler_base>(it->second);
		}
		return std::shared_ptr<details::class_handler_base>(nullptr);
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

	template <typename C, typename Conv, typename Fn, bool postproc = true, typename Attr = details::attributes<>, typename R=void, typename ...Ts>
	using callback_t = typename details::callback_expand_f<C, Conv, Fn, Fn, postproc, false, Attr>::type;
	
	template <int i>
	static inline std::integral_constant<int, i> arg;

	struct {
	} res;

	static constexpr details::attributes<true> cold = { };
	
	template <typename ...Extra>
	static auto eac(Extra... e) {
		return details::extra_args_conversions(e...);
	}
	template <typename ...Extra>
	static auto eap(Extra... e) {
		return details::extra_args_policies(e...);
	}
	template <typename Fn, typename ...Extra>
	static auto eaw(Fn fn, Extra ... e) {
		return details::apply_extra_args_wrappers(fn, e...);
	}
	
	template <class C, typename ...WExtra>
	class class_definer {
		std::tuple<WExtra...> with_extra;
	public:
		class_definer(details::class_handler<C> * ch, interpreter * inter, bool managed, WExtra... extra) : ch_(ch), interp_(inter), managed_(managed), with_extra(extra...) {}
		
		template <typename Fn, typename ...Extra>
		class_definer & def(std::string const & name, Fn fn, Extra... extra) {
			//return def2<Fn>(name, fn, std::make_index_sequence<sizeof...(WExtra)>(), extra...);
			return def2<Fn>(name, fn, std::make_index_sequence<sizeof...(WExtra)>(), extra...);
		}

		template <typename Fn, std::size_t ...Is, typename ...Extra>
		class_definer & def2(std::string const & name, Fn f, std::index_sequence<Is...>, Extra... extra) COLD;
		
		template <typename Fn, std::size_t ...Is, typename ...Over, typename ...Extra>
		class_definer & def2(std::string const & name, overloaded<Over...> const & over, std::index_sequence<Is...> seq, Extra... extra) COLD;

		template <typename CC, typename T>
		class_definer & defvar(std::string const & name, T CC::*varp) COLD;
	private:
		interpreter * interp_;
		details::class_handler<C> * ch_;
		bool managed_;
	};

	template <typename C, typename ...WExtra>
	struct function_definer {
		std::tuple<WExtra...> with_extra;
		
		function_definer(C * this_p, interpreter * interp, WExtra... extra) : this_p_(this_p), interp_(interp), with_extra(extra...) { }

		template <typename Fn, typename ...Extra>
		function_definer & def(std::string const & name, Fn fn, Extra... extra) {
			return def2(name, fn, std::make_index_sequence<sizeof...(WExtra)>(), extra...);
		}

		template <typename Fn, typename ...Extra, std::size_t ...Is>
		function_definer & def2(std::string const & name, Fn fn, std::index_sequence<Is...>, Extra... extra) COLD;
		
		template <typename T>
		function_definer & defvar(std::string const & name, T C::*t) COLD;
		//template <typename T>
		//function_definer & defvar(std::string const & name, T C::&t) {
		//	return defvar(name, &t);
		//}
		
		C * this_p_;
		interpreter * interp_;
	};

	const Tcl_ObjType   * list_type_               = nullptr;
	const Tcl_ObjType   * cmdname_type_            = nullptr;
	const Tcl_Namespace * object_namespace_        = nullptr;
	//const Tcl_Namespace * object_commandnamespace_ = nullptr;

	bool is_list(Tcl_Obj * o) { return o->typePtr == list_type_; }
	
	static constexpr const char * object_namespace_name = "o";
	static constexpr const char * object_namespace_prefix = "o::";
	static const int object_namespace_prefix_len = 3;

#if 0
	static constexpr const char * object_command_namespace_name = "O";
	static constexpr const char * object_command_namespace_prefix = "O::";
	static const int object_command_namespace_prefix_len = 3;
#endif

#if 0
	object make_object(long i) { return object(this, i); }
	object make_object(char const *s) { return object(this, s); }
	object make_object(std::string const &s) { return object(this, s); }
	object make_object(Tcl_Obj *o) { return object(this, o); }
	object make_object(bool b) { return object(this, b); }
	object make_object(char const *buf, size_t size) { return object(this, buf, size); }
	object make_object(double b) { return object(this, b); }
	object make_object(int i) { return object(this, i); }
	object make_object() { return object(this); }
#endif
	
	void find_standard_types();
	interpreter(Tcl_Interp *, bool owner = false);
	~interpreter();

	interpreter * master_ = nullptr;
	void set_master(interpreter * master) {
		master_ = master;
	}

	void make_safe();

	void custom_construct(const char * classname, const char * objname, void * p);

	Tcl_Interp * get() const  { return interp_; }
	Tcl_Interp * get_interp() { return interp_; }

	// free function definitions

	template <typename Fn, typename ...Extra>
	void def(std::string const & name, Fn fn, Extra... extra) {
		auto pol = eap(extra...);

		def2<details::has_policies<Extra...>::value>(name, fn, extra...);
	}
	template <bool Postproc, typename Fn, typename ...Extra>
	void def2(std::string const & name, Fn fn, Extra... extra) {
		if (master_) {
			add_function(name, master_->find_callback(name));
		} else {
			typedef decltype(details::extra_args_conversions(extra...)) conv_t;
			add_function(name, std::shared_ptr<details::callback_base>(new callback_t<details::no_class, conv_t, Fn, Postproc>(this, fn, name, eap(extra...), eac(extra...))));
		}
	}

	template <typename C, typename R, typename ...Ts, typename ...Extra>
	void def(std::string const & name, R (C::*f)(Ts...), C * this_p, Extra... extra) {
		def(name, [this_p, f](Ts... args) { return (this_p->*f)(args...); }, extra...);
	}

	
	template <typename ...Over, typename ...Extra>
	void def(std::string const & name, overloaded<Over...> over, Extra... extra) {
		if (master_) {
			add_function(name, master_->find_callback(name));
		} else {
			typedef decltype(details::extra_args_conversions(extra...)) conv_t;
			add_function(name, std::shared_ptr<details::callback_base>(new details::overloaded_callback<details::no_class, conv_t, Over...>(this, over, name, eap(extra...), eac(extra...))));
		}
	}
	
	template <typename T>
	void defvar(std::string const & name, T & v);

	template <typename C, typename ...Extra>
	function_definer<C, Extra...> with(C * this_p, Extra... extra) {
		return function_definer<C, Extra...>(this_p, this, extra...);
	}
	template <typename ...Extra>
	function_definer<details::no_class, Extra...> with(Extra... extra) {
		return function_definer<details::no_class, Extra...>(nullptr, this, extra...);
	}

	template <typename C, typename ...Ts> struct call_constructor {
		static C * doit(Ts... ts) {
			return new C(ts...);
		}
	};

	template <class C, typename ...Ts, typename ...Extra>
	class_definer<C, Extra...> class_(std::string const &name, init<Ts...> const & init_arg = init<Ts...>(details::init_default_tag()), Extra... extra) COLD;
	template <class C, typename ...Extra>
	class_definer<C, Extra...> class_(std::string const &name, details::no_init_type const &, Extra... extra) COLD;
	
	template <typename C, typename ...Ts> struct call_managed_constructor {
		interpreter * interp_;
		call_managed_constructor(interpreter * i) : interp_(i) { }

		object operator()(Ts... ts);
	};

	template <class C, bool ValueType, typename ...Ts, typename ...Extra>
	class_definer<C> managed_class_(std::string const &name, init<Ts...> const & = init<Ts...>(), Extra... extra) {
		if (master_) {
			std::shared_ptr<details::class_handler_base> chp = master_->find_class(name);
			assert(chp);
			add_class(name, chp, sizeof(C) <= sizeof(((Tcl_Obj *) 0)->internalRep));
			add_managed_constructor(name, chp.get(), master_->find_callback(name));
			details::class_handler<C> * ch = static_cast<details::class_handler<C> *>(chp.get());
			return class_definer<C, Extra...>(ch, this, true, extra...);
		} else {
			typedef details::callback_v<details::no_class, std::tuple<>, call_managed_constructor<C, Ts...>, object, Ts...> callback_type;

			if (get_class_handler(name)) {
				throw tcl_error("overloaded constructors not implemented");
			}
			
			details::class_handler<C> * ch = new details::class_handler<C>();
			
			this->type<type_ops<C, true>, ValueType >(name, (C *) 0);
			
			add_class(name, std::shared_ptr<details::class_handler_base>(ch), sizeof(C) <= sizeof(((Tcl_Obj *) 0)->internalRep));
			
			add_managed_constructor(name, ch, std::shared_ptr<details::callback_base>(new callback_type(this, call_managed_constructor<C, Ts...>(this), name)));
			
			return class_definer<C, Extra...>(ch, this, true, extra...);
		}
	}

	template <class C, bool ValueType, typename ...Extra>
	class_definer<C, Extra...> managed_class_(std::string const &name, details::no_init_type const &, Extra... extra) {
		if (master_) {
			std::shared_ptr<details::class_handler_base> chp = master_->find_class(name);
			if (all_classes_.count(name) == 0) {
				add_class(name, chp, sizeof(C) <= sizeof(((Tcl_Obj *) 0)->internalRep));
			}
			details::class_handler<C> * ch = static_cast<details::class_handler<C> *>(chp.get());
			return class_definer<C, Extra...>(ch, this, true, extra...);
		} else {
			details::class_handler<C> * ch = static_cast<details::class_handler<C> *>(get_class_handler(name));
			if (!ch) {
				ch = new details::class_handler<C>();
				this->type<type_ops<C, true>, ValueType >(name, (C *) 0);
				add_class(name, std::shared_ptr<details::class_handler_base>(ch), sizeof(C) <= sizeof(((Tcl_Obj *) 0)->internalRep));
			}
			return class_definer<C, Extra...>(ch, this, true, extra...);
		}
	}

	static std::map<std::string, Tcl_ObjType *> obj_type_by_tclname_;
	static std::map<std::type_index, Tcl_ObjType *> obj_type_by_cppname_;

	template <typename T>
	struct deleter {
		virtual bool free(Tcl_Obj * o) {
			delete (T *) o->internalRep.twoPtrValue.ptr1;
			return false;
		}
		virtual deleter * dup_deleter() { return this; }
		virtual void * dup(void * obj) {
			return obj;
		}
		virtual ~deleter() { }
	};

	template <typename T>
	struct dyn_deleter : public deleter<T> {
		bool free(Tcl_Obj * o) override {
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
		}
		shared_ptr_deleter(std::shared_ptr<T> & p) : p_(p) {
		}
		bool free(Tcl_Obj * o) override {
			return true;
		}
		virtual void * dup(void * obj) override { return obj; }
		deleter<T> * dup_deleter() override {
			return new shared_ptr_deleter(p_);
		}
		~shared_ptr_deleter() {
		}
	};
	
	template <typename T, bool Managed = false>
	struct type_ops {
		static void dup(Tcl_Obj * src, Tcl_Obj * dst) {
			if (src->internalRep.twoPtrValue.ptr2) {
				deleter<T> * d = (deleter<T> *) src->internalRep.twoPtrValue.ptr2;
				dst->internalRep.twoPtrValue.ptr1 = d->dup(src->internalRep.twoPtrValue.ptr1);
				dst->internalRep.twoPtrValue.ptr2 = (( deleter<T> *)src->internalRep.twoPtrValue.ptr2)->dup_deleter();
			} else {
				dst->internalRep.twoPtrValue.ptr1 = src->internalRep.twoPtrValue.ptr1;
				dst->internalRep.twoPtrValue.ptr2 = nullptr;
			}
			dst->typePtr = src->typePtr;
		}
		static void dup_v(Tcl_Obj * src, Tcl_Obj * dst) {
			if constexpr (sizeof(T) <= sizeof(dst->internalRep)) {
				memcpy(&dst->internalRep, &src->internalRep, sizeof(T));
			} else {
				if constexpr (std::is_copy_constructible<T>::value) {
					dst->internalRep.twoPtrValue.ptr1 = new T(* ((T *) src->internalRep.twoPtrValue.ptr1));
				} else {
					throw tcl_error("attempting to make value for class with no copy constructor");
				}
			}
		}
		static void free(Tcl_Obj * o) {
			if (o->internalRep.twoPtrValue.ptr2) {
				deleter<T> * d = (deleter<T> *) o->internalRep.twoPtrValue.ptr2;
				if (d->free(o)) {
					delete d;
				}
			}
		}
		static void free_v(Tcl_Obj * o) {
			if constexpr (sizeof(T) <= sizeof(o->internalRep)) {
				((T *) &o->internalRep)->~T();
			} else {
				delete ((T *) o->internalRep.twoPtrValue.ptr1);
			}
		}
		static void str(Tcl_Obj * o) {
			std::ostringstream oss;
			oss << (Managed ? object_namespace_name : object_namespace_name) << "::" << o->internalRep.twoPtrValue.ptr1;
			str_impl(o, oss.str());
		}
		static void str_v(Tcl_Obj * o) {
			if constexpr (sizeof(T) <= sizeof(o->internalRep)) {
				std::ostringstream oss;
				oss << &o->internalRep;
				str_impl(o, oss.str());
			} else {
				std::ostringstream oss;
				oss << o->internalRep.twoPtrValue.ptr1;
				str_impl(o, oss.str());
			}
		}
		static void str_impl(Tcl_Obj * o, std::string const & name) {
			o->bytes = Tcl_Alloc(name.size() + 1);
			strncpy(o->bytes, name.c_str(), name.size());
			o->bytes[name.size()] = 0;
			o->length = name.size();
		}
		static int set(Tcl_Interp * interp, Tcl_Obj * o) {
			//std::cerr << "setfromany " << interp << " " << o << "\n";
			return TCL_OK;
		}
		static int set_v(Tcl_Interp *, Tcl_Obj *) {
			return TCL_OK;
		}
	};

	template <typename TY>
	Tcl_ObjType * get_objtype() {
		auto it = obj_type_by_cppname_.find(std::type_index(typeid(TY)));
		if (it != obj_type_by_cppname_.end()) {
			return it->second;
		}
		return nullptr;
	}
	Tcl_ObjType * get_objtype(const char * name) {
		auto & o = obj_type_by_tclname_;
		auto it = o.find(name);
		return it == o.end() ? nullptr : it->second;
	}
	template <typename T>
	bool is_type(Tcl_Obj * o) {
		return o->typePtr && get_objtype<T>() == o->typePtr;
	}
	template <typename T>
	T * try_type(Tcl_Obj * o) {
		if (is_type<T>(o)) {
			return (T *) o->internalRep.twoPtrValue.ptr1;
		}
		return nullptr;
	}

	struct extra_typeinfo {
		bool value_type;
	};
	
	template <typename OPS, bool ValueType, typename TY>
	static void type(std::string const & name, TY * p) {
		auto it = obj_type_by_cppname_.find(std::type_index(typeid(TY)));
		if (it != obj_type_by_cppname_.end()) {
			//std::cerr << "attempt to register type " << name << " twice\n";
			return;
		}
		auto it2 = obj_type_by_tclname_.find(name);
		if (it2 != obj_type_by_tclname_.end()) {
			//std::cerr << "attempt to register type " << name << " twice\n";
			return;
		}
		Tcl_ObjType * ot = new Tcl_ObjType[4];

		if constexpr (ValueType) {
			char * cp_vt = new char[name.size() + 3];
			strcpy(cp_vt, name.c_str());
			cp_vt[name.size()] = '_';
			cp_vt[name.size() + 1] = 'v';
			cp_vt[name.size() + 2] = 0;
			ot[2].name = cp_vt;
			ot[2].freeIntRepProc   = &OPS::free_v;
			ot[2].dupIntRepProc    = &OPS::dup_v;
			ot[2].updateStringProc = &OPS::str_v;
			ot[2].setFromAnyProc   = &OPS::set_v;
			Tcl_RegisterObjType(ot + 2);
		} else {
			memset(ot + 2, 0, sizeof(*ot));
			ot[2].name = "unused";
		}
		//std::type_index * tip = (std::type_index *) (cp + 256);
		//*tip = std::type_index(typeid(TY));
		memset(ot, 0, sizeof(*ot));
		memset(ot + 3, 0, sizeof(*ot));

		char * cp = new char[name.size() + 1];
		strcpy(cp, name.c_str());
		cp[name.size()] = 0;
		ot[1].name = cp;
		ot[1].freeIntRepProc   = &OPS::free;
		ot[1].dupIntRepProc    = &OPS::dup;
		ot[1].updateStringProc = &OPS::str;
		ot[1].setFromAnyProc   = &OPS::set;

		Tcl_RegisterObjType(ot + 1);
		obj_type_by_tclname_.insert(std::pair<std::string, Tcl_ObjType *>(name, ot + 1));
		obj_type_by_cppname_.insert(std::pair<std::type_index, Tcl_ObjType *>(std::type_index(typeid(TY)), ot + 1));
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

	template <typename T, typename std::enable_if<std::is_pointer<T>::value && std::is_same<typename std::remove_const<typename std::remove_pointer<T>::type>::type, char>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp * i, Tcl_Obj * o, bool byref) {
		return Tcl_GetStringFromObj(o, nullptr);
	}

	template <typename T, typename std::enable_if<std::is_same<T, Tcl_Obj *>::value>::type = true>
	T tcl_cast(Tcl_Interp * i, Tcl_Obj * o, bool byref) {
		return o;
	}
	
	template <typename T, typename std::enable_if<std::is_pointer<T>::value && !std::is_same<typename std::remove_const<typename std::remove_pointer<T>::type>::type, char>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp * i, Tcl_Obj * o, bool byref) {
		auto it = obj_type_by_cppname_.find(std::type_index(typeid(typename std::remove_pointer<T>::type)));
		if (it != obj_type_by_cppname_.end()) {
			return this->template tcl_cast<T>(i, o, byref, it->second);
		}
		throw tcl_error("function argument type is not registered");
	}

	template <typename T, typename std::enable_if<std::is_reference<T>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp * interp, Tcl_Obj * o, bool byref) {
		return *tcl_cast<typename std::remove_reference<T>::type *>(interp, o, byref);
	}
	
	template <typename T, typename std::enable_if<std::is_pointer<T>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp * i, Tcl_Obj * o, bool byref, Tcl_ObjType * ot) {
		if (std::is_pointer<T>::value && o->typePtr) {
			Tcl_ObjType * otp = ot;
			if (otp == o->typePtr) {
				return (T) o->internalRep.twoPtrValue.ptr1;//otherValuePtr;
			} else if (otp + 1 == o->typePtr) {
				if (sizeof(typename std::remove_pointer<T>::type) <= sizeof(o->internalRep)) {
					return (T) &o->internalRep;
				} else {
					return (T) o->internalRep.twoPtrValue.ptr1;
				}
			} else if (is_list(o)) {
				int len;
				if (Tcl_ListObjLength(i, o, &len) == TCL_OK) {
					if (len == 1) {
						Tcl_Obj *o2;
						if (Tcl_ListObjIndex(i, o, 0, &o2) == TCL_OK) {
							if (otp == o2->typePtr) {
								return (T) o2->internalRep.twoPtrValue.ptr1;
							} else {
								throw tcl_error("function argument has wrong type. expect " + details::tcl_typename(otp) + ", got " + details::tcl_typename(o2->typePtr));
							}
						} else {
							throw tcl_error("internal error. cannot access valid list index");
						}
					} else {
						throw tcl_error("Expected single object, got list");
					}
				} else {
					throw tcl_error("Unknown tcl error");
				}
			} else {
				throw tcl_error("function argument has wrong type. expect " + details::tcl_typename(otp) + ", got " + details::tcl_typename(o->typePtr));
			}
		} else {
			throw tcl_error("function argument is not a tcl object type");
		}
		return nullptr;
	}
	template <typename T, typename std::enable_if<!std::is_pointer<T>::value && !std::is_reference<T>::value && std::is_same<T, typename details::list_unpack<T>::type>::value && !details::is_any<T>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp * i, Tcl_Obj * o, bool byref) {
		return details::tcl_cast<T>::from(i, o, byref);
	}
	template <typename T, typename std::enable_if<!std::is_pointer<T>::value && std::is_same<T, typename details::list_unpack<T>::type>::value && !details::is_any<T>::value, bool>::type = true>
	T tcl_cast(Tcl_Interp * i, Tcl_Obj * o, bool byref, Tcl_ObjType *) {
		return details::tcl_cast<T>::from(i, o, byref);
	}

	// free script evaluation
	details::result eval(std::string const &script);
	details::result eval(std::istream &s);

	details::result eval(object const &o);

	void result_reset() {
		Tcl_ResetResult(get_interp());
	}

	static std::string objinfo(object const & o);
	
	// the InputIterator should give object& or Tcl_Obj* when dereferenced
	template <class InputIterator> details::result eval(InputIterator first, InputIterator last);

	template <typename OT, typename DEL>
	object makeobj(OT * n, bool owning = false, DEL delfn = []{});
	template <typename OT>
	object makeobj(OT * n, bool owning = false);
	template <typename OT, typename DEL>
	void makeobj_inplace(OT * p, object & obj, bool owning = false, DEL delfn = []{});

	template <typename OT>
	void makevalue_inplace(OT * o, object & obj);
	template <typename OT>
	object makevalue(OT * o);

	
	template <typename T>
	void setVar(std::string const & name, T const & value);
	void setVar(std::string const & name, object const & value);

	template <typename OT>
	void setVar(std::string const & name, object & ptr);

	void unsetVar(std::string const & name);
	
	// Get a variable from TCL interpreter with Tcl_GetVar
	details::result getVar(std::string const &scalarTclVariable);
	details::result getVar(std::string const &arrayTclVariable, std::string const &arrayIndex);

	details::result upVar(std::string const & frame, std::string const & srcname, std::string const & destname);
	
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
	void clear_definitions();

  private:
	void operator=(const interpreter &);

	void add_function(std::string const &name, std::shared_ptr<details::callback_base> cb);

	void add_class(std::string const &name, std::shared_ptr<details::class_handler_base> chb, bool is_inline);
	details::class_handler_base * get_class_handler(std::string const & name);
	
	void add_constructor(std::string const &name, details::class_handler_base * chb, std::shared_ptr<details::callback_base> cb);
	void add_managed_constructor(std::string const &name, details::class_handler_base * chb, std::shared_ptr<details::callback_base> cb);

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
Tcl_Obj * list<T>::obj_at(std::size_t ix) const {
	Tcl_Obj *o;
	if (Tcl_ListObjIndex(interp_->get(), lo_, ix, &o) == TCL_OK) {
		return o;
	}
}

template <typename T>
std::pair<typename list<T>::return_t, Tcl_Obj *> list<T>::with_obj_at(std::size_t ix) const {
	typedef std::pair<return_t, Tcl_Obj *> ret_t;
	Tcl_Obj *o;
	if (Tcl_ListObjIndex(interp_->get(), lo_, ix, &o) == TCL_OK) {
		if constexpr (isany) {
			return ret_t(return_t(interp_, o, ot_), o);
		} else if constexpr (details::is_basic_type<T>::value) {
			return ret_t(interp_->template tcl_cast<T>(o), o);
		} else if constexpr (std::is_same<T, object>::value) {
			return ret_t(object(o), o);
		} else {
			if (o->typePtr == ot_[0]) {
				return ret_t((return_t) o->internalRep.twoPtrValue.ptr1, o);
			} else {
				throw tcl_error("Unexpected type in list. Got " + details::tcl_typename(o) + " need " + details::tcl_typename(ot_[0]));
			}
		}
	}
	return ret_t(return_t(), nullptr);
}

template <typename ...Ts>
template <typename TT>
typename std::conditional<details::is_list<TT>::value, TT, TT *>::type any<Ts...>::as() const {
	const int toff = sizeof...(Ts) - 1 - details::type_at<TT, Ts...>::value;

	if constexpr (details::is_list<TT>::value) {
		if (this->o_ && interp_->is_list(this->o_)) {
			Tcl_Obj * oo;
			if (Tcl_ListObjIndex(interp_->get_interp(), this->o_, 0, &oo) == TCL_OK) {
				if (oo->typePtr == this->ot_[toff]) {
					return TT(interp_, this->o_, this->ot_ + toff);
				}
			}
			return TT();
		} else {
			return TT();
		}
	} else {
		if (this->o_ && this->ot_[toff] == this->o_->typePtr) {
			return (TT *) this->o_->internalRep.twoPtrValue.ptr1;
		} else {
			return nullptr;
		}
	}
}

namespace details {
template <typename T> struct tcl_cast<T *> {
	static T *from(Tcl_Interp *, Tcl_Obj *obj, bool byReference) {
		std::string s(Tcl_GetString(obj) + interpreter::object_namespace_prefix_len);
		if (s.size() == 0) {
			throw tcl_error("Expected pointer value, got empty string.");
		}

		if (s.compare(0, interpreter::object_namespace_prefix_len, interpreter::object_namespace_prefix) != 0) {
			throw tcl_error("Expected pointer value.");
		}

		std::istringstream ss(s);
		void *p;
		ss >> p;

		return static_cast<T *>(p);
	}
};

template <int I, typename TArg, typename ...Ts>
struct generate_argtypes<I, TArg, Ts...> {
	template <typename WRAPT>
	struct wrap { };

	typedef typename remove_rc<TArg>::type T;
	
	template <typename TT>
	static Tcl_ObjType * lookup_type(interpreter * interp, int pos, wrap<TT> * np) {
		auto * ot = interp->get_objtype<typename std::remove_pointer<TT>::type>();
		return ot;
	}
	template <std::size_t ...Is>
	static void invoke_helper(std::index_sequence<Is...> const &, interpreter * interp, Tcl_ObjType ** all) {
		((all[Is] = lookup_type(interp, Is, (wrap<typename subtype<Is, T>::type> *) 0)), ...);
		generate_argtypes<I + 1, Ts...>::invoke(interp, all + sizeof...(Is));
	}
	static void invoke(interpreter * interp, Tcl_ObjType ** all) {
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

template <typename ...Ts>
struct back {
	typedef void type;
};

template <typename T, typename ...Ts>
struct back<T, Ts...> {
	using type = typename std::conditional<sizeof...(Ts) == 0, T, typename back<Ts...>::type>::type;
};

template <typename Cparm, typename Convs, typename Fn, typename R, typename ...Ts>
void callback_v<Cparm, Convs, Fn, R, Ts...>::install(interpreter * tcli) {
	client_data * cdata = new client_data { this, tcli };

	Tcl_CreateObjCommand(tcli->get_interp(), name_.c_str(), callback_handler, (ClientData) cdata, cleanup_client_data);
}
template <typename Cparm, typename Convs, typename Fn, typename R, typename ...Ts>
void callback_v<Cparm, Convs, Fn, R, Ts...>::uninstall(interpreter * tcli) {
	Tcl_DeleteCommand(tcli->get_interp(), name_.c_str());
}

template <typename Cparm, typename Convs, typename Fn, typename R, typename ...Ts>
void callback_v<Cparm, Convs, Fn, R, Ts...>::invoke_impl(interpreter * tcli, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv [], bool object_dot_method) {
	static const int num_gopt   = num_getopt<Ts...>::value;
	static const int num_gopt_d = num_getopt_d<Ts...>::value;
	static const int num_opt    = num_optional<Ts...>::value;
	Tcl_Obj * getopt_argv[num_gopt + 1] = { nullptr };
	bool getopt_allocated[num_gopt + 1] = { false };
	Tcl_Obj boolean_dummy_object;
	std::size_t ai = std::is_same<C, no_class>::value || pol_t::free_method || object_dot_method ? 1 : 2;
	static const bool has_variadic_ = has_variadic<Ts...>::value;
	bool any_getopt_allocated = false;
	
	if (tcli->trace()) {
		for (int i = 0; i < argc; ++i) {
			std::cerr << " ";
			if (tcli->is_list(argv[i])) {
				std::cerr << '{' << Tcl_GetString(argv[i]) << '}';
			} else {
				std::cerr << Tcl_GetString(argv[i]);
			}
		}
		std::cerr << "\n";
	}
	++tcli->trace_count_;
	
	if (num_gopt && argc == ai + 1 && strcmp(Tcl_GetString(argv[ai]), "-help") == 0 && std::find(opts_, opts_ + num_gopt, "help") == opts_ + num_gopt) {
		if (num_gopt) {
			std::cerr << Tcl_GetString(argv[0]);
			for (int oi = 0; oi < num_gopt; ++oi) {
				tcli->terr() << " -" << opts_[oi] << (has_arg_[oi] ? " <arg>" : "");
			}
			
			if (policies_.variadic_) {
				tcli->terr() << " objects...";
			}
			tcli->terr() << "\n";
		}
		return;
	}
	
	if constexpr (num_gopt) {
		for (; ai < argc; ++ai) {
			char * s = Tcl_GetString(argv[ai]);
			if (s[0] == '-') {
				if (s[1] == '-' && s[2] == 0) {
					++ai;
					break;
				}
				bool found = false;
				for (int oi = 0; oi < num_gopt; ++oi) {
					if (opts_[oi].compare(s + 1) == 0) {
						found = true;
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
		if (may_have_defaults) {
			for (int oi = 0; oi < num_gopt; ++oi) {
				if (! getopt_argv[oi]) {
					if (!defaults_[oi].empty()) {
						Tcl_IncrRefCount(getopt_argv[oi] = Tcl_NewStringObj(defaults_[oi].c_str(), defaults_[oi].size()));
						getopt_allocated[oi] = true;
						any_getopt_allocated |= true;
					}
				}
			}
		}
	}
	check_params_no(argc - ai, sizeof...(Ts) - (has_variadic_ || policies_.variadic_ ? 1 : 0) - num_gopt - num_opt - num_gopt_d, has_variadic_ || policies_.variadic_ ? -1 : sizeof...(Ts) - num_gopt - num_gopt_d, policies_.usage_);
	
	auto dealloc = [&](void *) {
		if (may_have_defaults && any_getopt_allocated) {
			for (int oi = 0; oi < num_gopt; ++oi) {
				if (getopt_allocated[oi]) {
					Tcl_DecrRefCount(getopt_argv[oi]);
				}
			}
		}
	};
	std::unique_ptr<void *, decltype(dealloc)> dealloc_raii(0, dealloc);
	
	do_invoke(std::make_index_sequence<sizeof... (Ts)>(), tcli, (C *) pv, interp, argc - ai, argv + ai, num_gopt + num_gopt_d, getopt_argv, policies_, void_return<std::is_same<R, void>::value>());
	if (pol_t::postproc) {
		tcli->post_process_policies(interp, policies_, argv + ai, false);
	}
}
template <typename Cparm, typename Convs, typename Fn, typename R, typename ...Ts>
int callback_v<Cparm, Convs, Fn, R, Ts...>::callback_handler(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	client_data * cdata = (client_data *) cd;
	//this_t * this_p = (this_t *) cd;
	
	return cdata->this_p->checked_invoke_impl(cdata->interp, nullptr, interp, objc, objv, false);
}

template <typename Cparm, typename Conv, typename ...Over>
void overloaded_callback<Cparm, Conv, Over...>::invoke_impl(interpreter * tcli, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], bool object_dot_method) {
	do_invoke<0>(tcli, pv, interp, argc, argv, object_dot_method);
}

template <typename Cparm, typename Conv, typename ...Over>
int overloaded_callback<Cparm, Conv, Over...>::checked_invoke_impl(interpreter * tcli, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], bool object_dot_method) {
	try {
		invoke_impl(tcli, pv, interp, argc, argv, object_dot_method);
	} catch (tcl_usage_message_printed & e) {
	} catch (std::exception & e) {
		for (int i = 0; i < argc; ++i) {
			if (i) { std::cerr << " "; }
			if (tcli->is_list(argv[i])) {
				std::cerr << '{' << Tcl_GetString(argv[i]) << '}';
			} else {
				std::cerr << Tcl_GetString(argv[i]);
			}
		}
		std::cerr << ": " << e.what() << "\n";
		return TCL_ERROR;
	} catch (...) {
		std::cerr << "Unknown error.\n";
		return TCL_ERROR;
	}
	return TCL_OK;
}
template <typename Cparm, typename Conv, typename ...Over>
int overloaded_callback<Cparm, Conv, Over...>::callback_handler(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	//this_t * this_p = (this_t *) cd;
	client_data * cdata = (client_data *) cd;
	
	return cdata->this_p->checked_invoke_impl(cdata->interp, nullptr, interp, objc, objv, false);
}

template <typename Cparm, typename Conv, typename ...Over>
void overloaded_callback<Cparm, Conv, Over...>::install(interpreter * tcli) {
	client_data * cdata = new client_data {this, tcli};

	Tcl_CreateObjCommand(tcli->get_interp(), name_.c_str(), callback_handler, (ClientData) cdata, cleanup_client_data);
}
template <typename Cparm, typename Conv, typename ...Over>
void overloaded_callback<Cparm, Conv, Over...>::uninstall(interpreter * tcli) {
	Tcl_DeleteCommand(tcli->get_interp(), name_.c_str());
}
}
#undef COLD

#include "cpptcl/cpptcl_object.h"

template <typename Iter, typename std::enable_if<!std::is_same<typename std::iterator_traits<Iter>::value_type, void>::value, bool>::type = true>
void set_result(Tcl_Interp * interp, std::pair<Iter, Iter> it) {
	object o;
	for (; it.first != it.second; ++it.first) {
		o.append(object(*it.first));
	}
	Tcl_SetObjResult(interp, o.get_object_refcounted());
}

template <typename C, typename ...Ts>
object interpreter::call_managed_constructor<C, Ts...>::operator()(Ts... ts) {
	object ret = interp_->makeobj(new C(ts...), true, [this](C * p) {
									  std::ostringstream oss;
									  oss << object_namespace_name << "::" << p << ".";
									  Tcl_DeleteCommand(interp_->get_interp(), oss.str().c_str());
									  delete p;
								  });
	return ret;
}

inline std::string interpreter::objinfo(object const & o) {
	Tcl_Obj * oo = o.get_object();
	std::ostringstream oss;
	oss << "obj=" << oo << " refcount=" << oo->refCount << " typePtr=" << oo->typePtr << " typename=" << (oo->typePtr ? oo->typePtr->name : "(notype)") << " ptr1=" << oo->internalRep.twoPtrValue.ptr1 << " ptr2=" << oo->internalRep.twoPtrValue.ptr2 << "\n";
	return oss.str();
}

template <typename ...Ts>
inline object any<Ts...>::as_object() {
	return object(this->o_);
}

namespace details {
template <typename Cparm, typename Convs, typename Fn, typename R, typename ...Ts>
int callback_v<Cparm, Convs, Fn, R, Ts...>::checked_invoke_impl(interpreter * tcli, void * pv, Tcl_Interp * interp, int argc, Tcl_Obj * const argv [], bool object_dot_method) {
	try {
		this->invoke_impl(tcli, pv, interp, argc, argv, object_dot_method);
	} catch (tcl_usage_message_printed & e) {
	} catch (std::exception & e) {
		for (int i = 0; i < argc; ++i) {
			if (i) { std::cerr << " "; }
			if (tcli->is_list(argv[i])) {
				std::cerr << '{' << Tcl_GetString(argv[i]) << '}';
			} else {
				std::cerr << Tcl_GetString(argv[i]);
			}
		}
		std::cerr << ": " << e.what() << "\n";
		//Tcl_SetObjResult(interp, Tcl_NewStringObj(const_cast<char *>(e.what()), -1));
		return TCL_ERROR;
	} catch (...) {
		std::cerr << "Unknown error.\n";
		return TCL_ERROR;
	}
	if (tcli->want_abort_) {
		Tcl_SetObjResult(interp, object().get_object());
		return TCL_ERROR;
	}
	return TCL_OK;
}

template <int i, int sz, int arg, int nargs, typename Conv>
struct conversion_offset {
	static const int value =
		(std::is_same<typename std::tuple_element<i, Conv>::type, std::integral_constant<int, arg> >::value ||
		 std::is_same<typename std::tuple_element<i, Conv>::type, std::integral_constant<int, -(nargs - arg)> >::value)
		? i + 1 : conversion_offset<i + 2, sz, arg, nargs, Conv>::value;
};

template <int sz, int arg, int nargs, typename Conv>
struct conversion_offset<sz, sz, arg, nargs, Conv> {
	static const int value = -1;
};
template <int arg, int nargs, typename Conv>
struct conversion_offset<0, 0, arg, nargs, Conv> {
	static const int value = -1;
};

template <int ArgOffset, int ConvOffset, typename TTArg, std::size_t In, std::size_t Ii, typename ...Ts, typename Conv, typename C>
typename details::fix_variadic_return<TTArg, Ii + 1 == In>::type do_cast(interpreter * tcli, std::pair<Tcl_ObjType **, Tcl_ObjType **> objecttypes, ppack<Ts...> pack, bool variadic, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[], int getoptc, Tcl_Obj * CONST getoptv[], policies const & pol, Conv const & conv, C * this_p) {
	//typedef typename remove_rc<TTArg>::type TT;
	typedef typename details::fix_variadic_return<TTArg, Ii + 1 == In>::type TT;
	
	static const bool is_variadic_arg = is_variadic<TT>::value;
	static const bool is_optional_arg = is_optional<TT>::value;
	static const bool is_getopt_arg   = is_getopt<TT>::value;

	static const bool is_old_variadic_arg = Ii + 1 == In && std::is_same<typename back<Ts...>::type, object const &>::value;

	typedef typename list_unpack<TT>::type list_unpack_t;
	static const bool is_list_arg  = !std::is_same<TT, list_unpack_t>::value;
	static const bool cpptcl_byref = false;

	static const bool is_any_arg = is_any<TT>::value;
	static const bool is_any_list_arg = is_any_arg && all_lists<TT>::value;

	typedef typename optional_unpack<TT>::type opt_unpack_t;
	
	static const int conversion = conversion_offset<0, std::tuple_size<Conv>::value, Ii + ConvOffset, In + ConvOffset, Conv>::value;

	if constexpr (is_getopt_d<TT>::value) {
		return TT(opt_unpack_t(), false, false);
	}
	
	if constexpr (conversion >= 0) {
		using conv_t = typename std::tuple_element<conversion, Conv>::type;
		if constexpr (std::is_invocable<conv_t, Tcl_Obj *>::value) {
			return std::get<conversion>(conv)(objv[Ii]);
		} else if constexpr (std::is_invocable<conv_t, C *, int, Tcl_Obj * const *, int>::value) {
			return std::get<conversion>(conv)(this_p, objc, objv, ArgOffset + Ii);
		} else if constexpr (std::is_invocable<conv_t, int, Tcl_Obj * const *, int>::value) {
			return std::get<conversion>(conv)(objc, objv, ArgOffset + Ii);
		} else if constexpr (std::is_invocable<conv_t, TTArg>::value) {
			//return std::get<conversion>(conv)(do_cast<0, ConvOffset, opt_unpack_t, In, Ii>(tcli, objecttypes, pack, variadic, interp, getoptc, getoptv, getoptc, getoptv, pol, std::tuple<>(), this_p));
		} else {
			static_assert(Conv::no_such_member);
			std::cerr << "_Z" << typeid(this_p).name() << "\n";
			throw tcl_error("conversion function has incompatible signature. this ought to be a compile time error");
		}
	}

	if constexpr (is_old_variadic_arg) {
		if (pol.variadic_) {
			return details::get_var_params(tcli->get(), objc, objv, Ii + ArgOffset, pol);
		}
	}

	if constexpr (is_optional_arg || is_getopt_arg) {
		if constexpr (is_getopt_arg) {
			static const int getopti = getopt_index<0, Ii, 0, Ts...>::value;

			if (getoptv[getopti]) {
				if constexpr (std::is_same<TT, getopt<bool> >::value) {
					return TT(true, true, false);
				} else {
					return TT(do_cast<0, ConvOffset, opt_unpack_t, In, Ii>(tcli, objecttypes, pack, variadic, interp, getoptc, getoptv, getoptc, getoptv, pol, conv, this_p), true, false);
				}
			} else {
				return TT(opt_unpack_t(), false, false);
			}				
		} else {
			if (objc > Ii + ArgOffset) {
				return TT(do_cast<ArgOffset, ConvOffset, opt_unpack_t, In, Ii>(tcli, objecttypes, pack, variadic, interp, objc, objv, getoptc, getoptv, pol, conv, this_p), true, false);
			} else {
				return { typename std::decay<opt_unpack_t>::type(), false, false };
			}
		}
	} else if constexpr (is_list_arg) {
		if constexpr (is_basic_type<list_unpack_t>::value || std::is_same<list_unpack_t, object>::value) {
			if (tcli->is_list(objv[Ii + ArgOffset])) {
				return TT(tcli, objv[Ii + ArgOffset], nullptr);
			} else {
				Tcl_Obj * lo = Tcl_NewListObj(1, objv + Ii + ArgOffset);
				return TT(tcli, lo, nullptr, true);
			}
		} else {
			if (objv[Ii + ArgOffset]->typePtr) {
				Tcl_ObjType * ot = tcli->get_objtype<list_unpack_t>();
				if (tcli->is_list(objv[Ii + ArgOffset])) {
					return TT(tcli, objv[Ii + ArgOffset], objecttypes.first);
				} else {
					Tcl_Obj * lo = Tcl_NewListObj(1, objv + Ii + ArgOffset);
					return TT(tcli, lo, objecttypes.first, true);
				}
			} else {
				return TT(nullptr, nullptr, nullptr);
			}
		}
	} else if constexpr (is_any_arg) {
		if (objv[Ii + ArgOffset]->typePtr) {
			if (tcli->is_list(objv[Ii + ArgOffset])) {//->typePtr == list_type_) {
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
		} else {
			if constexpr (is_any_list_arg) {
				return TT(nullptr, nullptr, nullptr);
			} else {
				throw tcl_error("any<> argument does not map to an object");
			}
		}
	} else {
		if constexpr (is_variadic_arg) {
			return TT(tcli, objc, objv);
		} else {
			if constexpr (std::is_pointer<opt_unpack_t>::value && std::is_class<typename std::remove_pointer<opt_unpack_t>::type>::value) {
				return tcli->template tcl_cast<typename std::remove_cv<typename std::remove_reference<opt_unpack_t>::type>::type>(interp, objv[Ii + ArgOffset], false, *objecttypes.first);
			} else {
				//return tcli->template tcl_cast<typename std::remove_cv<typename std::remove_reference<opt_unpack_t>::type>::type>(interp, objv[Ii + ArgOffset], false);
				return tcli->template tcl_cast<opt_unpack_t>(interp, objv[Ii + ArgOffset], false);
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

template <typename C, typename ...Extra>
template <typename T>
interpreter::function_definer<C, Extra...> & interpreter::function_definer<C, Extra...>::defvar(std::string const & name, T C::*t) {
	return this->def(name, [t, this] (opt<T> const & arg) -> T {
						 if (arg) {
							 this_p_->*t = arg;
						 }
						 return this_p_->*t;
					 });
}

template <typename C, typename ...Extra>
template <typename CC, typename T>
interpreter::class_definer<C, Extra...> & interpreter::class_definer<C, Extra...>::defvar(std::string const & name, T CC::*varp) {
	return this->def(name, [varp] (CC * this_p, opt<T> const & val) -> T {
						 if (val) this_p->*varp = *val;
						 return this_p->*varp;
					 });
}

template <typename C, typename ...WExtra>
template <typename Fn, typename ...Extra, std::size_t ...Is>
interpreter::function_definer<C, WExtra...> & interpreter::function_definer<C, WExtra...>::def2(std::string const & name, Fn fn, std::index_sequence<Is...>, Extra... extra) {
	if constexpr (std::is_same<C, details::no_class>::value) {
		interp_->def(name, fn, std::get<Is>(with_extra)..., extra...);
	} else {
		interp_->def(name, fn, this_p_, std::get<Is>(with_extra)..., extra...);
	}
	return *this;
}

template <typename C, typename ...WExtra>
template <typename Fn, std::size_t ...Is, typename ...Extra>
interpreter::class_definer<C, WExtra...> & interpreter::class_definer<C, WExtra...>::def2(std::string const & name, Fn f, std::index_sequence<Is...>, Extra... extra) {
	if (interp_->master_) {
		return *this;
	}
	const bool epol = std::tuple_size<decltype(eac(extra...))>::value != sizeof...(extra);
	auto pol = epol ? eap(extra...) : eap(std::get<Is>(with_extra)...);
	
	auto convtu = std::tuple_cat(eac(extra...), eac(std::get<Is>(with_extra)...));
	auto wrapped = eaw(f, std::get<Is>(with_extra)...);
	
	typedef typename details::attributes_merge<Extra..., WExtra...>::type attrs;
	
	ch_->register_method(name, new callback_t<C, decltype(convtu), decltype(wrapped), details::has_policies<WExtra...>::value || details::has_policies<Extra...>::value, attrs>
						 (interp_, wrapped, name, pol, convtu),
						 interp_->get_interp(), managed_);
	return *this;
}

template <typename C, typename ...WExtra>
template <typename Fn, std::size_t ...Is, typename ...Over, typename ...Extra>
interpreter::class_definer<C, WExtra...> & interpreter::class_definer<C, WExtra...>::def2(std::string const & name, overloaded<Over...> const & over, std::index_sequence<Is...> seq, Extra... extra) {
	if (interp_->master_) {
		return *this;
	}
	
	const bool epol = std::tuple_size<decltype(eac(extra...))>::value != sizeof...(extra);
	auto convtu = std::tuple_cat(eac(extra...), eac(std::get<Is>(with_extra)...));
	
	typedef typename details::attributes_merge<Extra..., WExtra...>::type attrs;

	ch_->register_method(name, new details::overloaded_callback<details::callback_attributes<C, attrs>, decltype(convtu), Over...>
						 (interp_, over, name, epol ? eap(extra...) : eap(std::get<Is>(with_extra)...), convtu),
						 interp_->get_interp(), managed_);
	return *this;
}

template <class C, typename ...Ts, typename ...Extra>
interpreter::class_definer<C, Extra...> interpreter::class_(std::string const &name, init<Ts...> const & init_arg, Extra... extra) {
	auto * ch = static_cast<details::class_handler<C> *>(get_class_handler(name));
	if (master_) {
		assert(ch);
		if (all_callbacks_.count(name) == 0) {
			add_constructor(name, ch, master_->find_callback(name));
		}
	} else {
		typedef callback_t<details::no_class, std::tuple<>, C * (*)(Ts...), true> callback_type;
		
		if (ch && ! init_arg.default_) {
			throw tcl_error("overloaded constructors not implemented");
		}
		if (!ch) {
			ch = new details::class_handler<C>();
			add_class(name, std::shared_ptr<details::class_handler_base>(ch), sizeof(C) <= sizeof(((Tcl_Obj *) 0)->internalRep));
			add_constructor(name, ch, std::shared_ptr<details::callback_base>(new callback_type(this, &call_constructor<C, Ts...>::doit, name, extra...)));
		}
	}
	return class_definer<C, Extra...>(ch, this, false, extra...);
}

template <class C, typename ...Extra>
interpreter::class_definer<C, Extra...> interpreter::class_(std::string const &name, details::no_init_type const &, Extra... extra) {
	details::class_handler<C> * ch = static_cast<details::class_handler<C> *>(get_class_handler(name));
	if (master_) {
	} else {
		if (! ch) {
			ch = new details::class_handler<C>();
			add_class(name, ch, sizeof(C) <= sizeof(((Tcl_Obj *) 0)->internalRep));
		}
	}
	return class_definer<C, Extra...>(ch, this, false, extra...);
}

inline void interpreter::setVar(std::string const & name, object const & value) {
	object name_obj(name);

	Tcl_ObjSetVar2(interp_, name_obj.get_object(), nullptr, value.get_object(), 0);
}

template <typename T>
void interpreter::setVar(std::string const & name, T const & value) {
	object val_obj(value);
	object name_obj(name);

	Tcl_ObjSetVar2(interp_, name_obj.get_object(), nullptr, val_obj.get_object(), 0);
}

template <typename OT>
void interpreter::makevalue_inplace(OT * p, object & obj) {
	Tcl_Obj * o = obj.get_object();

	if (Tcl_IsShared(o)) {
		throw tcl_error("cannot modify shared object in-place");
	}
	
	if (sizeof(OT) <= sizeof(o->internalRep)) {
		void * dp = (void *) &o->internalRep;
		new (dp) OT(*p);
	} else {
		o->internalRep.twoPtrValue.ptr1 = new OT(*p);
	}
	
	auto it = this->obj_type_by_cppname_.find(std::type_index(typeid(OT)));
	bool ok = it != this->obj_type_by_cppname_.end();
	if (ok) {
		o->typePtr = it->second + 1;
	} else {
		throw tcl_error("type lookup failed");
	}
	Tcl_InvalidateStringRep(o);
}

template <typename OT, typename DEL>
void interpreter::makeobj_inplace(OT * n, object & obj, bool owning, DEL delfn) {
	Tcl_Obj * o = obj.get_object();

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
object interpreter::makevalue(OT * v) {
	object obj = object().duplicate();
	makevalue_inplace(v, obj);
	return obj;
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
		if (want_abort_) {
			want_abort_ = false;
			throw tcl_aborted();
		} else {
			throw tcl_error(interp_);
		}
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
