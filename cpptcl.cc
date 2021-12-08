//
// Copyright (C) 2004-2006, Maciej Sobczak
// Copyright (C) 2017-2019, FlightAware LLC
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

#include <iterator>
#include <map>
#include <memory>
#include <sstream>

#include <boost/core/demangle.hpp>

#include <execinfo.h>

#include "cpptcl/cpptcl.h"

using namespace Tcl;
using namespace Tcl::details;
using namespace std;

typedef std::pair<shared_ptr<callback_base>, policies> callback_handler_client_data_t;
struct constructor_handler_client_data_t {
	callback_base * cb;
	class_handler_base * chb;
	interpreter * interp;
};

struct method_handler_client_data {
	void * obj;
	class_handler_base * chb;
	bool unroll;
	Tcl_Interp * interp;
	interpreter * tcli;
};

struct managed_method_handler_client_data {
	void * obj;
	callback_base * cb;
	interpreter * interp;
};

result::result(Tcl_Interp *interp) : interp_(interp) {}

result::operator bool()   const { return tcl_cast<bool  >::from(interp_, Tcl_GetObjResult(interp_)); }
result::operator double() const { return tcl_cast<double>::from(interp_, Tcl_GetObjResult(interp_)); }
result::operator int()    const { return tcl_cast<int   >::from(interp_, Tcl_GetObjResult(interp_)); }
result::operator long()   const { return tcl_cast<long  >::from(interp_, Tcl_GetObjResult(interp_)); }
result::operator string() const {
	Tcl_Obj *obj = Tcl_GetObjResult(interp_);
	return Tcl_GetString(obj);
}
result::operator object() const { object ret(Tcl_GetObjResult(interp_)); return ret; } //ret.set_interp(interp_); return ret; }

void result::reset() {
	Tcl_ResetResult(interp_);
}

void details::set_result(Tcl_Interp *interp, bool b         ) { Tcl_SetObjResult(interp, Tcl_NewBooleanObj(b)); }
void details::set_result(Tcl_Interp *interp, int i          ) { Tcl_SetObjResult(interp, Tcl_NewIntObj(i)); }
void details::set_result(Tcl_Interp *interp, long i         ) { Tcl_SetObjResult(interp, Tcl_NewLongObj(i)); }
void details::set_result(Tcl_Interp *interp, double d       ) { Tcl_SetObjResult(interp, Tcl_NewDoubleObj(d)); }
void details::set_result(Tcl_Interp *interp, string const &s) { Tcl_SetObjResult(interp, Tcl_NewStringObj(s.data(), static_cast<int>(s.size()))); }
void details::set_result(Tcl_Interp *interp, void *p) {
	ostringstream ss;
	ss << interpreter::object_namespace_name << "::" << p;
	string s(ss.str());
	Tcl_Obj * to = Tcl_NewStringObj(s.data(), static_cast<int>(s.size()));
	to->internalRep.otherValuePtr = nullptr;
	Tcl_SetObjResult(interp, to);
}
void details::set_result(Tcl_Interp *interp, named_pointer_result p) {
	Tcl_Obj * to;
	if (p.name.empty()) {
		ostringstream ss;
		ss << interpreter::object_namespace_name << "::" << p.p;
		string s(ss.str());
		to = Tcl_NewStringObj(s.data(), static_cast<int>(s.size()));
	} else {
		to = Tcl_NewStringObj(p.name.data(), p.name.size());
	}
	to->internalRep.otherValuePtr = p.p;
	Tcl_SetObjResult(interp, to);
}


void details::set_result(Tcl_Interp *interp, object const &o) { Tcl_SetObjResult(interp, o.get_object()); }

void details::check_params_no(int objc, int required, int maximum, const std::string &message) {
	if (objc < required) {
		std::ostringstream oss;
		oss << "too few arguments: " << objc << " given, " << required << " required";
		throw tcl_error(oss.str());
	}
	if (maximum != -1 && objc > maximum) {
		std::ostringstream oss;
		oss << "too many arguments: " << objc << " given, " << maximum << " allowed";
		throw tcl_error(oss.str());
	}
}

object details::get_var_params(Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int from, policies const &pol) {
	object o;

	if (pol.variadic_) {
		check_params_no(objc, from, -1, pol.usage_);
		o.assign(objv + from, objv + objc);
	} else {
		check_params_no(objc, from + 1, from + 1, pol.usage_);
		o.assign(objv[from]);
	}

	//o.set_interp(interp);
	return o;
}

namespace // anonymous
{

// map of polymorphic callbacks
typedef map<string, shared_ptr<callback_base>> callback_interp_map;
typedef map<Tcl_Interp *, callback_interp_map> callback_map;

callback_map callbacks;
callback_map constructors;

// map of call policies
//typedef map<string, policies> policies_interp_map;
//typedef map<Tcl_Interp *, policies_interp_map> policies_map;

//policies_map call_policies;

	//typedef std::map<std::string, callback_base *> all_definitions2_t;
	//typedef std::map<Tcl_Interp *, all_definitions2_t> all_definitions_t;
	//all_definitions_t all_definitions;

// map of object handlers
typedef map<string, class_handler_base *> class_handlers_map;
	//typedef map<Tcl_Interp *, class_interp_map> class_handlers_map;

class_handlers_map class_handlers;

#if 0
	// helper for finding call policies - returns true when found
bool find_policies(Tcl_Interp *interp, string const &cmdName, policies_interp_map::iterator &piti) {
	policies_map::iterator pit = call_policies.find(interp);

	if (pit == call_policies.end()) {
		return false;
	}

	piti = pit->second.find(cmdName);
	return piti != pit->second.end();
}
#endif
	
extern "C" int object_handler(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]);

