//
// Copyright (C) 2004-2006, Maciej Sobczak
// Copyright (C) 2017-2019, FlightAware LLC
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//

// Note: this file is not supposed to be a stand-alone header

template <class C, typename R> class method0 : public object_cmd_base {
	typedef R (C::*mem_type)();
	typedef R (C::*cmem_type)() const;

  public:
	method0(mem_type f) : f_(f), cmem_(false) {}
	method0(cmem_type f) : cf_(f), cmem_(true) {}

	virtual void invoke(void *pv, Tcl_Interp *interp, int, Tcl_Obj *CONST[], policies const &) {
		C *p = static_cast<C *>(pv);
		if (cmem_) {
			dispatch<R>::do_dispatch(interp, std::bind(cf_, p));
		} else {
			dispatch<R>::do_dispatch(interp, std::bind(f_, p));
		}
	}

  private:
	mem_type f_;
	cmem_type cf_;
	bool cmem_;
};

template <class C, typename R, typename T1> class method1 : public object_cmd_base {
	typedef R (C::*mem_type)(T1);
	typedef R (C::*cmem_type)(T1) const;

  public:
	method1(mem_type f) : f_(f), cmem_(false) {}
	method1(cmem_type f) : cf_(f), cmem_(true) {}

	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) {
		check_params_no(objc, 3, pol.usage_);
		tcl_cast_by_reference<T1> byRef;

		C *p = static_cast<C *>(pv);
		if (cmem_) {
			dispatch<R>::template do_dispatch<T1>(interp, std::bind(cf_, p, std::placeholders::_1), tcl_cast<T1>::from(interp, objv[2], byRef.value));
		} else {
			dispatch<R>::template do_dispatch<T1>(interp, std::bind(f_, p, std::placeholders::_1), tcl_cast<T1>::from(interp, objv[2], byRef.value));
		}
	}

  private:
	mem_type f_;
	cmem_type cf_;
	bool cmem_;
};

template <class C, typename R, typename T1, typename T2> class method2 : public object_cmd_base {
	typedef R (C::*mem_type)(T1, T2);
	typedef R (C::*cmem_type)(T1, T2) const;

  public:
	method2(mem_type f) : f_(f), cmem_(false) {}
	method2(cmem_type f) : cf_(f), cmem_(true) {}

	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) {
		check_params_no(objc, 4, pol.usage_);
		tcl_cast_by_reference<T1> byRef1;
		tcl_cast_by_reference<T2> byRef2;

		C *p = static_cast<C *>(pv);
		if (cmem_) {
			dispatch<R>::template do_dispatch<T1, T2>(interp, std::bind(cf_, p, std::placeholders::_1, std::placeholders::_2), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value));
		} else {
			dispatch<R>::template do_dispatch<T1, T2>(interp, std::bind(f_, p, std::placeholders::_1, std::placeholders::_2), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value));
		}
	}

  private:
	mem_type f_;
	cmem_type cf_;
	bool cmem_;
};

template <class C, typename R, typename T1, typename T2, typename T3> class method3 : public object_cmd_base {
	typedef R (C::*mem_type)(T1, T2, T3);
	typedef R (C::*cmem_type)(T1, T2, T3) const;

  public:
	method3(mem_type f) : f_(f), cmem_(false) {}
	method3(cmem_type f) : cf_(f), cmem_(true) {}

	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) {
		check_params_no(objc, 5, pol.usage_);
		tcl_cast_by_reference<T1> byRef1;
		tcl_cast_by_reference<T2> byRef2;
		tcl_cast_by_reference<T3> byRef3;

		C *p = static_cast<C *>(pv);
		if (cmem_) {
			dispatch<R>::template do_dispatch<T1, T2, T3>(interp, std::bind(cf_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value));
		} else {
			dispatch<R>::template do_dispatch<T1, T2, T3>(interp, std::bind(f_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value));
		}
	}

  private:
	mem_type f_;
	cmem_type cf_;
	bool cmem_;
};

