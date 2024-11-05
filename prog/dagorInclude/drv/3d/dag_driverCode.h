//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class DriverCode;

/**
 * This namespace holds the matcher types and operations.
 */
namespace d3d::drivercode::matcher
{
template <bool B, typename T = void>
struct EnableIf
{};
template <typename T>
struct EnableIf<true, T>
{
  using Type = T;
};
template <typename A, typename B>
struct SameAs
{
  static constexpr bool value = false;
};
template <typename T>
struct SameAs<T, T>
{
  static constexpr bool value = true;
};
//! Combines two matchers with a binary operator.
//! \tparam T0 Type of the first matcher.
//! \tparam T1 Type of the second matcher.
//! \tparam Operator The binary operator that should combine the two matchers.
template <typename T0, typename T1, typename Operator>
struct BinaryOp;
//! A binary operator that implements the logical and operator to combine the result of two matchers.
struct OpAnd
{
  //! Combines two matcher results with a logical and.
  //! \param a Result of the first matcher.
  //! \param b Result of the second matcher.
  //! \returns The logical and of both \a and \b.
  static constexpr bool invoke(bool a, bool b) { return a && b; }
};
//! A binary operator that implements the logical or operator to combine the result of two matchers.
struct OpOr
{
  //! Combines two matcher results with a logical or.
  //! \param a Result of the first matcher.
  //! \param b Result of the second matcher.
  //! \returns The logical or of both \a and \b.
  static constexpr bool invoke(bool a, bool b) { return a || b; }
};
//! This matcher inverts the input matcher of template parameter \p T.
//! \tparam T is the matcher type that should be inverted.
template <typename T>
struct Inverted
{
  //! State of the base type.
  T base;
  //! Shorthand for this type.
  using ThisType = Inverted<T>;
  //! Invers the result of the matcher \p T.
  //! \param value Is the driver code value that should be matched by the matcher.
  //! \returns The inverted result of the matcher of type \p T.
  constexpr bool is(int value) { return !base.is(value); }
  //! Creates the inverted matcher of this, which is just \p T.
  constexpr T operator!() const { return base; }
};
//! Combines two matchers with a binary operation.
//! \tparam T0 Type of the first operand.
//! \tparam T1 Type of the second operand.
//! \tparam Operator Type of the binary operator.
template <typename T0, typename T1, typename Operator>
struct BinaryOp
{
  //! State of the first matcher.
  T0 first;
  //! State of the second matcher.
  T1 second;
  //! Shorthand for this type.
  using ThisType = BinaryOp<T0, T1, Operator>;
  //! Executes matchers for both \p T1 and \p T2 and combines the result with the operator \p Operator.
  //! \param value Is the driver code value that should be matched by the matchers.
  //! \returns Combined result of the matchers \p T1 and \p T2, combined by \p Operator.
  constexpr bool is(int value) { return Operator::invoke(first.is(value), second.is(value)); }
  //! Creates the inverted matcher of this matcher.
  //! \returns Inverted matcher of this matcher.
  constexpr Inverted<ThisType> operator!() const { return {}; }
};
//! Helper to generate the IDs correctly so they can be used as four character codes.
//! This helper is needed as _MAKE4C macro may not be available here.
//! \param value 4 character code as integer.
//! \retruns Platform specific encoded 4 character code as integer.
inline constexpr int make_foucc(int value)
{
#if _TARGET_CPU_BE
  return value;
#else
  return ((value >> 24) & 0xFFu) | ((value & 0xFF0000u) >> 8) | ((value & 0x00FF00u) << 8) | ((value & 0xFFu) << 24);
#endif
}
//! Matcher that matches a given ID. This matcher can be used by \ref DriverCode::make.
//! \tparam The ID value the matcher should match to true.
template <int Ident>
struct ID
{
  //! Shorthand for this type.
  using ThisType = ID<Ident>;
  //! The value of \p Ident. This value is used by \ref DriverCode::make to initialize the driver code state.
  static constexpr int value = make_foucc(Ident);
  //! Matches true when the driver code values is equal to \p Ident
  //! \param cmp Is the driver code state that should be compared to \p Ident.
  //! \returns Returns true when the driver code state is equal to \p Ident.
  static constexpr bool is(int cmp) { return value == cmp; }
  //! Creates the inverted matcher of this matcher.
  //! \returns Inverted matcher of this matcher.
  constexpr Inverted<ThisType> operator!() const { return {*this}; }
};
//! A constant matcher, which will always yield the value of \p Constant.
//! \tparam Constant The value the matcher should yield during matching.
template <bool Constant>
struct Const
{
  //! Matches always \p Constant.
  //! \returns Always \p Constant.
  static constexpr bool is(int) { return Constant; }
  //! Creates the inverted matcher of this matcher, will always yield the opposite of \p Constant.
  //! \returns Inverted matcher of this matcher.
  constexpr Const<!Constant> operator!() const { return {}; }
};
//! A matcher with a matching id, but matching will always yield the value of \p Constant.
//! \tparam Ident The ID the matcher is associated with.
//! \tparam Constant The value the matcher should yield during matching.
template <int Ident, bool Constant>
struct ConstID : Const<Constant>
{
  using ThisType = ConstID<Ident, Constant>;
  //! The value of \p Ident. This value is used by \ref DriverCode::make to initialize the driver code state.
  static constexpr int value = make_foucc(Ident);
  //! Creates the inverted matcher of this matcher, will always yield false.
  //! \returns A const false matcher.
  constexpr Const<!Constant> operator!() const { return {}; }
};
/**
 * \brief The any matcher behaves like a constant true matcher, matching always to true.
 * \details Any has some special meaning in some context (like map), where it result in
 * different type / code-gen.
 */
struct Any : Const<true>
{
  //! Disallow operations with any, any should only be used on its own.
  constexpr Inverted<Any> operator!() const = delete;
};
/**
 * \brief This matcher type is just to result in compilation errors on not yet supported platforms and help doxygen.
 * \details So when the compiler is yelling at you about this type, then you may want to add a new set of matchers for
 * that \_TARGET\_* you want to build.
 * Use a existing target as a template and alter they types of the matchers to model the behavior of that target.
 */
struct Unsupported
{
  //! No one should be able to instance this type, it is supposed to fail the build.
  Unsupported() = delete;
};
//! Logical and operator, allows combining multiple matchers.
//! \param l First matcher to be combined.
//! \param r Second matcher to be combined.
//! \returns A new BinaryOp type of this type and \p T1 with an operator OpAnd.
template <typename T, typename T1>
constexpr auto operator&&(const Inverted<T> &l, const T1 &r)
{
  return BinaryOp<Inverted<T>, T1, OpAnd>{l, r};
}
//! Logical or operator, allows combining multiple matchers.
//! \param l First matcher to be combined.
//! \param r Second matcher to be combined.
//! \returns A new BinaryOp type of this type and \p T1 with an operator OpOr.
template <typename T, typename T1>
constexpr auto operator||(const Inverted<T> &l, const T1 &r)
{
  return BinaryOp<Inverted<T>, T1, OpOr>{l, r};
}
//! Logical and operator, allows combining multiple matchers.
//! \param l First matcher to be combined.
//! \param r Second matcher to be combined.
//! \returns A new BinaryOp type of this type and \p T1 with an operator OpAnd.
template <typename T0, typename T1, typename Operator, typename TO>
constexpr auto operator&&(const BinaryOp<T0, T1, Operator> &l, const TO &r)
{
  return BinaryOp<BinaryOp<T0, T1, Operator>, TO, OpAnd>{l, r};
}
//! Logical or operator, allows combining multiple matchers.
//! \param l First matcher to be combined.
//! \param r Second matcher to be combined.
//! \returns A new BinaryOp type of this type and \p T1 with an operator OpOr.
template <typename T0, typename T1, typename Operator, typename TO>
constexpr auto operator||(const BinaryOp<T0, T1, Operator> &l, const TO &r)
{
  return BinaryOp<BinaryOp<T0, T1, Operator>, TO, OpOr>{l, r};
}
//! Logical and operator, allows combining multiple matchers.
//! \tparam T1 is the type of the second operand.
//! \returns A new BinaryOp type of this type and \p T1 with an operator OpAnd.
template <int Ident, typename T1>
constexpr auto operator&&(const ID<Ident> &l, const T1 &r)
{
  return BinaryOp<ID<Ident>, T1, OpAnd>{l, r};
}
//! Logical or operator, allows combining multiple matchers.
//! \tparam T1 is the type of the second operand.
//! \returns A new BinaryOp type of this type and \p T1 with an operator OpOr.
template <int Ident, typename T1>
constexpr auto operator||(const ID<Ident> &l, const T1 &r)
{
  return BinaryOp<ID<Ident>, T1, OpOr>{l, r};
}
//! Logical and operator, allows combining multiple matchers.
//! \param l First matcher to be combined.
//! \param r Second matcher to be combined.
//! \returns The matercher of \p r, as logical and with constant true can be short circuited to just evaluating the second parameter.
template <typename T1>
constexpr const T1 &operator&&(const Const<true> &l, const T1 &r)
{
  (void)l;
  return r;
}
//! Logical or operator, allows combining multiple matchers.
//! \param l First matcher to be combined.
//! \param r Second matcher to be combined.
//! \returns A true matcher, as logical or with a constant true can be short circuited to just the constant true.
template <typename T1>
constexpr const Const<true> &operator||(const Const<true> &l, const T1 &r)
{
  (void)r;
  return l;
}
//! Logical and operator, allows combining multiple matchers.
//! \param l First matcher to be combined.
//! \param r Second matcher to be combined.
//! \returns A constant false matcher, as logical and with a constant false can be short circuited to just the constant false.
template <typename T1>
constexpr const Const<false> &operator&&(const Const<false> &l, const T1 &r)
{
  (void)r;
  return l;
}
//! Logical or operator, allows combining multiple matchers.
//! \param l First matcher to be combined.
//! \param r Second matcher to be combined.
//! \returns The matercher of \p r, as logical or with constant false can be short circuited to just evaluating the second parameter.
template <typename T1>
constexpr const T1 &operator||(const Const<false> &l, const T1 &r)
{
  (void)l;
  return r;
}
//! Disallow operations with any, any should only be used on its own.
template <typename T1>
constexpr const T1 &operator&&(Any, T1) = delete;
//! Disallow operations with any, any should only be used on its own.
template <typename T1>
constexpr Any operator||(Any, T1) = delete; //! Disallow operations with any, any should only be used on its own.
template <typename T1>
constexpr const T1 &operator&&(T1, Any) = delete;
//! Disallow operations with any, any should only be used on its own.
template <typename T1>
constexpr Any operator||(T1, Any) = delete;

/**
 * This type allows easy mappings of driver matchers to a value of the given type \p T.
 * \details The map maps the first mapping, not the most accurate mapping. So order the mapping accordingly.
 * Mappings can either be values directly, emplacement constructor parameters or generators (lambda).
 * \tparam T Is the type of the value the map should store. Must be move or copyable and either be able to be constructed by copy,
 * move or emplace.
 */
template <typename T>
class Map
{
  //! Driver code value, stored as a int to break up dependency to the DriverCode type.
  int value;
  //! Keeps track if anything was matched.
  bool wasMatched = false;
  //! Raw bytes to store T
  alignas(alignof(T)) unsigned char memory[sizeof(T)]; // -V730
  /**
   * Helper to retrieve the stored instance of \p T.
   * \returns Reference to the store instance of \p T.
   */
  T &object() { return *(T *)&memory[0]; }
  /**
   * \brief Conditional move constructor helper.
   * \details A new instance of \p T is going to be instantiated when \p should_construct is true and the internal state indicates that
   * there is no instance stored yet. In this case \p v will be moved into the parameter of the move constructor of \p T for the
   * new instance.
   * \param should_construct Indicates if a instance of \p T should be instantiated or not.
   * \param v May be used as a input for the move constructor of \p T.
   */
  void cMove(bool should_construct, T &&v)
  {
    if (!should_construct)
      return;
    if (wasMatched)
      return;
    ::new (&object()) T{(T &&) v};
    wasMatched = true;
  }
  /**
   * \brief Conditional constructor helper.
   * \details A new instance of \p T is going to be instantiated when \p should_construct is true and the internal state indicates that
   * there is no instance stored yet. In this case \p generator will be invoked to generate the input of the constructor of \p T for
   * the new instance.
   * \param should_construct Indicates if a instance of \p T should be instantiated or not.
   * \param generator When invoked it should generate valid input for the constructor of \p T.
   */
  template <typename C>
  void cContruct(bool should_construct, C &&generator)
  {
    if (!should_construct)
      return;
    if (wasMatched)
      return;
    ::new (&object()) T{generator()};
    wasMatched = true;
  }
  /**
   * Conditional emplace constructor helper.
   * \details A new instance of \p T is going to be instantiated when \p should_construct is true and the internal state indicates that
   * there is no instance stored yet. In this case \p vs will be moved into the parameter of the constructor of \p T for the new
   * instance.
   * \param should_construct Indicates if a instance of \p T should be instantiated or not.
   * \param vs May be used as a input for the constructor of \p T.
   */
  template <typename... Vs>
  void cEmplace(bool should_construct, Vs &&...vs)
  {
    if (!should_construct)
      return;
    if (wasMatched)
      return;
    ::new (&object()) T{(Vs &&) vs...};
    wasMatched = true;
  }
  /**
   * Helper to shorten matcher invocation.
   * \param d Matcher to invoke with the driver code state value.
   * \returns The value returned by the matchers \p d \p is method.
   */
  template <typename D>
  constexpr bool is(D d) const
  {
    return d.is(value);
  }

protected:
  friend class ::DriverCode;
  /**
   * Create a new object with a stored driver code value.
   * \param v Driver code value that is used on the matchers.
   */
  Map(int v) : value{v} // -V730
  {}
  template <typename D>
  Map(int v, D d, T &&val) : value{v} // -V730
  {
    cMove(is(d), (T &&) val);
  }
  template <typename D>
  Map(int v, D d, const T &val) : value{v} // -V730
  {
    cEmplace(is(d), val);
  }
  template <typename D, typename C>
  Map(int v, D d, C &&callable) : value{v} // -V730
  {
    cContruct(is(d), (C &&) callable);
  }
  template <typename D, typename... Vs>
  Map(int v, D d, Vs &&...vs) : value{v} // -V730
  {
    cEmplace(is(d), (Vs &&) vs...);
  }

public:
  //! Default construction is not allowed.
  Map() = delete;
  //! Disallow copying.
  Map(const Map &) = delete;
  //! Move constructor, will move internally stored object when needed.
  Map(Map &&other) : value{other.value}, wasMatched{other.wasMatched} // -V730
  {
    if (wasMatched)
    {
      ::new (&object()) T{(T &&) other.object()};
    }
  }
  //! Disallow copy assignment.
  Map &operator=(const Map &) = delete;
  //! Disallow move assignment.
  Map &operator=(Map &&) = delete;
  //! Destructor will invoke the destructor of the stored object when any was constructed.
  ~Map()
  {
    if (wasMatched)
    {
      object().~T();
    }
  }
  /**
   * Maps a matcher to a value.
   * \param d The matcher that should match the driver code value against.
   * \param v The value that should be stored should the matcher indicate a match.
   * \returns A reference to this, so that multiple matches can be chained.
   */
  template <typename D>
  Map &operator()(D d, T &&v)
  {
    cMove(is(d), (T &&) v);
    return *this;
  }
  /**
   * Maps a matcher to a generator.
   * \param d The matcher that should match the driver code value against.
   * \param callable Invoked to generate a input parameter for the constructor of \p T in case when a new instance of \p T is
   * instantiated. \returns A reference to this, so that multiple matches can be chained.
   */
  template <typename D, typename C>
  typename EnableIf<!SameAs<T, C>::value, Map &>::Type operator()(D d, C &&callable)
  {
    cContruct(is(d), (C &&) callable);
    return *this;
  }
  /**
   * Maps a matcher to a set of parameters for the constructor of \p T.
   * \param d The matcher that should match the driver code value against.
   * \param vs Values passed to the constructor of \p T in case when a new instance of \p T is instantiated.
   * \returns A reference to this, so that multiple matches can be chained.
   */
  template <typename D, typename... Vs>
  Map &operator()(D d, Vs &&...vs)
  {
    cEmplace(is(d), (Vs &&) vs...);
    return *this;
  }
  /**
   * Maps the drivercode::matcher::Any matcher to a value.
   * \param v A alternative value that may be used when no value of \p T is stored in this map.
   * \returns Either the lvalue reference to the maps stored object or the parameter \p v.
   */
  T &&operator()(Any, T &&v)
  {
    if (wasMatched)
    {
      return (T &&) object();
    }
    return (T &&) v;
  }
  /**
   * Maps the drivercode::matcher::Any matcher to a value.
   * \param v A alternative value that may be used when no value of \p T is stored in this map.
   * \returns A lvalue reference to the maps stored object.
   */
  T &&operator()(Any, const T &v)
  {
    cEmplace(true, v);
    return (T &&) v;
  }
  /**
   * Maps the drivercode::matcher::Any matcher to a generator.
   * \param callable Invoked to generate a input parameter for the constructor of \p T in case when a new instance of \p T is
   * instantiated.
   * \returns A lvalue reference to the maps stored object.
   */
  template <typename C>
  typename EnableIf<!SameAs<T, C>::value, T &&>::Type operator()(Any, C &&callable)
  {
    cContruct(true, (C &&) callable);
    return (T &&) object();
  }
  /**
   * Maps the drivercode::matcher::Any matcher to a set of parameters for the constructor of \p T.
   * \param vs Values passed to the constructor of \p T in case when a new instance of \p T is instantiated.
   * \returns A lvalue reference to the maps stored object.
   */
  template <typename... Vs>
  T &&operator()(Any, Vs &&...vs)
  {
    cEmplace(true, (Vs &&) vs...);
    return (T &&) object();
  }
  /**
   * Maps a matcher to a value.
   * \param d The matcher that should match the driver code value against.
   * \param v The value that should be stored should the matcher indicate a match.
   * \returns A reference to this, so that multiple matches can be chained.
   */
  template <typename D>
  Map &map(D d, T &&v)
  {
    cMove(is(d), (T &&) v);
    return *this;
  }
  /**
   * Maps a matcher to a generator.
   * \param d The matcher that should match the driver code value against.
   * \param callable Invoked to generate a input parameter for the constructor of \p T in case when a new instance of \p T is
   * instantiated.
   * \returns A reference to this, so that multiple matches can be chained.
   */
  template <typename D, typename C>
  typename EnableIf<!SameAs<T, C>::value, Map &>::Type map(D d, C &&callable)
  {
    cContruct(is(d), (C &&) callable);
    return *this;
  }
  /**
   * Maps a matcher to a set of parameters for the constructor of \p T.
   * \param d The matcher that should match the driver code value against.
   * \param vs Values passed to the constructor of \p T in case when a new instance of \p T is instantiated.
   * \returns A reference to this, so that multiple matches can be chained.
   */
  template <typename D, typename... Vs>
  Map &map(D d, Vs &&...vs)
  {
    cEmplace(is(d), (Vs &&) vs...);
    return *this;
  }
  /**
   * Maps the drivercode::matcher::Any matcher to the driver code.
   * \param v A alternative value that may be used when no value of \p T is stored in this map.
   * \returns Either the lvalue reference to the maps stored object or the parameter \p v.
   */
  T &&map(Any, T &&v)
  {
    if (wasMatched)
    {
      return (T &&) object();
    }
    return (T &&) v;
  }
  /**
   * Maps the drivercode::matcher::Any matcher to a value.
   * \param v A alternative value that may be used when no value of \p T is stored in this map.
   * \returns A lvalue reference to the maps stored object.
   */
  T &&map(Any, const T &v)
  {
    cEmplace(true, v);
    return (T &&) v;
  }
  /**
   * Maps the drivercode::matcher::Any matcher to a generator.
   * \param callable Invoked to generate a input parameter for the constructor of \p T in case when a new instance of \p T is
   * instantiated.
   * \returns A lvalue reference to the maps stored object.
   */
  template <typename C>
  typename EnableIf<!SameAs<T, C>::value, T &&>::Type map(Any, C &&callable)
  {
    cContruct(true, (C &&) callable);
    return (T &&) object();
  }
  /**
   * Maps the drivercode::matcher::Any matcher to a set of parameters for the constructor of \p T.
   * \param vs Values passed to the constructor of \p T in case when a new instance of \p T is instantiated.
   * \returns A lvalue reference to the maps stored object.
   */
  template <typename... Vs>
  T &&map(matcher::Any, Vs &&...vs)
  {
    cEmplace(true, (Vs &&) vs...);
    return (T &&) object();
  }
  /**
   * When a value was generated by any match, \p receiver will be invoked and the value will be moved into its input parameter.
   * \param receiver A callable that will be invoked when a value was generated by any match, otherwise it will not be invoked.
   * \returns When \p receiver was invoked true, otherwise false.
   */
  template <typename R>
  bool get(R &&receiver)
  {
    if (wasMatched)
    {
      receiver((T &&) object());
      return true;
    }
    return false;
  }
  /**
   * When a value was generated by any match, this will move the value into \p target and return true. Otherwise nothing will be done
   * to \p target and false will be returned.
   * \param target When a value was generated by any match, the value will move the value into this parameter.
   *  \returns When a value was moved to \p target true, otherwise false.
   */
  template <typename A>
  bool get(A &target)
  {
    if (wasMatched)
    {
      target = (T &&) object();
      return true;
    }
    return false;
  }
  /**
   * When a value was generated by any match, \p receiver will be invoked and the value will be moved into its input parameter.
   * \param receiver A callable that will be invoked when a value was generated by any match, otherwise it will not be invoked.
   * \returns When \p receiver was invoked true, otherwise false.
   */
  template <typename R>
  bool operator>>(R &&receiver)
  {
    if (wasMatched)
    {
      receiver((T &&) object());
      return true;
    }
    return true;
  }
  /**
   * When a value was generated by any match, this will move the value into \p target and return true. Otherwise nothing will be done
   * to \p target and false will be returned.
   *  \param target When a value was generated by any match, the value will move the value into this parameter.
   * \returns When a value was moved to \p target true, otherwise false.
   */
  template <typename A>
  bool operator>>(A &target)
  {
    if (wasMatched)
    {
      target = (T &&) object();
      return true;
    }
    return false;
  }
  /**
   * When a value was generated by any match, \p receiver will be invoked and the value will be moved into its input parameter.
   * \param receiver A callable that will be invoked when a value was generated by any match, otherwise it will not be invoked.
   * \returns When \p receiver was invoked true, otherwise false.
   */
  template <typename R>
  bool operator()(R &&receiver)
  {
    return get<R>((R &&) receiver);
  }
  /**
   * When a value was generated by any match, this will move the value into \p target and return true. Otherwise nothing will be done
   * to \p target and false will be returned.
   * \param target When a value was generated by any match, the value will move the value into this parameter.
   * \returns When a value was moved to \p target true, otherwise false.
   */
  template <typename A>
  bool operator()(A &target)
  {
    return get(target);
  }
};
/**
 */
class FirstMatch
{
  //! Driver code value, stored as a int to break up dependency to the DriverCode type.
  int value;
  //! Keeps track if anything was matched.
  bool wasMatched = false;
  /**
   * Match execution handler, will update internal state and call \p callable when \p did_match is true and its internal state
   * indicated that no matcher did match in previous match attempts.
   * \param did_match Indicator if a match was found.
   * \param callable Is called when \p did_match is true and the internal state indicated that no matcher did match in previous match
   * attempts.
   */
  template <typename T>
  void onMatch(bool did_match, T &&callable)
  {
    if (!did_match)
      return;
    if (wasMatched)
      return;
    callable();
    wasMatched = true;
  }
  /**
   * Helper to shorten matcher invocation.
   * \param d Matcher to invoke with the driver code state value.
   * \returns The value returned by the matchers \p d \p is method.
   */
  template <typename D>
  constexpr bool is(D d) const
  {
    return d.is(value);
  }

protected:
  friend class ::DriverCode;
  /**
   * Create a new object with a stored driver code value.
   * \param v Driver code value that is used on the matchers.
   */
  FirstMatch(int v) : value{v} {}
  /**
   * Create a new object with a stored driver code value and a initial match call.
   * \param v Driver code value that is used on the matchers.
   * \param d The matcher that should be immediately be matched on \p v.
   * \param callable The callable that should be executed when the matcher \p d matches the driver code value \p v.
   */
  template <typename D, typename T>
  FirstMatch(int v, D d, T &&callable) : value{v}
  {
    onMatch(is(d), (T &&) callable);
  }

public:
  //! Default construction is not allowed.
  FirstMatch() = delete;
  //! Disallow copying.
  FirstMatch(const FirstMatch &) = delete;
  //! Allow moving.
  FirstMatch(FirstMatch &&other) = default;
  //! Disallow copy assignment.
  FirstMatch &operator=(const FirstMatch &) = delete;
  //! Disallow move assignment.
  FirstMatch &operator=(FirstMatch &&) = delete;
  //! \copydoc match
  template <typename D, typename T>
  FirstMatch &operator()(D d, T &&callable)
  {
    onMatch(is(d), (T &&) callable);
    return *this;
  }
  /**
   * Executes the matcher of \p d with the driver code state and if it yields a match and there was no previous match it will execute
   *the callable of \p callable.
   * \param d The matcher that should match the driver code value against.
   * \param callable The callable that is called should the matcher of \p d return true and no matcher was found in previous matches.
   * \returns A reference to this, so that multiple matches can be chained.
   */
  template <typename D, typename T>
  FirstMatch &match(D d, T &&callable)
  {
    onMatch(is(d), (T &&) callable);
    return *this;
  }
  //! Explicit bool operator allows easy checking if a matcher was matched and a callable was executed.
  //! \returns True when a matcher was matched and a callable was called.
  explicit operator bool() const { return wasMatched; }
  //! Not operator overload allows easy checking if no matcher was matched and no callable was executed.
  //! \returns True when no matcher was matched and no callable was called.
  bool operator!() const { return !wasMatched; }
};
/**
 */
class AllMatch
{
  //! Driver code value, stored as a int to break up dependency to the DriverCode type.
  int value;
  //! Keeps track if anything was matched.
  bool wasMatched = false;
  /**
   * Match execution handler, will update internal state and call \p callable when \p did_match is true.
   * \param did_match Indicator if a match was found, when true \p callable will be called and the internal state is updated to
   * indicate that there was a match.
   * \param callable Is called when \p did_match is true.
   */
  template <typename T>
  void onMatch(bool did_match, T &&callable)
  {
    if (!did_match)
      return;
    callable();
    wasMatched = true;
  }
  /**
   * Helper to shorten matcher invocation.
   * \param d Matcher to invoke with the driver code state value.
   * \returns The value returned by the matchers \p d \p is method.
   */
  template <typename D>
  constexpr bool is(D d) const
  {
    return d.is(value);
  }

protected:
  friend class ::DriverCode;
  /**
   * Create a new object with a stored driver code value.
   * \param v Driver code value that is used on the matchers.
   */
  AllMatch(int v) : value{v} {}
  /**
   * Create a new object with a stored driver code value and a initial match call.
   * \param v Driver code value that is used on the matchers.
   * \param d The matcher that should be immediately be matched on \p v.
   * \param callable The callable that should be executed when the matcher \p d matches the driver code value \p v.
   */
  template <typename D, typename T>
  AllMatch(int v, D d, T &&callable) : value{v}
  {
    onMatch(is(d), (T &&) callable);
  }

public:
  //! Default construction is not allowed.
  AllMatch() = delete;
  //! Disallow copying.
  AllMatch(const AllMatch &) = delete;
  //! Allow moving.
  AllMatch(AllMatch &&other) = default;
  //! Disallow copy assignment.
  AllMatch &operator=(const AllMatch &) = delete;
  //! Disallow move assignment.
  AllMatch &operator=(AllMatch &&) = delete;
  //! \copydoc match
  template <typename D, typename T>
  AllMatch &operator()(D d, T &&callable)
  {
    onMatch(is(d), (T &&) callable);
    return *this;
  }
  /**
   * Executes the matcher of \p d with the driver code state and if it yields a match it will execute the callable of \p callable.
   * \param d The matcher that should match the driver code value against.
   * \param callable The callable that is called should the matcher of \p d return true.
   * \returns A reference to this, so that multiple matches can be chained.
   **/
  template <typename D, typename T>
  AllMatch &match(D d, T &&callable)
  {
    onMatch(is(d), (T &&) callable);
    return *this;
  }
  //! Explicit bool operator allows easy checking if any matcher was matched and any callable was executed.
  //! \returns True when any matcher was matched and any callable was called.
  explicit operator bool() const { return wasMatched; }
  //! Not operator overload allows easy checking if no matcher was matched and no callable was executed.
  //! \returns True when no matcher was matched and no callable was called.
  bool operator!() const { return !wasMatched; }
};
} // namespace d3d::drivercode::matcher