// actual functions handling various callbacks

// generic callback handler
#if 0
extern "C" int callback_handler(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	//	callback_handler_client_data_t * cdp = (callback_handler_client_data_t *) cd;

	callback_base * cb = (callback_base *) cd;
	
	try {
		cb->invoke(nullptr, interp, objc, objv, false);
		//post_process_policies(interp, cdp->second, objv, false);
	} catch (exception const &e) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(const_cast<char *>(e.what()), -1));
		return TCL_ERROR;
	} catch (...) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Unknown error.", -1));
		return TCL_ERROR;
	}

	return TCL_OK;
}
#endif
	
// generic "object" command handler
extern "C" int object_handler(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	// here, client data points to the singleton object
	// which is responsible for managing commands for
	// objects of a given type

	auto cdd = reinterpret_cast<method_handler_client_data *>(cd);

	//class_handler_base *chb = reinterpret_cast<class_handler_base *>(cd);

	// the command name has the form 'pXXX' where XXX is the address
	// of the "this" object

#if 0
	string const str(Tcl_GetString(objv[0]));
	istringstream ss(str);
	char dummy;
	void *p;
	ss >> dummy >> p;
#endif
	
	try {
		//string methodName(Tcl_GetString(objv[1]));
		//policies &pol = chb->get_policies(methodName);
		
		cdd->chb->invoke(cdd->tcli, cdd->obj, interp, objc, objv, false);
		//chb->invoke(p, interp, objc, objv);

		//post_process_policies(interp, pol, objv, true);
	} catch (exception const &e) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(const_cast<char *>(e.what()), -1));
		return TCL_ERROR;
	} catch (...) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Unknown error.", -1));
		return TCL_ERROR;
	}

	return TCL_OK;
}

extern "C" int managed_method_handler(ClientData cd, Tcl_Interp * interp, int objc, Tcl_Obj *CONST objv[]) {
	auto cdd = reinterpret_cast<managed_method_handler_client_data *>(cd);
	
	try {
		cdd->cb->invoke(cdd->interp, cdd->obj, interp, objc, objv, true);
	} catch (std::exception & e) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(const_cast<char *>(e.what()), -1));
		return TCL_ERROR;
	} catch (...) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Unknown error.", -1));
		return TCL_ERROR;
	}

	return TCL_OK;
}

static void method_handler_client_data_delete(ClientData cd) {
	auto * p = reinterpret_cast<method_handler_client_data *>(cd);
	if (p->unroll) {
		std::ostringstream oss;
		oss << 'p' << p->obj;
		p->chb->uninstall_methods(p->tcli, p->interp, oss.str().c_str());
	}
	delete p;
}
	
// generic "constructor" command
extern "C" int constructor_handler(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	// here, client data points to the singleton object
	// which is responsible for managing commands for
	// objects of a given type

	constructor_handler_client_data_t * up = (constructor_handler_client_data_t *) cd;

	try {
		up->cb->invoke(up->interp, nullptr, interp, objc, objv, false);

		// if everything went OK, the result is the address of the
		// new object in the 'pXXX' form
		// - we can create a new command with this name

		// convert it back to a pointer for faster method calls
		// object creation could be sped up by getting the raw pointer across
		string const str(Tcl_GetString(Tcl_GetObjResult(interp)) + interpreter::object_namespace_prefix_len);
		istringstream ss(str);
		void *p;
		ss >> p;

		method_handler_client_data * cd = new method_handler_client_data;
		cd->obj = p;
		cd->chb = up->chb;
		cd->unroll = false;
		cd->interp = nullptr;
		cd->tcli   = up->interp;
		
		//Tcl_CreateObjCommand(interp, Tcl_GetString(Tcl_GetObjResult(interp)), object_handler, static_cast<ClientData>(up->chb), 0);
		Tcl_CreateObjCommand(interp, Tcl_GetString(Tcl_GetObjResult(interp)), object_handler, static_cast<ClientData>(cd), method_handler_client_data_delete);
	} catch (exception const &e) {
		Tcl_SetResult(interp, const_cast<char *>(e.what()), TCL_VOLATILE);
		return TCL_ERROR;
	} catch (...) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Unknown error.", -1));
		return TCL_ERROR;
	}

	return TCL_OK;
}