template <class C, typename R, typename T1, typename T2, typename T3, typename T4> class method4 : public object_cmd_base {
	typedef R (C::*mem_type)(T1, T2, T3, T4);
	typedef R (C::*cmem_type)(T1, T2, T3, T4) const;

  public:
	method4(mem_type f) : f_(f), cmem_(false) {}
	method4(cmem_type f) : cf_(f), cmem_(true) {}

	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) {
		check_params_no(objc, 6, pol.usage_);
		tcl_cast_by_reference<T1> byRef1;
		tcl_cast_by_reference<T2> byRef2;
		tcl_cast_by_reference<T3> byRef3;
		tcl_cast_by_reference<T4> byRef4;

		C *p = static_cast<C *>(pv);
		if (cmem_) {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4>(interp, std::bind(cf_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value));
		} else {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4>(interp, std::bind(f_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value));
		}
	}

  private:
	mem_type f_;
	cmem_type cf_;
	bool cmem_;
};

template <class C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5> class method5 : public object_cmd_base {
	typedef R (C::*mem_type)(T1, T2, T3, T4, T5);
	typedef R (C::*cmem_type)(T1, T2, T3, T4, T5) const;

  public:
	method5(mem_type f) : f_(f), cmem_(false) {}
	method5(cmem_type f) : cf_(f), cmem_(true) {}

	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) {
		check_params_no(objc, 7, pol.usage_);
		tcl_cast_by_reference<T1> byRef1;
		tcl_cast_by_reference<T2> byRef2;
		tcl_cast_by_reference<T3> byRef3;
		tcl_cast_by_reference<T4> byRef4;
		tcl_cast_by_reference<T5> byRef5;

		C *p = static_cast<C *>(pv);
		if (cmem_) {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4, T5>(interp, std::bind(cf_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value), tcl_cast<T5>::from(interp, objv[6], byRef5.value));
		} else {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4, T5>(interp, std::bind(f_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value), tcl_cast<T5>::from(interp, objv[6], byRef5.value));
		}
	}

  private:
	mem_type f_;
	cmem_type cf_;
	bool cmem_;
};

template <class C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> class method6 : public object_cmd_base {
	typedef R (C::*mem_type)(T1, T2, T3, T4, T5, T6);
	typedef R (C::*cmem_type)(T1, T2, T3, T4, T5, T6) const;

  public:
	method6(mem_type f) : f_(f), cmem_(false) {}
	method6(cmem_type f) : cf_(f), cmem_(true) {}

	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) {
		check_params_no(objc, 8, pol.usage_);
		tcl_cast_by_reference<T1> byRef1;
		tcl_cast_by_reference<T2> byRef2;
		tcl_cast_by_reference<T3> byRef3;
		tcl_cast_by_reference<T4> byRef4;
		tcl_cast_by_reference<T5> byRef5;
		tcl_cast_by_reference<T6> byRef6;

		C *p = static_cast<C *>(pv);
		if (cmem_) {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4, T5, T6>(interp, std::bind(cf_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value), tcl_cast<T5>::from(interp, objv[6], byRef5.value), tcl_cast<T6>::from(interp, objv[7], byRef6.value));
		} else {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4, T5, T6>(interp, std::bind(f_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value), tcl_cast<T5>::from(interp, objv[6], byRef5.value), tcl_cast<T6>::from(interp, objv[7], byRef6.value));
		}
	}

  private:
	mem_type f_;
	cmem_type cf_;
	bool cmem_;
};

template <class C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> class method7 : public object_cmd_base {
	typedef R (C::*mem_type)(T1, T2, T3, T4, T5, T6, T7);
	typedef R (C::*cmem_type)(T1, T2, T3, T4, T5, T6, T7) const;

  public:
	method7(mem_type f) : f_(f), cmem_(false) {}
	method7(cmem_type f) : cf_(f), cmem_(true) {}

	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) {
		check_params_no(objc, 9, pol.usage_);
		tcl_cast_by_reference<T1> byRef1;
		tcl_cast_by_reference<T2> byRef2;
		tcl_cast_by_reference<T3> byRef3;
		tcl_cast_by_reference<T4> byRef4;
		tcl_cast_by_reference<T5> byRef5;
		tcl_cast_by_reference<T6> byRef6;
		tcl_cast_by_reference<T7> byRef7;

		C *p = static_cast<C *>(pv);
		if (cmem_) {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4, T5, T6, T7>(interp, std::bind(cf_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value), tcl_cast<T5>::from(interp, objv[6], byRef5.value), tcl_cast<T6>::from(interp, objv[7], byRef6.value), tcl_cast<T7>::from(interp, objv[8], byRef7.value));
		} else {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4, T5, T6, T7>(interp, std::bind(f_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value), tcl_cast<T5>::from(interp, objv[6], byRef5.value), tcl_cast<T6>::from(interp, objv[7], byRef6.value), tcl_cast<T7>::from(interp, objv[8], byRef7.value));
		}
	}

  private:
	mem_type f_;
	cmem_type cf_;
	bool cmem_;
};