#if _TARGET_XBOXONE
namespace d3d
{
inline constexpr drivercode::matcher::Const<true> xboxOne;
inline constexpr drivercode::matcher::Const<false> scarlett;
inline constexpr drivercode::matcher::Const<false> iOS;
inline constexpr drivercode::matcher::Const<false> tvOS;
inline constexpr drivercode::matcher::Const<false> nintendoSwitch;
inline constexpr drivercode::matcher::Const<false> android;
inline constexpr drivercode::matcher::Const<false> macOSX;
inline constexpr drivercode::matcher::Const<false> linux;
inline constexpr drivercode::matcher::Const<false> windows;
inline constexpr drivercode::matcher::Const<false> dx11;
inline constexpr drivercode::matcher::ConstID<'DX12', true> dx12;
inline constexpr drivercode::matcher::Const<false> vulkan;
inline constexpr drivercode::matcher::Const<false> ps4;
inline constexpr drivercode::matcher::Const<false> ps5;
inline constexpr drivercode::matcher::Const<false> metal;
inline constexpr drivercode::matcher::Const<false> null;
inline constexpr drivercode::matcher::Const<false> stub;
} // namespace d3d
#elif _TARGET_SCARLETT
namespace d3d
{
inline constexpr drivercode::matcher::Const<false> xboxOne;
inline constexpr drivercode::matcher::Const<true> scarlett;
inline constexpr drivercode::matcher::Const<false> iOS;
inline constexpr drivercode::matcher::Const<false> tvOS;
inline constexpr drivercode::matcher::Const<false> nintendoSwitch;
inline constexpr drivercode::matcher::Const<false> android;
inline constexpr drivercode::matcher::Const<false> macOSX;
inline constexpr drivercode::matcher::Const<false> linux;
inline constexpr drivercode::matcher::Const<false> windows;
inline constexpr drivercode::matcher::Const<false> dx11;
inline constexpr drivercode::matcher::ConstID<'DX12', true> dx12;
inline constexpr drivercode::matcher::Const<false> vulkan;
inline constexpr drivercode::matcher::Const<false> ps4;
inline constexpr drivercode::matcher::Const<false> ps5;
inline constexpr drivercode::matcher::Const<false> metal;
inline constexpr drivercode::matcher::Const<false> null;
inline constexpr drivercode::matcher::Const<false> stub;
} // namespace d3d
#elif _TARGET_C1




















