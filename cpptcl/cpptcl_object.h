//
//  object.h
//  cpplib
//
//  Created by Shannon Noe on 11/9/17.
//

#ifndef CPPTCL_OBJECT_H
#define CPPTCL_OBJECT_H

class object;

/*
 * Allow object to hold a optional value of the same class object. By using a template, O can be replaced with object
 * when the class object is actually defined.
 */
template <typename O> class maybe_object {
private:
	interpreter const & interp_;
	Tcl_Obj * obj_;
	// name and index are just for producing nice error messages
	std::string name_;
	std::string index_;

public:
	maybe_object(Tcl_Obj *obj, interpreter const & interp, std::string name, std::string index): interp_(interp), obj_(obj), name_(name), index_(index) {}

	bool has_value() const {
		return obj_ != 0;
	}

	bool exists() const {
		return has_value();
	}

	operator bool() const {
		return has_value();
	}

	/*
	 * This value reference cannot compile until object is known.
	 */
	O get() const {
		if (obj_ == 0) {
			throw tcl_error(std::string("no such element '" + index_ + "' in array '" + name_ + "'"));
		}
		return O(obj_);
	}

	O operator *() const {
		return get();
	}

	std::string asString() const {
		return get().asString();
	}

	bool asBool() const {
		return get().asBool();
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
	explicit object(long i);
	explicit object(char const *s);		   // string construction
	explicit object(std::string const &s); // string construction
	explicit object(Tcl_Obj *o, bool shared = false);


	object(interpreter *);
	explicit object(interpreter *, bool b);
	object(interpreter *, char const *buf, size_t size); // byte array
	explicit object(interpreter *, double b);
	explicit object(interpreter *, int i);
	explicit object(interpreter *, long i);
	explicit object(interpreter *, char const *s);		   // string construction
	explicit object(interpreter *, std::string const &s); // string construction
	explicit object(interpreter *, Tcl_Obj *o, bool shared = false);

	
	// list creation
	// the InputIterator should give object& or Tcl_Obj* when dereferenced
	template <class InputIterator> object(InputIterator first, InputIterator last) : interp_(0) {
		std::vector<Tcl_Obj *> v;
		fill_vector(v, first, last);
		obj_ = Tcl_NewListObj(static_cast<int>(v.size()), v.empty() ? NULL : &v[0]);
		Tcl_IncrRefCount(obj_);
	}
	template <class InputIterator> object(interpreter * interp, InputIterator first, InputIterator last) : interp_(interp) {
		std::vector<Tcl_Obj *> v;
		fill_vector(v, first, last);
		obj_ = Tcl_NewListObj(static_cast<int>(v.size()), v.empty() ? NULL : &v[0]);
		Tcl_IncrRefCount(obj_);
	}

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

	//template <typename T> T get(interpreter &i = *interpreter::defaultInterpreter) const;
	template <typename T> T get() const; //interpreter &i = *interpreter::defaultInterpreter) const;

	char const *get() const;			 // string get
	char const *get(size_t &size) const; // byte array get

	size_t size() const; //interpreter &i = *interpreter::defaultInterpreter) const; // returns list length
	object at(size_t index) const; //, interpreter &i = *interpreter::defaultInterpreter) const;
	object at_ref(size_t index) const; //, interpreter &i = *interpreter::defaultInterpreter) const;

	bool is_list() const;
	
	Tcl_Obj *get_object() const { return obj_; }
	Tcl_Obj * get_object_refcounted() {
		Tcl_IncrRefCount(obj_);
		return obj_;
	}
	
	// modifying members

	object &append(object const &o); //, interpreter &i = *interpreter::defaultInterpreter);
	object &append_list(object const &o); //, interpreter &i = *interpreter::defaultInterpreter);

	// list replace
	// the InputIterator should give Tcl_Obj* or object& when dereferenced
	template <class InputIterator> object &replace(size_t index, size_t count, InputIterator first, InputIterator last) { //, interpreter &i = *interpreter::defaultInterpreter) {
		std::vector<Tcl_Obj *> v;
		fill_vector(v, first, last);
		int res = Tcl_ListObjReplace(interp_->get_interp(), obj_, static_cast<int>(index), static_cast<int>(count), static_cast<int>(v.size()), v.empty() ? NULL : &v[0]);
		if (res != TCL_OK) {
			throw tcl_error(interp_->get_interp());
		}

		return *this;
	}

	object &replace(size_t index, size_t count, object const &o); //, interpreter &i = *interpreter::defaultInterpreter);
	object &replace_list(size_t index, size_t count, object const &o); //, interpreter &i = *interpreter::defaultInterpreter);

	// helper functions for piggy-backing interpreter info
	//void set_interp(Tcl_Interp *interp);
	//Tcl_Interp *get_interp() const;

	// helper function, also used from interpreter::eval
	template <class InputIterator> static void fill_vector(std::vector<Tcl_Obj *> &v, InputIterator first, InputIterator last) {
		for (; first != last; ++first) {
			object o(*first, true);
			v.push_back(o.obj_);
		}
	}

	const maybe_object<object> operator()(std::string const & idx) const {
		Tcl_Obj *array = obj_;
		const char *name = Tcl_GetString(array);
		Tcl_Obj *o = Tcl_GetVar2Ex(interp_->get_interp(), name, idx.c_str(), TCL_LEAVE_ERR_MSG);
		return maybe_object<object>(o, *interp_, std::string(name), idx);
	}

	bool exists(std::string idx) const {
		Tcl_Obj *array = obj_;
		Tcl_Obj *o = Tcl_GetVar2Ex(interp_->get_interp(), Tcl_GetString(array), idx.c_str(), 0);
		return (o != 0);
	}
    
    // Bind variable to name in TCL interpreter
    // This increases the ref count so if the C++ object
    // leaves scope the variable exists
    void bind(std::string const& variableName) {
        object n(variableName);
        Tcl_IncrRefCount(obj_);
        //if (interp_ == nullptr) {
        //    interp_ = interpreter::getDefault()->get();
        //}
        Tcl_ObjSetVar2(interp_->get_interp(), n.get_object(), nullptr, obj_, 0);
    }

    // Bind array value in TCL interpreter
    // This increases the ref count so if the C++ object
    // leaves scope the variable exists
    void bind(std::string const& variableName, std::string const& indexName) {
        object n(variableName);
        object i(indexName);
        Tcl_IncrRefCount(obj_);
        //if (interp_ == nullptr) {
        //    interp_ = interpreter::getDefault()->get();
        //}
        Tcl_ObjSetVar2(interp_->get_interp(), n.get_object(), i.get_object(), obj_, 0);
    }

	std::string asString() const;
	int asInt() const;
	bool asBool() const;
	long asLong() const;
	double asDouble() const;

  private:
	// helper function used from copy constructors
	void init(Tcl_Obj *o, bool shared);

	static thread_local Tcl_Obj * default_object_;
	static void static_initialize() {
		default_object_ = Tcl_NewObj();
		Tcl_IncrRefCount(default_object_);
	}
	void dupshared() {
		if (Tcl_IsShared(obj_)) {
			Tcl_Obj * newo = Tcl_DuplicateObj(obj_);
			Tcl_IncrRefCount(newo);
			Tcl_DecrRefCount(obj_);
			obj_ = newo;
		}
	}
 public:
	static object make() {
		//if (! default_object_) {
		//	static_initialize();
		//}
		return object().duplicate();
	}
	void disown() {
		Tcl_DecrRefCount(obj_);
	}
	void reown() {
		Tcl_IncrRefCount(obj_);
	}
	object duplicate() const;
	bool shared() const {
		return Tcl_IsShared(obj_);
	}
	Tcl_Obj *obj_;
	interpreter * interp_;
	//Tcl_Interp *interp_;
};

#endif /* CPPTCL_OBJECT_H */
