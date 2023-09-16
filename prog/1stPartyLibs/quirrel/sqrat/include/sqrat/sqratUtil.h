// Sqrat: altered version by Gaijin Games KFT
// SqratUtil: Quirrel Utilities
//

//
// Copyright (c) 2009 Brandon Jones
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//  claim that you wrote the original software. If you use this software
//  in a product, an acknowledgment in the product documentation would be
//  appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//  misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//  distribution.
//

#pragma once
#if !defined(_SQRAT_UTIL_H_)
#define _SQRAT_UTIL_H_

#ifdef USE_SQRAT_CONFIG

  #include "sqratConfig.h"

#else

  #include <assert.h>

  #define SQRAT_ASSERT assert
  #define SQRAT_ASSERTF(cond, msg, ...) assert((cond) && (msg))
  #define SQRAT_VERIFY(cond) do { if (!(cond)) assert(#cond); } while(0)
#endif

#include <squirrel.h>
#include <sqstdaux.h>

#if (defined(_MSC_VER) && _MSC_VER >= 1900) || (__cplusplus >= 201402L) || (__cplusplus == 201300L)
#else
  #error C++14 support required
#endif

#if defined(SQRAT_HAS_SKA_HASH_MAP)
# include <ska_hash_map/flat_hash_map2.hpp>
#endif

#ifndef SQRAT_STD
#define SQRAT_STD std
#endif

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) // c++17
# define SQRAT_STD_CPP17
#endif

#if defined(SQRAT_HAS_EASTL)
# include <EASTL/string.h>
# include <EASTL/string_view.h>
# include <EASTL/unordered_map.h>
# include <EASTL/unordered_set.h>
# include <EASTL/vector_map.h>
# include <EASTL/shared_ptr.h>
EA_DISABLE_ALL_VC_WARNINGS()
#else
# include <string>
# include <unordered_map>
# include <unordered_set>
# include <memory>
# include <tuple>
# include <type_traits>
# if defined(SQRAT_STD_CPP17)
#   include <string_view>
# endif
#endif


namespace Sqrat {

#if defined(SQRAT_HAS_EASTL)
  using string = eastl::basic_string<SQChar>;
  using string_view = eastl::basic_string_view<SQChar>;
  template <class T> using hash = eastl::hash<T>;
  template <class T> using shared_ptr = eastl::shared_ptr<T>;
  template <class T> using weak_ptr = eastl::weak_ptr<T>;
# define SQRAT_HAS_STRING_VIEW 1
#else
  using string = std::basic_string<SQChar>;
  template <class T> using hash = std::hash<T>;
  template <class T> using shared_ptr = std::shared_ptr<T>;
  template <class T> using weak_ptr = std::weak_ptr<T>;
# if defined(SQRAT_STD_CPP17)
  using string_view = std::basic_string_view<SQChar>;
# define SQRAT_HAS_STRING_VIEW 1
# endif
#endif //defined(SQRAT_HAS_EASTL)

#if defined(SQRAT_HAS_SKA_HASH_MAP)

  template <class K, class V, typename H = hash<K>>
  using class_hash_map = ska::flat_hash_map<K, V, H>;

  template <typename T, typename H = hash<T>>
  using hash_set = ska::flat_hash_set<T, H>;

#elif defined(SQRAT_HAS_EASTL)

  template <class K, class V, typename H = hash<K>>
  using class_hash_map = eastl::unordered_map<K, V, H>;

  template <typename T, typename H = hash<T>>
  using hash_set = eastl::unordered_set<T, H>;

#else

  template <class K, class V, typename H = hash<K>>
  using class_hash_map = std::unordered_map<K, V, H>;

