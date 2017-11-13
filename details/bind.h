template <typename R, typename T1, typename T2 = void, typename T3 = void, typename T4 = void, typename T5 = void, typename T6 = void, typename T7 = void, typename T8 = void, typename T9 = void> struct Bind {
  private:
	object cmd_;

  public:
	Bind(std::string cmd) : cmd_(object(cmd)){};

	R operator()(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4, const T5 &t5, const T6 &t6, const T7 &t7, const T8 &t8, const T9 &t9) {
		object obj(cmd_);
		object e(t1);
		e.append(object(t2));
		e.append(object(t3));
		e.append(object(t4));
		e.append(object(t5));
		e.append(object(t6));
		e.append(object(t7));
		e.append(object(t8));
		e.append(object(t9));
		obj.append_list(e);
		return (R)(interpreter::getDefault()->eval(obj));
	}
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> struct Bind<R, T1, T2, T3, T4, T5, T6, T7, T8, void> {
  private:
	object cmd_;

  public:
	Bind(std::string cmd) : cmd_(object(cmd)){};

	R operator()(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4, const T5 &t5, const T6 &t6, const T7 &t7, const T8 &t8) {
		object obj(cmd_);
		object e(t1);
		e.append(object(t2));
		e.append(object(t3));
		e.append(object(t4));
		e.append(object(t5));
		e.append(object(t6));
		e.append(object(t7));
		e.append(object(t8));
		obj.append_list(e);
		return (R)(interpreter::getDefault()->eval(obj));
	}
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> struct Bind<R, T1, T2, T3, T4, T5, T6, T7, void, void> {
  private:
	object cmd_;

  public:
	Bind(std::string cmd) : cmd_(object(cmd)){};

	R operator()(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4, const T5 &t5, const T6 &t6, const T7 &t7) {
		object obj(cmd_);
		object e(t1);
		e.append(object(t2));
		e.append(object(t3));
		e.append(object(t4));
		e.append(object(t5));
		e.append(object(t6));
		e.append(object(t7));
		obj.append_list(e);
		return (R)(interpreter::getDefault()->eval(obj));
	}
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> struct Bind<R, T1, T2, T3, T4, T5, T6, void, void, void> {
  private:
	object cmd_;

  public:
	Bind(std::string cmd) : cmd_(object(cmd)){};

	R operator()(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4, const T5 &t5, const T6 &t6) {
		object obj(cmd_);
		object e(t1);
		e.append(object(t2));
		e.append(object(t3));
		e.append(object(t4));
		e.append(object(t5));
		e.append(object(t6));
		obj.append_list(e);
		return (R)(interpreter::getDefault()->eval(obj));
	}
};

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5> struct Bind<R, T1, T2, T3, T4, T5, void, void, void, void> {
  private:
	object cmd_;

  public:
	Bind(std::string cmd) : cmd_(object(cmd)){};

	R operator()(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4, const T5 &t5) {
		object obj(cmd_);
		object e(t1);
		e.append(object(t2));
		e.append(object(t3));
		e.append(object(t4));
		e.append(object(t5));
		obj.append_list(e);
		return (R)(interpreter::getDefault()->eval(obj));
	}
};

template <typename R, typename T1, typename T2, typename T3, typename T4> struct Bind<R, T1, T2, T3, T4, void, void, void, void, void> {
  private:
	object cmd_;

  public:
	Bind(std::string cmd) : cmd_(object(cmd)){};

	R operator()(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4) {
		object obj(cmd_);
		object e(t1);
		e.append(object(t2));
		e.append(object(t3));
		e.append(object(t4));
		obj.append_list(e);
		return (R)(interpreter::getDefault()->eval(obj));
	}
};

template <typename R, typename T1, typename T2, typename T3> struct Bind<R, T1, T2, T3, void, void, void, void, void, void> {
  private:
	object cmd_;

  public:
	Bind(std::string cmd) : cmd_(object(cmd)){};

	R operator()(const T1 &t1, const T2 &t2, const T3 &t3) {
		object obj(cmd_);
		object e(t1);
		e.append(object(t2));
		e.append(object(t3));
		obj.append_list(e);
		return (R)(interpreter::getDefault()->eval(obj));
	}
};

template <typename R, typename T1, typename T2> struct Bind<R, T1, T2, void, void, void, void, void, void, void> {
  private:
	object cmd_;

  public:
	Bind(std::string cmd) : cmd_(object(cmd)){};

	R operator()(const T1 &t1, const T2 &t2) {
		object obj(cmd_);
		object e(t1);
		e.append(object(t2));
		obj.append_list(e);
		return (R)(interpreter::getDefault()->eval(obj));
	}
};

template <typename R, typename T1> struct Bind<R, T1, void, void, void, void, void, void, void, void> {
  private:
	object cmd_;

  public:
	Bind(std::string cmd) : cmd_(object(cmd)){};

	R operator()(const T1 &t1) {
		object obj(cmd_);
		object e(t1);
		obj.append_list(e);
		return (R)(interpreter::getDefault()->eval(obj));
	}
};

template <typename R> struct Bind<R, void, void, void, void, void, void, void, void, void> {
  private:
	object cmd_;

  public:
	Bind(std::string cmd) : cmd_(object(cmd)){};

	R operator()() {
		object obj(cmd_);
		return (R)(interpreter::getDefault()->eval(obj));
	}
};
