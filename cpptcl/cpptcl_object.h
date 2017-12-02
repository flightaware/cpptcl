//
//  object.h
//  cpplib
//
//  Created by Shannon Noe on 11/9/17.
//

#ifndef CPPTCL_OBJECT_H
#define CPPTCL_OBJECT_H

class object;

template <typename T, typename O> class maybe_object {
private:
	interpreter const & interp_;
	T * obj_;
	std::string name_;
	std::string index_;

public:
	maybe_object(T *obj, interpreter const & interp, std::string name, std::string index): interp_(interp), obj_(obj), name_(name), index_(index) {}

	bool has_value() const {
		return obj_ != 0;
	}

	bool exists() const {
		return has_value();
	}

	operator bool() const {
		return has_value();
	}

	O get() const {
		if (obj_ == 0) {
			throw tcl_error(std::string("array ") + name_ + " does not have field " + index_);
		}
		return O(obj_);
	}

	O operator *() const {
		return get();
	}

	std::string asString() const {
		return get().asString();
	}

	int asInt() const {
		return get().asInt();
	}

	long asLong() const {
		return get().asLong();
	}

	double asDouble() const {
		return get().asDouble();
	}
};


// object wrapper
class object {
  public:
	// constructors

	object();

	explicit object(bool b);
	object(char const *buf, size_t size); // byte array
	explicit object(double b);
	explicit object(int i);

	// list creation
	// the InputIterator should give object& or Tcl_Obj* when dereferenced
	template <class InputIterator> object(InputIterator first, InputIterator last) : interp_(0) {
		std::vector<Tcl_Obj *> v;
		fill_vector(v, first, last);
		obj_ = Tcl_NewListObj(static_cast<int>(v.size()), v.empty() ? NULL : &v[0]);
		Tcl_IncrRefCount(obj_);
	}

	explicit object(long i);
	explicit object(char const *s);		   // string construction
	explicit object(std::string const &s); // string construction

	explicit object(Tcl_Obj *o, bool shared = false);

	object(object const &other, bool shared = false);
	~object();

	// assignment

	object &assign(bool b);
	object &resize(size_t size);				  // byte array resize
	object &assign(char const *buf, size_t size); // byte array assignment
	object &assign(double d);
	object &assign(int i);

	// list assignment
	// the InputIterator should give Tcl_Obj* or object& when dereferenced
	template <class InputIterator> object &assign(InputIterator first, InputIterator last) {
		std::vector<Tcl_Obj *> v;
		fill_vector(v, first, last);
		Tcl_SetListObj(obj_, static_cast<int>(v.size()), v.empty() ? NULL : &v[0]);
		return *this;
	}

	object &assign(long l);
	object &assign(char const *s);		  // string assignment
	object &assign(std::string const &s); // string assignment
	object &assign(object const &o);
	object &assign(Tcl_Obj *o);

	object &operator=(bool b) { return assign(b); }
	object &operator=(double d) { return assign(d); }
	object &operator=(int i) { return assign(i); }
	object &operator=(long l) { return assign(l); }
	object &operator=(char const *s) { return assign(s); }
	object &operator=(std::string const &s) { return assign(s); }

	object &operator=(object const &o) { return assign(o); }
	object &swap(object &other);

	// (logically) non-modifying members

	template <typename T> T get(interpreter &i = *interpreter::defaultInterpreter) const;

	char const *get() const;			 // string get
	char const *get(size_t &size) const; // byte array get

	size_t size(interpreter &i = *interpreter::defaultInterpreter) const; // returns list length
	object at(size_t index, interpreter &i = *interpreter::defaultInterpreter) const;

	Tcl_Obj *get_object() const { return obj_; }

	// modifying members

	object &append(object const &o, interpreter &i = *interpreter::defaultInterpreter);
	object &append_list(object const &o, interpreter &i = *interpreter::defaultInterpreter);

	// list replace
	// the InputIterator should give Tcl_Obj* or object& when dereferenced
	template <class InputIterator> object &replace(size_t index, size_t count, InputIterator first, InputIterator last, interpreter &i = *interpreter::defaultInterpreter) {
		std::vector<Tcl_Obj *> v;
		fill_vector(v, first, last);
		int res = Tcl_ListObjReplace(i.get(), obj_, static_cast<int>(index), static_cast<int>(count), static_cast<int>(v.size()), v.empty() ? NULL : &v[0]);
		if (res != TCL_OK) {
			throw tcl_error(i.get());
		}

		return *this;
	}

	object &replace(size_t index, size_t count, object const &o, interpreter &i = *interpreter::defaultInterpreter);
	object &replace_list(size_t index, size_t count, object const &o, interpreter &i = *interpreter::defaultInterpreter);

	// helper functions for piggy-backing interpreter info
	void set_interp(Tcl_Interp *interp);
	Tcl_Interp *get_interp() const;

	// helper function, also used from interpreter::eval
	template <class InputIterator> static void fill_vector(std::vector<Tcl_Obj *> &v, InputIterator first, InputIterator last) {
		for (; first != last; ++first) {
			object o(*first, true);
			v.push_back(o.obj_);
		}
	}

	const maybe_object<Tcl_Obj, object> operator()(std::string const & idx) const {
		Tcl_Obj *array = obj_;
		const char *name = Tcl_GetString(array);
		Tcl_Obj *o = Tcl_GetVar2Ex(interp_, name, idx.c_str(), TCL_LEAVE_ERR_MSG);
		return maybe_object<Tcl_Obj, object>(o, interp_, std::string(name), idx);
	}

	const bool exists(std::string idx) const {
		Tcl_Obj *array = obj_;
		Tcl_Obj *o = Tcl_GetVar2Ex(interp_, Tcl_GetString(array), idx.c_str(), 0);
		return (o != 0);
	}

	std::string asString() const;
	int asInt() const;
	long asLong() const;
	double asDouble() const;

  private:
	// helper function used from copy constructors
	void init(Tcl_Obj *o, bool shared);

  public:
	Tcl_Obj *obj_;
	Tcl_Interp *interp_;
};

// available specializations for object::get
template <> bool object::get<bool>(interpreter &i) const;
template <> double object::get<double>(interpreter &i) const;
template <> int object::get<int>(interpreter &i) const;
template <> long object::get<long>(interpreter &i) const;
template <> char const *object::get<char const *>(interpreter &i) const;
template <> std::string object::get<std::string>(interpreter &i) const;
template <> std::vector<char> object::get<std::vector<char>>(interpreter &i) const;

class upvar {
  public:
	upvar(Tcl_Obj *obj) : objtarget(object(obj, true)) {}
	void set_interp(Tcl_Interp *interp) { this->interp = interp; }
	Tcl_Interp *get_interp() const { return interp; }
	object const &get() const { return objtarget; }

  private:
	object objtarget;
	Tcl_Interp *interp;
};

#endif /* CPPTCL_OBJECT_H */