  template <typename T, typename H = hash<T>>
  using hash_set = std::unordered_set<T, H>;

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Define an inline function to avoid MSVC's "conditional expression is constant" warning
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
    template <typename T>
    inline T _c_def(T value) { return value; }
    #define SQRAT_CONST_CONDITION(value) _c_def(value)
#else
    #define SQRAT_CONST_CONDITION(value) value
#endif


template <typename T>
void SQRAT_UNUSED(const T&) {}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Returns a string that has been formatted to give a nice type error message (for usage with Class::SquirrelFunc)
///
/// \param vm           VM the error occurred with
/// \param idx          Index on the stack of the argument that had a type error
/// \param expectedType The name of the type that the argument should have been
///
/// \return String containing a nice type error message
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline string FormatTypeError(HSQUIRRELVM vm, SQInteger idx, const SQChar *expectedType) {
    SQInteger prevTop = sq_gettop(vm);
    const SQChar* actualType = _SC("unknown");
    if (SQ_SUCCEEDED(sq_typeof(vm, idx))) {
        sq_tostring(vm, -1);
        sq_getstring(vm, -1, &actualType);
    }

    const SQChar *fmtString = _SC("wrong type (%s expected, got %s)");
    int l = snprintf(nullptr, 0, fmtString, expectedType, actualType);
    string err(l + 1, '\0');
    snprintf(&err[0], err.size(), fmtString, expectedType, actualType);

    sq_settop(vm, prevTop);
    return err;
}


/// Returns the last error that occurred with a Squirrel VM (not associated with Sqrat errors)
inline string LastErrorString(HSQUIRRELVM vm) {
    const SQChar* sqErr = _SC("n/a");
    sq_getlasterror(vm);
    if (sq_gettype(vm, -1) == OT_NULL) {
        sq_pop(vm, 1);
        return string();
    }
    sq_tostring(vm, -1);
    sq_getstring(vm, -1, &sqErr);
    string res(sqErr);
    sq_pop(vm, 2);
    return res;
}

template<class Obj>
SQInteger ImplaceFreeReleaseHook(SQUserPointer p, SQInteger)
{
  SQRAT_UNUSED(p); // for Obj without destructor
  static_cast<Obj*>(p)->~Obj();
  return 1;
}

template<class T, class = void> struct Var;

// utilities for manipulations with variadic templates arguments
namespace vargs {
  template<class... T>
  struct TailElem
  {
    typedef SQRAT_STD::tuple<T...> tuple;
    typedef typename SQRAT_STD::tuple_element<SQRAT_STD::tuple_size<tuple>::value - 1, tuple>::type type;
  };

  template<class Head, class... T>
  struct HeadElem
  {
    typedef Head type;
  };

  template<class... T>
  using TailElem_t = typename TailElem<T...>::type;

  template<class... T>
  using HeadElem_t = typename HeadElem<T...>::type;

  template<class Arg>
  constexpr Arg tail(Arg&& arg)
  {
    return arg;
  }

  template<class Head, class... Tail>
  constexpr TailElem_t<Head, Tail...> tail(Head&&, Tail&&... args)
  {
    return tail(SQRAT_STD::forward<Tail>(args)...);
  }

  template<class Arg>
  constexpr Arg head(Arg&& )
  {
    return head;
  }

  template<class Head, class... Tail>
  constexpr Head head(Head&& head, Tail&&... )
  {
    return head;
  }

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif
  template <class Func, class Tuple, size_t... Indexes>
  auto apply_helper(Func pf, SQRAT_STD::index_sequence<Indexes...>, Tuple &args)
  {
    return pf(SQRAT_STD::get<Indexes>(args).value...);
  }

  template <class Func, class Tuple>
  auto apply(Func pf, Tuple &&args)
  {
    constexpr auto argsN = SQRAT_STD::tuple_size<SQRAT_STD::decay_t<Tuple>>::value;
    return apply_helper(pf, SQRAT_STD::make_index_sequence<argsN>(), args);
  }

  template <class C, class Member, class Tuple, size_t... Indexes>
  auto apply_member_helper(C *ptr, Member pf, SQRAT_STD::index_sequence<Indexes...>,
                           Tuple &args)
  {
    return (ptr->*pf)(SQRAT_STD::get<Indexes>(args).value...);
  }

