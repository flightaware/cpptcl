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
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <type_traits>
#include <tuple>

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

void post_process_policies(Tcl_Interp *interp, policies &pol, Tcl_Obj *CONST objv[], bool isMethod);

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

template <> struct tcl_cast<std::string> { static std::string from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };

template <> struct tcl_cast<char const *> { static char const *from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };

template <> struct tcl_cast<object> { static object from(Tcl_Interp *, Tcl_Obj *, bool byReference = false); };



	
// helper for checking for required number of parameters
// (throws tcl_error when not met)
void check_params_no(int objc, int required, const std::string &message);

// helper for gathering optional params in variadic functions
object get_var_params(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int from, policies const &pol);

// the callback_base is used to store callback handlers in a polynorphic map
class callback_base {
  public:
	virtual ~callback_base() {}

	virtual void invoke(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) = 0;
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

template <std::size_t ...Is> struct my_index_sequence { };
template <std::size_t N, std::size_t ...Is> struct my_make_index_sequence : public my_make_index_sequence<N - 1, N - 1, Is...> { };
template <std::size_t ...Is> struct my_make_index_sequence<0, Is...> : public my_index_sequence<Is...> { };
template <typename T, bool Last>
struct fix_variadic_return {
	typedef T type;
};
template <>
struct fix_variadic_return<object const &, true> {
	typedef object type;
};
template <typename TT, bool dummy>
struct return_dummy_value {
	static TT doit(object const & o) {
		return TT();
	}
};
template <typename TT> struct return_dummy_value<TT, false> {
	static TT doit(object const & o) {
		return o;
	}
};
template <int ArgOffset, typename TT, std::size_t In, std::size_t Ii>
typename details::fix_variadic_return<TT, Ii + 1 == In>::type do_cast(bool variadic, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[], policies const & pol) {
static const bool is_variadic_arg = std::is_same<TT, object const &>::value && In == Ii + 1;
	if (is_variadic_arg && variadic) {
		return details::return_dummy_value<TT, !is_variadic_arg>::doit(details::get_var_params(interp, objc, objv, In + ArgOffset - 1, pol));
	} else {
		return details::tcl_cast<TT>::from(interp, objv[Ii + ArgOffset], false);
	}
}

template <typename Fn, class C, typename R, typename ...Ts> class method_v : public object_cmd_base {
	//typedef R (C::*mem_type)(Ts...);
	//typedef R (C::*cmem_type)(Ts...) const;
	typedef Fn fn_type;
	
	policies policies_;

public:
	method_v(fn_type f, policies const & pol = policies()) : policies_(pol), f_(f) { }
	//method_v(mem_type f, policies const & pol = policies()) : policies_(pol), f_(f), cmem_(false) { }
	//method_v(cmem_type f, policies const & pol = policies()) : policies_(pol), cf_(f), cmem_(true) { }

	template <bool v> struct void_return { };
	
  	template <typename Fnn, std::size_t ...Is>
	void do_invoke(Fnn fn, my_index_sequence<Is...> const &, C * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], policies const & pol, void_return<true>) {
		(p->*fn)(do_cast<2, Ts, sizeof...(Is), Is>(pol.variadic_, interp, argc, argv, pol)...);
	}
	template <typename Fnn, std::size_t ...Is>
	void do_invoke(Fnn fn, my_index_sequence<Is...> const &, C * p, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], policies const & pol, void_return<false>) {
		details::set_result(interp, (p->*fn)(do_cast<2, Ts, sizeof...(Is), Is>(pol.variadic_, interp, argc, argv, pol)...));
	}

	virtual void invoke(void *pv, Tcl_Interp *interp, int argc, Tcl_Obj *CONST argv[], policies const &pol__) {
		check_params_no(argc, sizeof...(Ts), policies_.usage_);
		C *p = static_cast<C *>(pv);
		do_invoke(f_, my_make_index_sequence<sizeof... (Ts)>(), p, interp, argc, argv, policies_, void_return<std::is_same<R, void>::value>());
#if 0
			if (cmem_) {
			do_invoke(cf_, my_make_index_sequence<sizeof... (Ts)>(), p, interp, argc, argv, policies_, void_return<std::is_same<R, void>::value>());
		} else {
			do_invoke(f_, my_make_index_sequence<sizeof... (Ts)>(), p, interp, argc, argv, policies_, void_return<std::is_same<R, void>::value>());
		}
#endif
		post_process_policies(interp, policies_, argv, true);
	}
  private:
	fn_type f_;
	//cmem_type cf_;
	//bool cmem_;
};

} // namespace details

// init type for defining class constructors
//template <typename T1 = void, typename T2 = void, typename T3 = void, typename T4 = void, typename T5 = void, typename T6 = void, typename T7 = void, typename T8 = void, typename T9 = void> class init {};

template <typename ...Ts> struct init { };
	