#elif _TARGET_C2




















#elif _TARGET_IOS
namespace d3d
{
inline constexpr drivercode::matcher::Const<false> xboxOne;
inline constexpr drivercode::matcher::Const<false> scarlett;
inline constexpr drivercode::matcher::Const<true> iOS;
inline constexpr drivercode::matcher::Const<false> tvOS;
inline constexpr drivercode::matcher::Const<false> nintendoSwitch;
inline constexpr drivercode::matcher::Const<false> android;
inline constexpr drivercode::matcher::Const<false> macOSX;
inline constexpr drivercode::matcher::Const<false> linux;
inline constexpr drivercode::matcher::Const<false> windows;
inline constexpr drivercode::matcher::Const<false> dx11;
inline constexpr drivercode::matcher::Const<false> dx12;
inline constexpr drivercode::matcher::Const<false> vulkan;
inline constexpr drivercode::matcher::Const<false> ps4;
inline constexpr drivercode::matcher::Const<false> ps5;
inline constexpr drivercode::matcher::ConstID<'MTL', true> metal;
inline constexpr drivercode::matcher::Const<false> null;
inline constexpr drivercode::matcher::Const<false> stub;
} // namespace d3d
#elif _TARGET_TVOS
namespace d3d
{
inline constexpr drivercode::matcher::Const<false> xboxOne;
inline constexpr drivercode::matcher::Const<false> scarlett;
inline constexpr drivercode::matcher::Const<false> iOS;
inline constexpr drivercode::matcher::Const<true> tvOS;
inline constexpr drivercode::matcher::Const<false> nintendoSwitch;
inline constexpr drivercode::matcher::Const<false> android;
inline constexpr drivercode::matcher::Const<false> macOSX;
inline constexpr drivercode::matcher::Const<false> linux;
inline constexpr drivercode::matcher::Const<false> windows;
inline constexpr drivercode::matcher::Const<false> dx11;
inline constexpr drivercode::matcher::Const<false> dx12;
inline constexpr drivercode::matcher::Const<false> vulkan;
inline constexpr drivercode::matcher::Const<false> ps4;
inline constexpr drivercode::matcher::Const<false> ps5;
inline constexpr drivercode::matcher::ConstID<'MTL', true> metal;
inline constexpr drivercode::matcher::Const<false> null;
inline constexpr drivercode::matcher::Const<false> stub;
} // namespace d3d
#elif _TARGET_C3




