  template <class C, class Member, class Tuple>
  auto apply_member(C *ptr, Member pf, Tuple &&args)
  {
    constexpr auto argsN = SQRAT_STD::tuple_size<SQRAT_STD::decay_t<Tuple>>::value;
    return apply_member_helper(ptr, pf, SQRAT_STD::make_index_sequence<argsN>(), args);
  }
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}

// add meta-functions aliases that are missing in std
template <typename T>
using remove_pointer_t = typename SQRAT_STD::remove_pointer<T>::type;

template <typename T>
using remove_const_t = typename SQRAT_STD::remove_const<T>::type;

#if __cplusplus < 201703L
template <typename...>
struct __or;

template <>
struct __or<> : SQRAT_STD::false_type
{
};
template <typename B1>
struct __or<B1> : B1
{
};
template <typename B1, typename B2>
struct __or<B1, B2> : SQRAT_STD::conditional<B1::value, B1, B2>::type
{
};

template <typename B1, typename B2, typename B3, typename... Bn>
struct __or<B1, B2, B3, Bn...> : SQRAT_STD::conditional<B1::value, B1, __or<B2, B3, Bn...>>::type
{
};

template<typename... Tn>
struct disjunction : __or<Tn...> {};

template<typename... Ts> struct make_void { typedef void type; };
template<typename... Ts> using void_t = typename make_void<Ts...>::type;
#else
template<typename ...Tn>
using disjunction = SQRAT_STD::disjunction<Tn...>;
template <typename ...Tn>
using void_t = SQRAT_STD::void_t<Tn...>;
#endif

template <typename T>
struct is_function
  : disjunction<SQRAT_STD::is_function<T>,
                SQRAT_STD::is_function<SQRAT_STD::remove_pointer_t<SQRAT_STD::remove_reference_t<T>>>>
{
};

template <bool B, typename T>
using disable_if = SQRAT_STD::enable_if<!B, T>;

template <typename T>
struct member_function_signature
{
  using type = void;
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...)>
{
  using type = R(A...);
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...) const>
{
  using type = R(A...);
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...) volatile>
{
  using type = R(A...);
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...) volatile const>
{
  using type = R(A...);
};

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) // c++17

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...) noexcept>
{
  using type = R(A...);
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...) const noexcept>
{
  using type = R(A...);
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...) volatile noexcept>
{
  using type = R(A...);
};

template <typename C, typename R, typename... A>
struct member_function_signature<R (C::*)(A...) volatile const noexcept>
{
  using type = R(A...);
};

#endif // c++17

template<typename T>
using member_function_signature_t = typename member_function_signature<T>::type;

template<class T>
struct get_class_callop_signature
{
  using type = member_function_signature_t<decltype(&SQRAT_STD::remove_reference_t<T>::operator())>;
};

template<class T>
using get_class_callop_signature_t = typename get_class_callop_signature<T>::type;

template<class F>
struct get_function_signature
{
  using type = void;
};

template <typename R, typename... A>
struct get_function_signature<R(A...)>
{
  using type = R(A...);
};

template <typename R, typename... A>
struct get_function_signature<R (&)(A...)>
{
  using type = R(A...);
};

template <typename R, typename... A>
struct get_function_signature<R (*)(A...)>
{
  using type = R(A...);
};

template <typename R, typename... A>
struct get_function_signature<R (*&)(A...)>
{
  using type = R(A...);
};

template<typename Func>
using get_function_signature_t = typename get_function_signature<Func>::type;


template<typename T>
struct get_callable_function : SQRAT_STD::conditional_t<SQRAT_STD::is_member_function_pointer<T>::value,
                                                    member_function_signature<T>,
                                                    SQRAT_STD::conditional_t<is_function<T>::value,
                                                                          get_function_signature<remove_pointer_t<T>>,
                                                                          SQRAT_STD::conditional_t<SQRAT_STD::is_class<T>::value,
                                                                                            get_class_callop_signature<T>,
                                                                                            void_t<T>>
                                                                        >

                                                    >
{
};


template<typename T>
using get_callable_function_t = typename get_callable_function<T>::type;

template<typename Function>
struct result_of;

template<typename R, typename... A>
struct result_of<R(A...)>
{
  using type = R;
};

template<typename F>
using result_of_t = typename result_of<F>::type;

template<typename Function>
struct function_args_num;

template<typename R, typename... A>
struct function_args_num<R(A...)>
{
  static size_t const value = sizeof...(A);
};

template<typename F>
constexpr size_t function_args_num_v = function_args_num<F>::value;

template<typename T>
struct has_call_operator
{
  using yes = char[2];
  using no  = char[1];

  struct Fallback { void operator()();};
  struct Derived : T, Fallback { Derived() {} };

  template<typename U>
  static no& test(decltype(&U::operator())*);

  template<typename U>
  static yes& test(...);

  static bool const value = sizeof(test<Derived>(0)) == sizeof(yes);
};

template<typename T>
struct is_callable :
  SQRAT_STD::conditional<SQRAT_STD::is_class<T>::value,
                      has_call_operator<T>,
                      is_function<T>>::type
{
};

template<typename T>
constexpr int is_callable_v = is_callable<T>::value;

template<class F>
int SqGetArgCount()
{
  return function_args_num_v<get_callable_function_t<F>>;
}

}

#if defined(SQRAT_HAS_EASTL)
EA_RESTORE_ALL_VC_WARNINGS()
#endif

#endif