// no_init type and object - to define classes without constructors
namespace details {
struct no_init_type {};

template <typename R, typename ...Ts> class callback_v : public details::callback_base {
	typedef R (*functor_type)(Ts...);
	functor_type f_;

	policies policies_;
public :
	callback_v(functor_type f, policies const & pol) : policies_(pol), f_(f) { }

	template <bool v> struct void_return { };

	template <std::size_t ...Is>
	void do_invoke(my_index_sequence<Is...> const &, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], policies const & pol, void_return<false>) {
		details::set_result(interp, f_(do_cast<1, Ts, sizeof...(Is), Is>(pol.variadic_, interp, argc, argv, pol)...));
	}
	template <std::size_t ...Is>
	void do_invoke(my_index_sequence<Is...> const &, Tcl_Interp * interp, int argc, Tcl_Obj * const argv[], policies const & pol, void_return<true>) {
		f_(do_cast<1, Ts, sizeof...(Is), Is>(pol.variadic_, interp, argc, argv, pol)...);
	}

	void invoke(Tcl_Interp * interp, int argc, Tcl_Obj * const argv []) {
		check_params_no(argc - 1, sizeof...(Ts) - (policies_.variadic_ ? 1 : 0), policies_.usage_);
		do_invoke(my_make_index_sequence<sizeof... (Ts)>(), interp, argc, argv, policies_, void_return<std::is_same<R, void>::value>());
		post_process_policies(interp, policies_, argv, false);
	}
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
		}

		void doallocate() {
		}
		void cdoallocate() {
		}


		std::streamsize xsputn(const char_type * s, std::streamsize n) {
			//std::cerr << "xsputn " << n << "\n";
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
				//	cdoallocate ();
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
			
			// Nothing to do if the buffer hasn't underflowed.
			
			if (egptr() > gptr()) {
				return *gptr ();
			}
			
			// Make sure we have a reserve area
			
			if (!base()) {
				doallocate ();
			}
			
			// Flush any pending output
			
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


	template <class C> class class_definer {
	public:
		class_definer(std::shared_ptr<details::class_handler<C>> ch) : ch_(ch) {}
		
		template <typename Fn>
		class_definer & def(std::string const & name, Fn fn, policies const & p = policies()) {
			return def2<Fn>(name, fn, p);
		}

		template <typename Fn, typename R, typename ...Ts>
		class_definer & def2(std::string const & name, R (C::*f)(Ts...), policies const & p = policies()) {
			ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method_v<Fn, C, R, Ts...>(f, p)), p);
			return *this;
		}
		template <typename Fn, typename R, typename ...Ts>
		class_definer & def2(std::string const & name, R (C::*f)(Ts...) const, policies const & p = policies()) {
			ch_->register_method(name, std::shared_ptr<details::object_cmd_base>(new details::method_v<Fn, C, R, Ts...>(f, p)), p);
			return *this;
		}
	private:
		std::shared_ptr<details::class_handler<C>> ch_;
	};

	interpreter(Tcl_Interp *, bool owner = false);
	~interpreter();

	void make_safe();

	Tcl_Interp *get() const { return interp_; }

	// free function definitions

	template <typename R, typename ...Ts>
	void def(std::string const & name, R(*f)(Ts...), policies const & p = policies()) {
		add_function(name, new details::callback_v<R, Ts...>(f, p));
	}

	// class definitions

	template <typename C, typename ...Ts> struct call_constructor {
		static C * doit(Ts... ts) {
			return new C(ts...);
		}
	};
	
	template <class C, typename ...Ts> class_definer<C> class_(std::string const &name, init<Ts...> const & = init<Ts...>(), policies const &p = policies()) {
		typedef details::callback_v<C *, Ts...> callback_type;
		
		std::shared_ptr<details::class_handler<C>> ch(new details::class_handler<C>());

		add_class(name, ch);

		add_constructor(name, ch, new callback_type(&call_constructor<C, Ts...>::doit, p), p);

		return class_definer<C>(ch);
	}
	
	template <class C> class_definer<C> class_(std::string const &name, details::no_init_type const &) {
		std::shared_ptr<details::class_handler<C>> ch(new details::class_handler<C>());

		add_class(name, ch);

		return class_definer<C>(ch);
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

	void add_function(std::string const &name, details::callback_base * cb);

	void add_class(std::string const &name, std::shared_ptr<details::class_handler_base> chb);

	void add_constructor(std::string const &name, std::shared_ptr<details::class_handler_base> chb, details::callback_base * cb, policies const &p = policies());

	Tcl_Interp *interp_;
	bool owner_;
	std::iostream tin_, tout_, terr_;
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

inline std::ostream & operator<<(std::ostream &os, const object& obj)
{
    return os << obj.get<std::string>();
}


#if 0
	extern "C" void
	TclConsoleStreambufSetup () {
		cin = new TclChannelStreambuf (Tcl_GetStdChannel (TCL_STDIN));
		cout = new TclChannelStreambuf (Tcl_GetStdChannel (TCL_STDOUT));
		cerr = new TclChannelStreambuf (Tcl_GetStdChannel (TCL_STDERR));
		cerr << "C++ standard input and output now on the Tcl console" << endl;
	}
#endif
	
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