extern "C" int managed_constructor_handler(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
	// here, client data points to the singleton object
	// which is responsible for managing commands for
	// objects of a given type

	constructor_handler_client_data_t * up = (constructor_handler_client_data_t *) cd;

	try {
		bool unroll = false;
		bool nocommand = false;

		Tcl_Obj ** objv2 = nullptr;
		if (objc > 1) {
			const char * arg1 = Tcl_GetString(objv[1]);

			if (strcmp(arg1, "-methods") == 0) {
				unroll = true;
			} else if (strcmp(arg1, "-nocommand") == 0) {
				nocommand = true;
			}

			if ((nocommand || unroll)) {
				if (objc > 2) {
					objv2 = new Tcl_Obj *[objc - 1];
					objv2[0] = objv[0];
					for (int j = 2; j < objc; ++j) {
						objv2[j - 1] = objv[j];
					}
				}
				--objc;
			}
		}

		if (objv2) {
			up->cb->invoke(up->interp, nullptr, interp, objc, objv2, false);
			delete[] objv2;
		} else {
			up->cb->invoke(up->interp, nullptr, interp, objc, objv, false);
		}

		//std::cerr << " managed construct " << Tcl_GetObjResult(interp)->refCount << "\n";
		
		// if everything went OK, the result is the address of the
		// new object in the 'pXXX' form
		// - we can create a new command with this name

		//string const str(Tcl_GetString(Tcl_GetObjResult(interp)) + interpreter::object_namespace_prefix_len);

		//istringstream ss(str);
		//void *p;
		//ss >> p;

		Tcl_Obj * ores = Tcl_GetObjResult(interp);
		
		method_handler_client_data * cd = new method_handler_client_data;
		void * p = cd->obj = ores->internalRep.twoPtrValue.ptr1;
		cd->chb = up->chb;
		cd->unroll = unroll;
		cd->interp = interp;
		cd->tcli   = up->interp;
		
		//Tcl_CreateObjCommand(interp, cmd.c_str(), object_handler, static_cast<ClientData>(up->chb), 0);

		if (unroll) {
			//std::cerr << "install methods\n";
			up->chb->install_methods(up->interp, interp, Tcl_GetString(Tcl_GetObjResult(interp)), p);
		}

		if (! nocommand) {
			std::string cmd = Tcl_GetString(Tcl_GetObjResult(interp));
			cmd += ".";
			//std::cerr << "ohandler " << (void *) object_handler << " " << (void *) cd << "\n";
			//std::cerr << "create command " << cmd << "\n";
			if (! Tcl_CreateObjCommand(interp, cmd.c_str(), object_handler, static_cast<ClientData>(cd), method_handler_client_data_delete)) {
				std::cerr << "cannot create command " << cmd << "\n";
			}

			//Tcl_CmdInfo cmdinfo;

			//Tcl_GetCommandInfo(interp, cmd.c_str(), &cmdinfo);

			//std::cerr << "cmdinfo proc=" << (void *) cmdinfo.objProc << " cd=" << cmdinfo.objClientData << " ns=" << cmdinfo.namespacePtr << "\n";

			//std::cerr << " managed construct2 " << Tcl_GetObjResult(interp)->refCount << "\n";
		}
	} catch (exception const &e) {
		Tcl_SetResult(interp, const_cast<char *>(e.what()), TCL_VOLATILE);
		return TCL_ERROR;
	} catch (...) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj("Unknown error.", -1));
		return TCL_ERROR;
	}

	return TCL_OK;
}

} // namespace

Tcl::details::no_init_type Tcl::no_init;

policies &policies::factory(string const &name) {
	factory_ = name;
	return *this;
}

policies &policies::sink(int index) {
	sinks_.push_back(index);
	return *this;
}

policies & policies::options(string const & options) {
	options_ = options;
	return *this;
}

#if 0
policies &policies::variadic() {
	variadic_ = true;
	return *this;
}
#endif

policies &policies::usage(string const &message) {
	usage_ = std::string("Usage: ") + message;
	return *this;
}

policies Tcl::factory(string const &name) { return policies().factory(name); }

policies Tcl::sink(int index) { return policies().sink(index); }

//policies Tcl::variadic() { return policies().variadic(); }

policies Tcl::options(std::string const & opt) { return policies().options(opt); }

policies Tcl::usage(string const &message) { return policies().usage(message); }

class_handler_base::class_handler_base() {
	// default policies for the -delete command
	//policies_["-delete"] = policies();
}

static void managed_method_client_data_delete(ClientData cd) {
	auto * p = reinterpret_cast<managed_method_handler_client_data *>(cd);
	delete[] p;
}

void class_handler_base::uninstall_methods(interpreter * tcli, Tcl_Interp * interp, const char * prefix) {
	char buf[1024];
	strcpy(buf, prefix);
	char * p = buf + strlen(buf);
	*p = '.'; ++p;
	
	for (auto it = methods_.begin(); it != methods_.end(); ++it) {
		strcpy(p, it->first);
		Tcl_DeleteCommand(interp, buf);
		//throw tcl_error(std::string("cannot register method ") + buf + " on object creation of managed class");
		//}
	}
}

void class_handler_base::install_methods(interpreter * tcli, Tcl_Interp * interp, const char * prefix, void * obj) {
	char buf[1024];
	strcpy(buf, prefix);
	char * p = buf + strlen(buf);
	*p = '.'; ++p;
	
	managed_method_handler_client_data * cds = new managed_method_handler_client_data[methods_.size()];
	int ix = 0;
	for (auto it = methods_.begin(); it != methods_.end(); ++it, ++ix) {
		strcpy(p, it->first);
		cds[ix].obj = obj;
		cds[ix].cb  = it->second;
		cds[ix].interp = tcli;
		//std::cerr << "install method " << buf << "\n";
		Tcl_CreateObjCommand(interp, buf, managed_method_handler, static_cast<ClientData>(cds + ix), ix == 0 ? managed_method_client_data_delete : nullptr);
		//throw tcl_error(std::string("cannot register method ") + buf + " on object creation of managed class");
		//}
	}
}

void class_handler_base::register_method(string const & name, callback_base * ocb, Tcl_Interp * interp, bool managed) {
	char * str = new char[name.size() + 1];
	strcpy(str, name.c_str());
	methods_[str] = ocb;
	//policies_[name] = p;
}