#elif _TARGET_ANDROID
namespace d3d
{
inline constexpr drivercode::matcher::Const<false> xboxOne;
inline constexpr drivercode::matcher::Const<false> scarlett;
inline constexpr drivercode::matcher::Const<false> iOS;
inline constexpr drivercode::matcher::Const<false> tvOS;
inline constexpr drivercode::matcher::Const<false> nintendoSwitch;
inline constexpr drivercode::matcher::Const<true> android;
inline constexpr drivercode::matcher::Const<false> macOSX;
inline constexpr drivercode::matcher::Const<false> linux;
inline constexpr drivercode::matcher::Const<false> windows;
inline constexpr drivercode::matcher::Const<false> dx11;
inline constexpr drivercode::matcher::Const<false> dx12;
inline constexpr drivercode::matcher::ConstID<'VULK', true> vulkan;
inline constexpr drivercode::matcher::Const<false> ps4;
inline constexpr drivercode::matcher::Const<false> ps5;
inline constexpr drivercode::matcher::Const<false> metal;
inline constexpr drivercode::matcher::Const<false> null;
inline constexpr drivercode::matcher::Const<false> stub;
} // namespace d3d
#elif _TARGET_PC_MACOSX
namespace d3d
{
inline constexpr drivercode::matcher::Const<false> xboxOne;
inline constexpr drivercode::matcher::Const<false> scarlett;
inline constexpr drivercode::matcher::Const<false> iOS;
inline constexpr drivercode::matcher::Const<false> tvOS;
inline constexpr drivercode::matcher::Const<false> nintendoSwitch;
inline constexpr drivercode::matcher::Const<false> android;
inline constexpr drivercode::matcher::Const<true> macOSX;
inline constexpr drivercode::matcher::Const<false> linux;
inline constexpr drivercode::matcher::Const<false> windows;
inline constexpr drivercode::matcher::ConstID<'DX11', false> dx11;
inline constexpr drivercode::matcher::ConstID<'DX12', false> dx12;
inline constexpr drivercode::matcher::ID<'VULK'> vulkan;
inline constexpr drivercode::matcher::Const<false> ps4;
inline constexpr drivercode::matcher::Const<false> ps5;
inline constexpr drivercode::matcher::ID<'MTL'> metal;
inline constexpr drivercode::matcher::ID<0> null;
inline constexpr drivercode::matcher::ID<'STUB'> stub;
} // namespace d3d
#elif _TARGET_PC_LINUX
namespace d3d
{
inline constexpr drivercode::matcher::Const<false> xboxOne;
inline constexpr drivercode::matcher::Const<false> scarlett;
inline constexpr drivercode::matcher::Const<false> iOS;
inline constexpr drivercode::matcher::Const<false> tvOS;
inline constexpr drivercode::matcher::Const<false> nintendoSwitch;
inline constexpr drivercode::matcher::Const<false> android;
inline constexpr drivercode::matcher::Const<false> macOSX;
inline constexpr drivercode::matcher::Const<true> linux;
inline constexpr drivercode::matcher::Const<false> windows;
inline constexpr drivercode::matcher::ConstID<'DX11', false> dx11;
inline constexpr drivercode::matcher::ConstID<'DX12', false> dx12;
inline constexpr drivercode::matcher::ID<'VULK'> vulkan;
inline constexpr drivercode::matcher::Const<false> ps4;
inline constexpr drivercode::matcher::Const<false> ps5;
inline constexpr drivercode::matcher::ConstID<'MTL', false> metal;
inline constexpr drivercode::matcher::ID<0> null;
inline constexpr drivercode::matcher::ID<'STUB'> stub;
} // namespace d3d
#elif _TARGET_PC_WIN
namespace d3d
{
inline constexpr drivercode::matcher::Const<false> xboxOne;
inline constexpr drivercode::matcher::Const<false> scarlett;
inline constexpr drivercode::matcher::Const<false> iOS;
inline constexpr drivercode::matcher::Const<false> tvOS;
inline constexpr drivercode::matcher::Const<false> nintendoSwitch;
inline constexpr drivercode::matcher::Const<false> android;
inline constexpr drivercode::matcher::Const<false> macOSX;
inline constexpr drivercode::matcher::Const<false> linux;
inline constexpr drivercode::matcher::Const<true> windows;
inline constexpr drivercode::matcher::ID<'DX11'> dx11;
inline constexpr drivercode::matcher::ID<'DX12'> dx12;
inline constexpr drivercode::matcher::ID<'VULK'> vulkan;
inline constexpr drivercode::matcher::Const<false> ps4;
inline constexpr drivercode::matcher::Const<false> ps5;
inline constexpr drivercode::matcher::ConstID<'MTL', false> metal;
inline constexpr drivercode::matcher::ID<0> null;
inline constexpr drivercode::matcher::ID<'STUB'> stub;
} // namespace d3d
#else
namespace d3d
{
//! Matches to true when the current platform is \xbone and otherwise false
inline constexpr drivercode::matcher::Unsupported xboxOne;
//! Matches to true when the current platform is \scarlett and otherwise false
inline constexpr drivercode::matcher::Unsupported scarlett;
//! Matches to true when the current platform is \ios and otherwise false
inline constexpr drivercode::matcher::Unsupported iOS;
//! Matches to true when the current platform is \tvos and otherwise false
inline constexpr drivercode::matcher::Unsupported tvOS;
//! Matches to true when the current platform is \nswitch and otherwise false
inline constexpr drivercode::matcher::Unsupported nintendoSwitch;
//! Matches to true when the current platform is \android and otherwise false
inline constexpr drivercode::matcher::Unsupported android;
//! Matches to true when the current platform is \mac and otherwise false
inline constexpr drivercode::matcher::Unsupported macOSX;
//! Matches to true when the current platform is \linux and otherwise false
inline constexpr drivercode::matcher::Unsupported linux;
//! Matches to true when the current platform is \win32 and otherwise false
inline constexpr drivercode::matcher::Unsupported windows;
//! Matches to true when the current driver is \dx11 and otherwise false
inline constexpr drivercode::matcher::Unsupported dx11;
//! Matches to true when the current driver is \dx12 and otherwise false
inline constexpr drivercode::matcher::Unsupported dx12;
//! Matches to true when the current driver is \vk and otherwise false
inline constexpr drivercode::matcher::Unsupported vulkan;
//! Matches to true when the current platform / driver is \ps4 and otherwise false
inline constexpr drivercode::matcher::Unsupported ps4;
//! Matches to true when the current platform / driver is \ps5 and otherwise false
inline constexpr drivercode::matcher::Unsupported ps5;
//! Matches to true when the current driver is \metal and otherwise false
inline constexpr drivercode::matcher::Unsupported metal;
//! Matches to true when the current driver is null and otherwise false
inline constexpr drivercode::matcher::Unsupported null;
//! Matches to true when the current driver is stub and otherwise false
inline constexpr drivercode::matcher::Unsupported stub;
} // namespace d3d
#endif
namespace d3d::drivercode::matcher
{
//! Helper type for doxygen, do not use!
using NotAnyDriver =
  decltype(!(d3d::dx11 || d3d::dx12 || d3d::vulkan || d3d::ps4 || d3d::ps5 || d3d::metal || d3d::null || d3d::stub));
/**
 * \brief The undefined matcher is special, compared to other composed matchers.
 * \details This matcher matches against all known drivers, and when none of those matchers evaluate to true, it will return true,
 * otherwise to false. The difference to a normal inverted matcher of all known drivers, is that it has a value that can be used for
 * initialization of the driver code state value.
 */
struct Undefined : NotAnyDriver
{
  //! Value of undefined driver state, matchers that are not constant matchers have to match false to this value.
  static constexpr int value = ~0L;
};
} // namespace d3d::drivercode::matcher
namespace d3d
{
//! Matches true when either \xbone or \scarlett matches true, otherwise false.
inline constexpr auto anyXbox = xboxOne || scarlett;
//! Matches true when either \ps4 or \ps5 matches true, otherwise false.
inline constexpr auto anyPS = ps4 || ps5;
//! Matches true when either \mac, \linux or \win32 matches true, otherwise false.
inline constexpr auto anyPC = macOSX || linux || windows;
//! Matches true when no driver could be matched, otherwise false.
//! \note On platforms with a fixed driver, this will always match false.
inline constexpr d3d::drivercode::matcher::Undefined undefined;
//! Matches always true, has special meaning for some uses, like map.
inline constexpr drivercode::matcher::Any any;
//! Matches true when either \ios, \tvos or \mac matches true, otherwise false
inline constexpr auto apple = iOS || tvOS || macOSX;
#if _TARGET_64BIT
inline constexpr drivercode::matcher::Const<false> bit32;
inline constexpr drivercode::matcher::Const<true> bit64;
#else
//! Matches true when the build of the project targets a 32 bit architecture.
inline constexpr drivercode::matcher::Const<true> bit32;
//! Matches true when the build of the project targets a 64 bit architecture.
inline constexpr drivercode::matcher::Const<false> bit64;
#endif
} // namespace d3d