template <class C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> class method8 : public object_cmd_base {
	typedef R (C::*mem_type)(T1, T2, T3, T4, T5, T6, T7, T8);
	typedef R (C::*cmem_type)(T1, T2, T3, T4, T5, T6, T7, T8) const;

  public:
	method8(mem_type f) : f_(f), cmem_(false) {}
	method8(cmem_type f) : cf_(f), cmem_(true) {}

	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) {
		check_params_no(objc, 10, pol.usage_);
		tcl_cast_by_reference<T1> byRef1;
		tcl_cast_by_reference<T2> byRef2;
		tcl_cast_by_reference<T3> byRef3;
		tcl_cast_by_reference<T4> byRef4;
		tcl_cast_by_reference<T5> byRef5;
		tcl_cast_by_reference<T6> byRef6;
		tcl_cast_by_reference<T7> byRef7;
		tcl_cast_by_reference<T8> byRef8;

		C *p = static_cast<C *>(pv);
		if (cmem_) {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4, T5, T6, T7, T8>(interp, std::bind(cf_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value), tcl_cast<T5>::from(interp, objv[6], byRef5.value), tcl_cast<T6>::from(interp, objv[7], byRef6.value), tcl_cast<T7>::from(interp, objv[8], byRef7.value), tcl_cast<T8>::from(interp, objv[9], byRef8.value));
		} else {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4, T5, T6, T7, T8>(interp, std::bind(f_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value), tcl_cast<T5>::from(interp, objv[6], byRef5.value), tcl_cast<T6>::from(interp, objv[7], byRef6.value), tcl_cast<T7>::from(interp, objv[8], byRef7.value), tcl_cast<T8>::from(interp, objv[9], byRef8.value));
		}
	}

  private:
	mem_type f_;
	cmem_type cf_;
	bool cmem_;
};

template <class C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9> class method9 : public object_cmd_base {
	typedef R (C::*mem_type)(T1, T2, T3, T4, T5, T6, T7, T8, T9);
	typedef R (C::*cmem_type)(T1, T2, T3, T4, T5, T6, T7, T8, T9) const;

  public:
	method9(mem_type f) : f_(f), cmem_(false) {}
	method9(cmem_type f) : cf_(f), cmem_(true) {}

	virtual void invoke(void *pv, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], policies const &pol) {
		check_params_no(objc, 11, pol.usage_);
		tcl_cast_by_reference<T1> byRef1;
		tcl_cast_by_reference<T2> byRef2;
		tcl_cast_by_reference<T3> byRef3;
		tcl_cast_by_reference<T4> byRef4;
		tcl_cast_by_reference<T5> byRef5;
		tcl_cast_by_reference<T6> byRef6;
		tcl_cast_by_reference<T7> byRef7;
		tcl_cast_by_reference<T8> byRef8;
		tcl_cast_by_reference<T9> byRef9;

		C *p = static_cast<C *>(pv);
		if (cmem_) {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4, T5, T6, T7, T8, T9>(interp, std::bind(cf_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8, std::placeholders::_9), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value), tcl_cast<T5>::from(interp, objv[6], byRef5.value), tcl_cast<T6>::from(interp, objv[7], byRef6.value), tcl_cast<T7>::from(interp, objv[8], byRef7.value), tcl_cast<T8>::from(interp, objv[9], byRef8.value), tcl_cast<T9>::from(interp, objv[10], byRef9.value));
		} else {
			dispatch<R>::template do_dispatch<T1, T2, T3, T4, T5, T6, T7, T8, T9>(interp, std::bind(f_, p, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8, std::placeholders::_9), tcl_cast<T1>::from(interp, objv[2], byRef1.value), tcl_cast<T2>::from(interp, objv[3], byRef2.value), tcl_cast<T3>::from(interp, objv[4], byRef3.value), tcl_cast<T4>::from(interp, objv[5], byRef4.value), tcl_cast<T5>::from(interp, objv[6], byRef5.value), tcl_cast<T6>::from(interp, objv[7], byRef6.value), tcl_cast<T7>::from(interp, objv[8], byRef7.value), tcl_cast<T8>::from(interp, objv[9], byRef8.value), tcl_cast<T9>::from(interp, objv[10], byRef9.value));
		}
	}

  private:
	mem_type f_;
	cmem_type cf_;
	bool cmem_;
};
