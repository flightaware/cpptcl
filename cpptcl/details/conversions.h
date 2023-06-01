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

// helper functor for converting Tcl objects to the given type
// (it is a struct instead of function,
// because I need to partially specialize it)
namespace Tcl { namespace details {

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

}

}