/**
 * Stores which D3D driver is currently active.
 */
class DriverCode
{
protected:
  //! Stores the id value of the currently active driver.
  int value;

  //! Private constructor, use \ref make to create DriverCode instances.
  constexpr DriverCode(int v) : value{v} {}

public:
  //! Enables support for default construction, using any operation on a default constructed DriverCode is undefined behavior.
  DriverCode() = default;
  //! Enables copy construction support.
  DriverCode(const DriverCode &) = default;
  //! Enables copy operation support.
  DriverCode &operator=(const DriverCode &) = default;
  /**
   * Returns the currently stored id value.
   * \returns Currently stored id value as four character code integer.
   * \note For Null and Undefined driver this is either all 0 or all 0xFF.
   **/
  unsigned asFourCC() const { return value; }

  /**
   * Runs the matcher of \p t on the currently stored id value and returns its result.
   * \param t Matcher that should be run with the stored id value.
   * \returns Returns the result of the matcher of \p t.
   */
  template <typename T>
  constexpr bool is(T t) const
  {
    return t.is(value);
  }
  /**
   * Runs the matcher of \p t on the currently stored id value and returns its result.
   * \param t Matcher that should be run with the stored id value.
   * \returns Returns the result of the matcher of \p t.
   */
  template <typename T>
  constexpr bool operator==(T t) const
  {
    return t.is(value);
  }
  /**
   * Runs the matcher of \p t on the currently stored id value and returns its inverted result.
   * \param t Matcher that should be run with the stored id value.
   * \returns Returns the inverted result of the matcher of \p t.
   */
  template <typename T>
  constexpr bool operator!=(T t) const
  {
    return !t.is(value);
  }
  /**
   * Creates a DriverCode value with the ID of the given driver matcher.
   * \param d The matcher that should be used to generate the driver id value, only id matchers work (drivercode::matcher::ID and
   * drivercode::matcher::ConstID). \returns A DriverCode value that will match true for the matcher of the input parameter.
   */
  template <typename D>
  static constexpr DriverCode make(D d)
  {
    return d.value;
  }
  /**
   * Creates a d3d::drivercode::matcher::Map to map driver code matchers to values.
   * \tparam T The type of the value that should be generated by a match.
   * \returns A d3d::drivercode::matcher::Map value that can be used to chain matchers and value pairs and check if anything was
   *matched and retrieve the value.
   **/
  template <typename T>
  d3d::drivercode::matcher::Map<T> map() const
  {
    return {value};
  }
  /**
   * Creates a d3d::drivercode::matcher::Map to with directly passing in the first map expression.
   * \param d The matcher that should match the driver code value against.
   * \param v The value that should be moved into d3d::drivercode::matcher::Map storage should the matcher of \p d return true.
   * \returns A d3d::drivercode::matcher::Map value that can be used to chain matchers and value pairs and check if anything was
   *matched and retrieve the value.
   **/
  template <typename T, typename D>
  d3d::drivercode::matcher::Map<T> map(D d, T &&v) const
  {
    return {value, d, (T &&) v};
  }
  /**
   * Creates a d3d::drivercode::matcher::Map to with directly passing in the first map expression.
   * \param d The matcher that should match the driver code value against.
   * \param v The value that should be moved into the d3d::drivercode::matcher::Map storage should the matcher of \p d return true.
   * \returns A d3d::drivercode::matcher::Map value that can be used to chain matchers and value pairs and check if anything was
   *matched and retrieve the value.
   **/
  template <typename T, typename D>
  d3d::drivercode::matcher::Map<T> map(D d, const T &v) const
  {
    return {value, d, v};
  }
  /**
   * Creates a d3d::drivercode::matcher::Map to with directly passing in the first map expression. This is a specialized version for
   *strings, it will yield a const char * and takes an const array of chars. \param d The matcher that should match the driver code
   *value against. \param v The string of which the pointer should be stored in d3d::drivercode::matcher::Map storage should the
   *matcher of \p d return true. \returns A d3d::drivercode::matcher::Map value that can be used to chain matchers and value pairs and
   *check if anything was matched and retrieve the value.
   **/
  template <typename D, size_t N>
  d3d::drivercode::matcher::Map<const char *> map(D d, const char (&v)[N]) const
  {
    return {value, d, v};
  }
  /**
   * Creates a d3d::drivercode::matcher::Map to with directly passing in the first map expression.
   * \param d The matcher that should match the driver code value against.
   * \param callable A generator callable that is invoked when the matcher of \p d returns true. Its returned value will be passed to
   *the constructor of \p T. \returns A d3d::drivercode::matcher::Map value that can be used to chain matchers and value pairs and
   *check if anything was matched and retrieve the value.
   **/
  template <typename T, typename D, typename C>
  typename d3d::drivercode::matcher::EnableIf<!d3d::drivercode::matcher::SameAs<T, C>::value, d3d::drivercode::matcher::Map<T>>::Type
  map(D d, C &&callable) const
  {
    return {value, d, (C &&) callable};
  }
  /**
   * Creates a d3d::drivercode::matcher::Map to with directly passing in the first map expression.
   * \details Invokes the constructor of \p T should the matcher of \p d return true for a in-place construction of \p T in the storage
   *of the returned d3d::drivercode::matcher::Map. \param d The matcher that should match the driver code value against. \param a1, a2,
   *as Arguments passed to the constructor of \p T should the matcher of \p d return true. \returns A d3d::drivercode::matcher::Map
   *value that can be used to chain matchers and value pairs and check if anything was matched and retrieve the value.
   **/
  template <typename T, typename D, typename A1, typename A2, typename... As>
  d3d::drivercode::matcher::Map<T> map(D d, A1 &&a1, A2 &&a2, As &&...as)
  {
    return {value, d, (A1 &&) a1, (A2 &&) a2, (As &&) as...};
  }
  /**
   * Creates a DriverCodeFirstMatch to match matchers against the driver code value.
   * \returns A DriverCodeFirstMatch value that can be used to chain matchers and check if a callable was called.
   **/
  d3d::drivercode::matcher::FirstMatch match() const { return {value}; }
  /**
   * Creates a DriverCodeFirstMatch with directly passing in the first match expression.
   * \param d The matcher that should match the driver code value against.
   * \param callable The callable that is called should the matcher of \p d return true.
   * \returns A DriverCodeFirstMatch value that can be used to chain additional matchers and check if a callable was called.
   **/
  template <typename D, typename T>
  d3d::drivercode::matcher::FirstMatch match(D d, T &&callable) const
  {
    return {value, d, (T &&) callable};
  }
  /**
   * Creates a DriverCodeFirstMatch to match matchers against the driver code value.
   * \returns A DriverCodeFirstMatch value that can be used to chain matchers and check if a callable was called.
   **/
  d3d::drivercode::matcher::FirstMatch matchFirst() const { return {value}; }
  /**
   * Creates a DriverCodeFirstMatch with directly passing in the first match expression.
   * \param d The matcher that should match the driver code value against.
   * \param callable The callable that is called should the matcher of \p d return true.
   * \returns A DriverCodeFirstMatch value that can be used to chain additional matchers and check if a callable was called.
   **/
  template <typename D, typename T>
  d3d::drivercode::matcher::FirstMatch matchFirst(D d, T &&callable) const
  {
    return {value, d, (T &&) callable};
  }
  /**
   * Creates a DriverCodeAllMatch to match matchers against the driver code value.
   * \returns A DriverCodeAllMatch value that can be used to chain matchers and check if any callable was called.
   **/
  d3d::drivercode::matcher::AllMatch matchAll() const { return {value}; }
  /**
   * Creates a DriverCodeAllMatch with directly passing in the first match expression.
   * \param d The matcher that should match the driver code value against.
   * \param callable The callable that is called should the matcher of \p d return true.
   * \returns A DriverCodeAllMatch value that can be used to chain additional matchers and check if any callable was called.
   **/
  template <typename D, typename T>
  d3d::drivercode::matcher::AllMatch matchAll(D d, T &&callable) const
  {
    return {value, d, (T &&) callable};
  }
};