#if 0
policies &class_handler_base::get_policies(string const &name) {
	policies_map_type::iterator it = policies_.find(name);
	if (it == policies_.end()) {
		throw tcl_error("Trying to use non-existent policy: " + name);
	}

	return it->second;
}
#endif

thread_local Tcl_Obj * object::default_object_ = nullptr;

object::object() : interp_(nullptr) {
	if (! default_object_) { static_initialize(); }
	obj_ = default_object_;
	Tcl_IncrRefCount(obj_);
}
object::object(interpreter * interp) : interp_(interp) {
	if (! default_object_) { static_initialize(); }
	obj_ = default_object_;
	Tcl_IncrRefCount(obj_);
}

object::object(bool b) : interp_(nullptr) {
	obj_ = Tcl_NewBooleanObj(b);
	Tcl_IncrRefCount(obj_);
}
object::object(interpreter * interp, bool b) : interp_(interp) {
	obj_ = Tcl_NewBooleanObj(b);
	Tcl_IncrRefCount(obj_);
}

object::object(char const * buf, size_t size) : interp_(0) {
	obj_ = Tcl_NewByteArrayObj(reinterpret_cast<unsigned char const *>(buf), static_cast<int>(size));
	Tcl_IncrRefCount(obj_);
}
object::object(interpreter * interp, char const * buf, size_t size) : interp_(interp) {
	obj_ = Tcl_NewByteArrayObj(reinterpret_cast<unsigned char const *>(buf), static_cast<int>(size));
	Tcl_IncrRefCount(obj_);
}

object::object(double d) : interp_(0) {
	obj_ = Tcl_NewDoubleObj(d);
	Tcl_IncrRefCount(obj_);
}
object::object(interpreter * interp, double d) : interp_(interp) {
	obj_ = Tcl_NewDoubleObj(d);
	Tcl_IncrRefCount(obj_);
}

object::object(int i) : interp_(0) {
	obj_ = Tcl_NewIntObj(i);
	Tcl_IncrRefCount(obj_);
}
object::object(interpreter * interp, int i) : interp_(interp) {
	obj_ = Tcl_NewIntObj(i);
	Tcl_IncrRefCount(obj_);
}

object::object(long l) : interp_(0) {
	obj_ = Tcl_NewLongObj(l);
	Tcl_IncrRefCount(obj_);
}
object::object(interpreter * interp, long l) : interp_(interp) {
	obj_ = Tcl_NewLongObj(l);
	Tcl_IncrRefCount(obj_);
}

object::object(char const * s) : interp_(0) {
	obj_ = Tcl_NewStringObj(s, -1);
	Tcl_IncrRefCount(obj_);
}
object::object(interpreter * interp, char const * s) : interp_(interp) {
	obj_ = Tcl_NewStringObj(s, -1);
	Tcl_IncrRefCount(obj_);
}

object::object(string const & s) : interp_(0) {
	obj_ = Tcl_NewStringObj(s.data(), static_cast<int>(s.size()));
	Tcl_IncrRefCount(obj_);
}
object::object(interpreter * interp, string const & s) : interp_(interp) {
	obj_ = Tcl_NewStringObj(s.data(), static_cast<int>(s.size()));
	Tcl_IncrRefCount(obj_);
}

object::object(Tcl_Obj * o, bool shared) : interp_(0) { init(o, shared); }
object::object(interpreter * interp, Tcl_Obj * o, bool shared) : interp_(interp) { init(o, shared); }

object::object(object const & other, bool shared) : interp_(other.interp_) { init(other.obj_, shared); }

void object::init(Tcl_Obj * o, bool shared) {
	if (true || shared) {
		obj_ = o;
	} else {
		obj_ = Tcl_DuplicateObj(o);
	}
	Tcl_IncrRefCount(obj_);
}

object::~object() { Tcl_DecrRefCount(obj_); }

object &object::assign(bool b) {
	if (Tcl_IsShared(obj_)) {
		Tcl_DecrRefCount(obj_);
		obj_ = Tcl_NewBooleanObj(b ? 1 : 0);
		Tcl_IncrRefCount(obj_);
	} else {
		Tcl_SetBooleanObj(obj_, b ? 1 : 0);
	}
	return *this;
}

object &object::resize(size_t size) {
	dupshared();
	Tcl_SetByteArrayLength(obj_, static_cast<int>(size));
	return *this;
}

object &object::assign(char const *buf, size_t size) {
	if (Tcl_IsShared(obj_)) {
		Tcl_DecrRefCount(obj_);
		obj_ = Tcl_NewByteArrayObj(reinterpret_cast<unsigned char const *>(buf), static_cast<int>(size));
		Tcl_IncrRefCount(obj_);
	} else {
		Tcl_SetByteArrayObj(obj_, reinterpret_cast<unsigned char const *>(buf), static_cast<int>(size));
	}
	return *this;
}

object &object::assign(double d) {
	if (Tcl_IsShared(obj_)) {
		Tcl_DecrRefCount(obj_);
		obj_ = Tcl_NewDoubleObj(d);
		Tcl_IncrRefCount(obj_);
	} else {
		Tcl_SetDoubleObj(obj_, d);
	}
	return *this;
}

object &object::assign(int i) {
	if (Tcl_IsShared(obj_)) {
		Tcl_DecrRefCount(obj_);
		obj_ = Tcl_NewIntObj(i);
		Tcl_IncrRefCount(obj_);
	} else {
		Tcl_SetIntObj(obj_, i);
	}
	return *this;
}

object &object::assign(long l) {
	if (Tcl_IsShared(obj_)) {
		Tcl_DecrRefCount(obj_);
		obj_ = Tcl_NewLongObj(l);
		Tcl_IncrRefCount(obj_);
	} else {
		Tcl_SetLongObj(obj_, l);
	}
	return *this;
}

object &object::assign(char const *s) {
	if (Tcl_IsShared(obj_)) {
		Tcl_DecrRefCount(obj_);
		obj_ = Tcl_NewStringObj(s, -1);
		Tcl_IncrRefCount(obj_);
	} else {
		Tcl_SetStringObj(obj_, s, -1);
	}
	return *this;
}

object &object::assign(string const &s) {
	if (Tcl_IsShared(obj_)) {
		Tcl_DecrRefCount(obj_);
		obj_ = Tcl_NewStringObj(s.data(), static_cast<int>(s.size()));
		Tcl_IncrRefCount(obj_);
	} else {
		Tcl_SetStringObj(obj_, s.data(), static_cast<int>(s.size()));
	}
	return *this;
}

object &object::assign(object const &other) {
	object(other).swap(*this);
	return *this;
}

object &object::assign(Tcl_Obj *o) {
	object(o).swap(*this);
	return *this;
}

object object::duplicate() const {
	Tcl_Obj * to = Tcl_DuplicateObj(obj_);
	return object(interp_, to);
}

object &object::swap(object &other) {
	std::swap(obj_, other.obj_);
	std::swap(interp_, other.interp_);
	return *this;
}

template <typename T> T object::get() const { //interpreter &i) const {
	return tcl_cast<T>::from(interp_ ? interp_->get() : nullptr, obj_, false);
}

template bool         object::get<bool        >() const;
template double       object::get<double      >() const;
template float        object::get<float       >() const;
template int          object::get<int         >() const;
template long         object::get<long        >() const;
template char const * object::get<char const *>() const;
template std::string  object::get<std::string >() const;
template std::vector<char> object::get<std::vector<char>>() const;

string object::asString() const { return get<string>(); }
int    object::asInt()    const { return get<int>(); }
bool   object::asBool()   const { return get<bool>(); }
long   object::asLong()   const { return get<long>(); }
double object::asDouble() const { return get<double>(); }

char const *object::get() const { return Tcl_GetString(obj_); }

char const *object::get(size_t &size) const {
	int len;
	unsigned char *buf = Tcl_GetByteArrayFromObj(obj_, &len);
	size = len;
	return const_cast<char const *>(reinterpret_cast<char *>(buf));
}

bool object::is_list() const {
	return obj_->typePtr && strcmp(obj_->typePtr->name, "list") == 0;
}

size_t object::size() const {
	int len;
	if (Tcl_ListObjLength(interp_ ? interp_->get_interp() : nullptr, obj_, &len) != TCL_OK) {
		throw tcl_error(interp_->get_interp());
	}
	return static_cast<size_t>(len);
}

object object::at_ref(size_t index) const {
	Tcl_Obj *o;
	if (Tcl_ListObjIndex(interp_ ? interp_->get_interp() : nullptr, obj_, static_cast<int>(index), &o) != TCL_OK) {
		throw tcl_error(interp_->get_interp());
	}
	if (o == NULL) {
		throw tcl_error("Index out of range.");
	}
	return object(o, true);
}

object object::at(size_t index) const {
	Tcl_Obj *o;
	if (Tcl_ListObjIndex(interp_ ? interp_->get_interp() : nullptr, obj_, static_cast<int>(index), &o) != TCL_OK) {
		throw tcl_error(interp_->get_interp());
	}
	if (o == NULL) {
		throw tcl_error("Index out of range.");
	}
	return object(o);
}

object &object::append(object const &o) {
	dupshared();

	if (Tcl_ListObjAppendElement(interp_ ? interp_->get_interp() : nullptr, obj_, o.obj_) != TCL_OK) {
		throw tcl_error(interp_->get_interp());
	}
	return *this;
}

object &object::append_list(object const &o) {
	dupshared();

	if (Tcl_ListObjAppendList(interp_ ? interp_->get_interp() : nullptr, obj_, o.obj_) != TCL_OK) {
		throw tcl_error(interp_->get_interp());
	}
	return *this;
}

object &object::replace(size_t index, size_t count, object const &o) {
	dupshared();
	int res = Tcl_ListObjReplace(interp_ ? interp_->get_interp() : nullptr, obj_, static_cast<int>(index), static_cast<int>(count), 1, &(o.obj_));
	if (res != TCL_OK) {
		throw tcl_error(interp_->get_interp());
	}
	return *this;
}

object &object::replace_list(size_t index, size_t count, object const &o) {
	int objc;
	Tcl_Obj **objv;

	int res = Tcl_ListObjGetElements(interp_ ? interp_->get_interp() : nullptr, o.obj_, &objc, &objv);
	if (res != TCL_OK) {
		throw tcl_error(interp_->get_interp());
	}
	dupshared();
	res = Tcl_ListObjReplace(interp_ ? interp_->get_interp() : nullptr, obj_, static_cast<int>(index), static_cast<int>(count), objc, objv);
	if (res != TCL_OK) {
		throw tcl_error(interp_->get_interp());
	}
	return *this;
}

//void object::set_interp(Tcl_Interp *interp) { interp_ = interp; }

//Tcl_Interp *object::get_interp() const { return interp_; }

Tcl::interpreter *interpreter::defaultInterpreter = nullptr;

void interpreter::unsetVar(std::string const & name) {
	Tcl_UnsetVar(get_interp(), name.c_str(), 0);
}

details::result interpreter::upVar(std::string const & frame, std::string const & srcname, std::string const & destname) {
	if (Tcl_UpVar(interp_, frame.c_str(), srcname.c_str(), destname.c_str(), 0) == TCL_OK) {
		return getVar(destname);
	}
	return result(interp_);
}


int interpreter::dot_call_handler(ClientData cd, Tcl_Interp * interp, int objc, Tcl_Obj * CONST * objv) {
	interpreter * this_p = (interpreter *) cd;
	if (objc > 1) {
		Tcl_Obj * o = objv[1];
		if (Tcl_ObjType const * ot = o->typePtr) {
			if (ot[1].name) {
				auto it = this_p->all_classes_by_objtype_.find(ot);
				if (it != this_p->all_classes_by_objtype_.end()) {
					it->second.class_handler->invoke(this_p, o->internalRep.twoPtrValue.ptr1, interp, objc - 1, objv + 1, false);
				} else {
					std::cerr << "primary argument is not a registered cpptcl object\n";
					return TCL_ERROR;
				}
			} else {
				assert(ot[-1].name);
				auto it = this_p->all_classes_by_objtype_.find(ot - 1);
				if (it != this_p->all_classes_by_objtype_.end()) {
					if (it->second.is_inline) {
						it->second.class_handler->invoke(this_p, (void *) &o->internalRep, interp, objc - 1, objv + 1, false);
					} else {
						it->second.class_handler->invoke(this_p, o->internalRep.twoPtrValue.ptr1, interp, objc - 1, objv + 1, false);
					}
				} else {
					std::cerr << "primary argument is not a registered cpptcl object\n";
					return TCL_ERROR;
				}
			}				
		} else {
			std::cerr << "primary argument in object call is not a tcl object\n";
			return TCL_ERROR;
		}
	} else {
		std::cerr << "object call requires at least 2 parameters\n";
		return TCL_ERROR;
	}
	return TCL_OK;
}

interpreter::interpreter() : tin_(nullptr), tout_(nullptr), terr_(nullptr) {
	interp_ = Tcl_CreateInterp();
	owner_ = true;
	if (defaultInterpreter) {
		throw tcl_error("expecting a single interpreter");
	}
	find_standard_types();
	Tcl_CreateObjCommand(interp_, ".", dot_call_handler, (ClientData) this, nullptr);
}

interpreter::interpreter(Tcl_Interp *interp, bool owner) : tin_(nullptr), tout_(nullptr), terr_(nullptr) {
	interp_ = interp;

	if (! interp_) { interp_ = Tcl_CreateInterp(); }
	owner_ = owner;
	if (!defaultInterpreter) {
		if (Tcl_InitStubs(interp_, "8.6", 0) == NULL) {
			throw tcl_error("Failed to initialize stubs");
		}
		// Make a copy
	}
	find_standard_types();
	if (!defaultInterpreter) {
		defaultInterpreter = new interpreter(*this);
	}
	Tcl_CreateObjCommand(interp_, ".", dot_call_handler, (ClientData) this, nullptr);
}

interpreter::interpreter(const interpreter &i) : interp_(i.interp_), owner_(i.owner_), tin_(nullptr), tout_(nullptr), terr_(nullptr),
												 list_type_(i.list_type_), cmdname_type_(i.cmdname_type_), object_namespace_(i.object_namespace_) {
}

void interpreter::custom_construct(const char * c_name, const char * o_name, void * p) {
	auto it2 = all_classes_.find(c_name);
	if (it2 != all_classes_.end()) {
		auto * cd = new method_handler_client_data;
		cd->obj = p;
		cd->chb = it2->second.get();
		cd->unroll = false;
		cd->interp = nullptr;
		Tcl_CreateObjCommand(interp_, o_name, object_handler, static_cast<ClientData>(cd), method_handler_client_data_delete);
		return;
	}
	throw tcl_error("custom construct failed");
}

void interpreter::find_standard_types() {
	list_type_                = Tcl_GetObjType("list");
	cmdname_type_             = Tcl_GetObjType("cmdName");
	object_namespace_         = Tcl_FindNamespace(interp_, object_namespace_name, Tcl_GetGlobalNamespace(interp_), 0);
	//object_command_namespace_ = Tcl_FindNamespace(interp_, object_command_namespace_name, nullptr, 0);
}

interpreter::~interpreter() {
	if (owner_) {
		// clear all callback info belonging to this interpreter
		clear_definitions(interp_);
		Tcl_DeleteInterp(interp_);
	}
}

void interpreter::make_safe() {
	if (Tcl_MakeSafe(interp_) != TCL_OK) {
		throw tcl_error(interp_);
	}
}

result interpreter::eval(string const &script) {
	int cc = Tcl_Eval(interp_, script.c_str());
	if (cc != TCL_OK) {
		if (want_abort_) {
			want_abort_ = false;
			throw tcl_aborted();
		} else {
			throw tcl_error(interp_);
		}
	}
	return result(interp_);
}

result interpreter::eval(istream &s) {
	string str(istreambuf_iterator<char>(s.rdbuf()), istreambuf_iterator<char>());
	return eval(str);
}

result interpreter::eval(object const &o) {
	int cc = Tcl_EvalObjEx(interp_, o.get_object(), 0);
	if (cc != TCL_OK) {
		if (want_abort_) {
			want_abort_ = false;
			throw tcl_aborted();
		} else {
			throw tcl_error(interp_);
		}
	}
	return result(interp_);
}

result interpreter::getVar(string const &variableName, string const &indexName) {
	object n = object(variableName.c_str());
	object i = object(indexName.c_str());
	Tcl_Obj * obj = Tcl_ObjGetVar2(interp_, n.get_object(), i.get_object(), 0);
	if (obj == NULL) {
		std::cerr << "throw!\n";
		throw tcl_error("no such variable: " + variableName + "(" + indexName + ")");
	} else {
        Tcl_SetObjResult(interp_, obj);
	}
	return result(interp_);
}

result interpreter::getVar(string const &variableName) {
	object n = object(variableName.c_str());
    Tcl_Obj * obj = Tcl_ObjGetVar2(interp_, n.get_object(), nullptr, 0);
	if (obj == NULL) {
		throw tcl_error("no such variable: " + variableName);
	} else {
        Tcl_SetObjResult(interp_, obj);
	}
	return result(interp_);
}

bool interpreter::exists(string const &variableName, string const &indexName) {
    object n = object(variableName.c_str());
    object i = object(indexName.c_str());
    Tcl_Obj * obj = Tcl_ObjGetVar2(interp_, n.get_object(), i.get_object(), 0);
    return (obj != NULL);
}

bool interpreter::exists(string const &variableName) {
    object n = object(variableName.c_str());
    Tcl_Obj * obj = Tcl_ObjGetVar2(interp_, n.get_object(), nullptr, 0);
    return (obj != NULL);
}

void interpreter::pkg_provide(string const &name, string const &version) {
	int cc = Tcl_PkgProvide(interp_, name.c_str(), version.c_str());
	if (cc != TCL_OK) {
		throw tcl_error(interp_);
	}
}

void interpreter::create_namespace(string const &name) {
	if ( Tcl_CreateNamespace(interp_, name.c_str(), 0, 0) == 0) {
		throw tcl_error(interp_);
	}
}

void interpreter::create_alias(string const &cmd, interpreter &targetInterp, string const &targetCmd) {
	int cc = Tcl_CreateAlias(interp_, cmd.c_str(), targetInterp.interp_, targetCmd.c_str(), 0, 0);
	if (cc != TCL_OK) {
		throw tcl_error(interp_);
	}
}

void interpreter::clear_definitions(Tcl_Interp *interp) {
	for (auto & c : all_callbacks_) {
		Tcl_DeleteCommand(get_interp(), c.first.c_str());
	}

#if 0
	{
		auto it = class_handlers.find(interp);
		if (it != class_handlers.end()) {
			for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
				delete it2->second;
			}
		}
		class_handlers.erase(interp);
	}
#endif
}

void interpreter::add_function(string const &name, std::shared_ptr<callback_base> cb) {
	//Tcl_CreateObjCommand(interp_, name.c_str(), callback_handler, cb, 0);
	cb->install(this);
	all_callbacks_.insert(std::pair(name, cb));
	//all_definitions[interp_][name] = cb;
}

void interpreter::add_class(string const &name, std::shared_ptr<class_handler_base> chb, bool is_inline) {
	//class_handlers[name] = chb;
	all_classes_.insert(std::pair(name, chb));
	Tcl_ObjType * ot = get_objtype(name.c_str());
	if (ot) {
		all_classes_by_objtype_.insert(std::pair(ot, all_classes_by_objtype_value{ chb, is_inline }));
	}
}
details::class_handler_base * interpreter::get_class_handler(std::string const & name) {
	auto it2 = all_classes_.find(name);
	if (it2 == all_classes_.end()) return nullptr;
	return it2->second.get();
}


void interpreter::add_constructor(string const &name, class_handler_base * chb, std::shared_ptr<callback_base> cb) {
	constructor_handler_client_data_t * up = new constructor_handler_client_data_t;
	up->cb = cb.get();
	up->chb = chb;
	up->interp = this;
	
	Tcl_CreateObjCommand(interp_, name.c_str(), constructor_handler, up, 0);
	all_callbacks_.insert(std::pair(name, cb));
	//	all_definitions[interp_][name] = cb;
}

void interpreter::add_managed_constructor(string const &name, class_handler_base * chb, std::shared_ptr<callback_base> cb) {
	constructor_handler_client_data_t * up = new constructor_handler_client_data_t;
	up->cb = cb.get();
	up->chb = chb;
	up->interp = this;
	
	Tcl_CreateObjCommand(interp_, name.c_str(), managed_constructor_handler, up, 0);
	all_callbacks_.insert(std::pair(name, cb));
	//all_definitions[interp_][name] = cb;
}

int tcl_cast<int>::from(Tcl_Interp *interp, Tcl_Obj *obj, bool) {
	int res;
	if (Tcl_GetIntFromObj(interp, obj, &res) != TCL_OK) {
		throw tcl_error(interp);
	}
	return res;
}
unsigned int tcl_cast<unsigned int>::from(Tcl_Interp *interp, Tcl_Obj *obj, bool) {
	int res;
	if (Tcl_GetIntFromObj(interp, obj, &res) != TCL_OK) {
		throw tcl_error(interp);
	}
	return res;
}


long tcl_cast<long>::from(Tcl_Interp *interp, Tcl_Obj *obj, bool) {
	long res;
	if (Tcl_GetLongFromObj(interp, obj, &res) != TCL_OK) {
		throw tcl_error(interp);
	}
	return res;
}

bool tcl_cast<bool>::from(Tcl_Interp *interp, Tcl_Obj *obj, bool) {
	int res;
	if (Tcl_GetBooleanFromObj(interp, obj, &res) != TCL_OK) {
		throw tcl_error(interp);
	}
	return res != 0;
}

double tcl_cast<double>::from(Tcl_Interp *interp, Tcl_Obj *obj, bool) {
	double res;
	if (Tcl_GetDoubleFromObj(interp, obj, &res) != TCL_OK) {
		throw tcl_error(interp);
	}
	return res;
}

float tcl_cast<float>::from(Tcl_Interp * interp, Tcl_Obj * obj, bool b) {
	return tcl_cast<double>::from(interp, obj, b);
}

string tcl_cast<string>::from(Tcl_Interp *, Tcl_Obj *obj, bool) { return Tcl_GetString(obj); }
char const *tcl_cast<char const *>::from(Tcl_Interp *, Tcl_Obj *obj, bool) { return Tcl_GetString(obj); }

object tcl_cast<object>::from(Tcl_Interp * interp, Tcl_Obj * obj, bool) {
	object o(obj);
	//o.set_interp(interp);
	return o;
}

std::vector<char> tcl_cast<std::vector<char> >::from(Tcl_Interp * interp, Tcl_Obj * obj, bool asref) {
	std::string s = tcl_cast<string>::from(interp, obj, asref);
	return std::vector<char>(s.begin(), s.end());
}

namespace Tcl {
// helper function for post-processing call policies
// for both free functions (isMethod == false)
// and class methods (isMethod == true)
void interpreter::post_process_policies(Tcl_Interp *interp, policies &pol, Tcl_Obj *CONST objv[], bool isMethod) {
	// check if it is a factory
	if (!pol.factory_.empty()) {
		auto oit = all_classes_.find(pol.factory_);
		if (oit == all_classes_.end()) {
			throw tcl_error("Factory was registered for unknown class.");
		}

		class_handler_base *chb = oit->second.get();

		// register a new command for the object returned
		// by this factory function
		// if everything went OK, the result is the address of the
		// new object in the 'pXXX' form
		// - the new command will be created with this name

		Tcl_Obj * to = Tcl_GetObjResult(interp);
		void * p = nullptr;
		if (to->internalRep.otherValuePtr) {
			p = to->internalRep.otherValuePtr;
		} else {
			string const str(Tcl_GetString(to) + interpreter::object_namespace_prefix_len);
			istringstream ss(str);
			ss >> p;
		}

		
		auto * cd = new method_handler_client_data;
		cd->obj = p;
		cd->chb = chb;
		cd->unroll = false;
		cd->interp = nullptr;
		cd->tcli   = this;
		
		Tcl_CreateObjCommand(interp, Tcl_GetString(Tcl_GetObjResult(interp)), object_handler, static_cast<ClientData>(cd), 0);
	}

	// process all declared sinks
	// - unregister all object commands that envelopes the pointers
	for (vector<int>::iterator s = pol.sinks_.begin(); s != pol.sinks_.end(); ++s) {
		if (isMethod == false) {
			// example: if there is a declared sink at parameter 3,
			// and the Tcl command was:
			// % fun par1 par2 PAR3 par4
			// then the index 3 correctly points into the objv array

			int index = *s;
			Tcl_DeleteCommand(interp, Tcl_GetString(objv[index]));
		} else {
			// example: if there is a declared sink at parameter 3,
			// and the Tcl command was:
			// % $p method par1 par2 PAR3 par4
			// then the index 3 needs to be incremented
			// in order correctly point into the 4th index of objv array

			int index = *s + 1;
			Tcl_DeleteCommand(interp, Tcl_GetString(objv[index]));
		}
	}
}

tcl_error::tcl_error(std::string const & msg) : msg_(msg), std::runtime_error(msg) {
	backtrace_size_ = backtrace(backtrace_, sizeof(backtrace_) / sizeof(backtrace_[0]));
}
tcl_error::tcl_error(Tcl_Interp * interp) : std::runtime_error(Tcl_GetString(Tcl_GetObjResult(interp))), msg_(Tcl_GetString(Tcl_GetObjResult(interp))) {}
const char * tcl_error::what() const throw() {
	std::ostringstream oss;
	oss << msg_ << "\n";
#if 0
	char ** b = backtrace_symbols(backtrace_, backtrace_size_);
	for (int i = 0; i < backtrace_size_; ++i) {
		bool done = false;
		const char * pos = index(b[i], '(');
		if (pos) {
			++pos;
			const char * epos = index(pos, '+');
			if (epos) {
				oss << std::string(b[i], pos - b[i]);
				std::string str(pos, epos - pos);
				oss << boost::core::demangle(str.c_str());
				oss << epos << "\n";
				done = true;
			}
		}
		if (! done) {
			oss << boost::core::demangle(b[i]) << "\n";
		}
	}
	free(b);
#endif
	return strdup(oss.str().c_str());
}

	decltype(interpreter::obj_type_by_tclname_) interpreter::obj_type_by_tclname_;
	decltype(interpreter::obj_type_by_cppname_) interpreter::obj_type_by_cppname_;
	
}
