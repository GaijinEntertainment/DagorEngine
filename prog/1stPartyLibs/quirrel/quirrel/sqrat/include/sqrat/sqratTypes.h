// Sqrat: altered version by Gaijin Games KFT
// SqratTypes: Type Translators
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
#if !defined(_SQRAT_TYPES_H_)
#define _SQRAT_TYPES_H_

#include <squirrel.h>

#include "sqratClassType.h"
#include "sqratUtil.h"

namespace Sqrat {


template <typename T>
struct getAsInt
{
    static bool getFromStack(HSQUIRRELVM vm, SQInteger idx, T &value)
    {
        static_assert(SQRAT_STD::is_convertible<T, SQInteger>::value, "type is not convertible to int");

        SQObjectType value_type = sq_gettype(vm, idx);
        switch(value_type) {
          case OT_BOOL: {
              SQBool sqValueb = SQFalse;
              SQRESULT res = sq_getbool(vm, idx, &sqValueb);
              value = static_cast<T>(sqValueb);
              return SQ_SUCCEEDED(res);
          }
          case OT_INTEGER: {
              SQInteger sqValue = 0;
              SQRESULT res = sq_getinteger(vm, idx, &sqValue);
              value = static_cast<T>(sqValue);
              return SQ_SUCCEEDED(res);
          }
          case OT_FLOAT: {
              SQFloat sqValuef = 0;
              SQRESULT res = sq_getfloat(vm, idx, &sqValuef);
              value = static_cast<T>(static_cast<int>(sqValuef));
              return SQ_SUCCEEDED(res);
          }
          default:
              SQRAT_ASSERTF(0, FormatTypeError(vm, idx, "integer").c_str());
              value = static_cast<T>(0);
              break;
        }
        return false;
    }
};


template <typename T>
struct getAsFloat
{
    static bool getFromStack(HSQUIRRELVM vm, SQInteger idx, T& value)
    {
        static_assert(SQRAT_STD::is_convertible<T, float>::value, "type is not convertible to float");

        SQObjectType value_type = sq_gettype(vm, idx);
        switch(value_type) {
          case OT_BOOL: {
              SQBool sqValueb = SQFalse;
              SQRESULT res = sq_getbool(vm, idx, &sqValueb);
              value = static_cast<T>(sqValueb);
              return SQ_SUCCEEDED(res);
          }
          case OT_INTEGER: {
              SQInteger sqValue = 0;
              SQRESULT res = sq_getinteger(vm, idx, &sqValue);
              value = static_cast<T>(sqValue);
              return SQ_SUCCEEDED(res);
          }
          case OT_FLOAT: {
              SQFloat sqValuef = 0;
              SQRESULT res = sq_getfloat(vm, idx, &sqValuef);
              value = static_cast<T>(sqValuef);
              return SQ_SUCCEEDED(res);
          }
          default:
              SQRAT_ASSERTF(0, FormatTypeError(vm, idx, "float").c_str());
              value = 0;
              break;
        }
        return false;
    }
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push class instances to and from the stack as copies
///
/// \remarks
/// This specialization requires T to have a default constructor.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T, class>
struct Var {

    using ClassT = ClassType<SQRAT_STD::remove_const_t<T>>;
    T value; ///< The actual value of get operations

    /// Attempts to get the value off the stack at idx as the given type
    Var(HSQUIRRELVM vm, SQInteger idx) { //-V1077
        T* ptr = ClassT::GetInstance(vm, idx);
        if (ptr != NULL) {
            value = *ptr;
        }
    }

    /// Called by Sqrat::PushVar to put a class object on the stack
    static void push(HSQUIRRELVM vm, const T& value) {
        if (ClassT::hasClassData(vm))
            ClassT::PushInstanceCopy(vm, value);
        else
            SQRAT_ASSERTF(0, "Class/typename was not bound");
    }

    static const char * getVarTypeName() { return ClassT::ClassName().c_str(); }

    static bool check_type(HSQUIRRELVM vm, SQInteger idx) {
        return ClassT::IsClassInstance(vm, idx);
    }
};

// Forward-declare Var<> specializations for host project types (if any).
// These turn missing-include bugs into compile errors instead of runtime asserts.
#ifdef SQRAT_FORWARD_VAR_SPECIALIZATIONS
  SQRAT_FORWARD_VAR_SPECIALIZATIONS
#endif

// Var<T> trait to find-out whether value can be used after Var has been
// destroyed. If Var has user-provided destructor it likely controls value
// lifetime
template<typename T>
struct VarControlsValueLifeTime
{
  enum {value = 0};
};

template<class Callable> SQFUNCTION SqGlobalThunk();

template<typename Func>
struct Var<Func, SQRAT_STD::enable_if_t<is_callable_v<Func>>>
{
  static void push(HSQUIRRELVM vm, const Func& value)
  {
    SQFUNCTION funcThunk = SqGlobalThunk<Func>();
    SQUserPointer funcPtr = sq_newuserdata(vm, sizeof(Func));
    new (funcPtr) Func(value);
    sq_setreleasehook(vm, -1, ImplaceFreeReleaseHook<Func>);
    sq_newclosure(vm, funcThunk, 1);
  }
};


/// Used to get and push class instances to and from the stack as references
template<class T>
struct Var<T&, SQRAT_STD::enable_if_t<!SQRAT_STD::is_arithmetic_v<SQRAT_STD::remove_const_t<T>>
                                      && !SQRAT_STD::is_enum_v<SQRAT_STD::remove_const_t<T>>>> {

    using ClassT = ClassType<SQRAT_STD::remove_const_t<T>>;
    T& value; ///< The actual value of get operations

    /// Attempts to get the value off the stack at idx as the given type
    Var(HSQUIRRELVM vm, SQInteger idx) : value(*getCheckedInstance(vm, idx)) {
    }

private:
    static T* getCheckedInstance(HSQUIRRELVM vm, SQInteger idx) {
        T* ptr = ClassT::GetInstance(vm, idx);
        if (!ptr) {
            // Return pointer to aligned dummy storage to avoid null dereference.
            // GetInstance already raised an assert; the caller should check for errors.
            // Uses raw storage instead of T{} to compile for non-default-constructible types.
            alignas(SQRAT_STD::remove_const_t<T>) static char dummy[sizeof(SQRAT_STD::remove_const_t<T>)] = {};
            return reinterpret_cast<T*>(dummy);
        }
        return ptr;
    }

public:

    /// Called by Sqrat::PushVarR to put a class object on the stack
    static void push(HSQUIRRELVM vm, T& value) {
        if (ClassT::hasClassData(vm))
        {
          if constexpr (SQRAT_STD::is_const_v<T>)
            ClassT::PushInstanceCopy(vm, value);
          else
            ClassT::PushNativeInstance(vm, const_cast<SQRAT_STD::remove_const_t<T>*>(&value));
        }
        else
            SQRAT_ASSERTF(0, "Class/typename was not bound");
    }

    static const char * getVarTypeName() { return ClassT::ClassName().c_str(); }

    static bool check_type(HSQUIRRELVM vm, SQInteger idx) {
        return ClassT::IsClassInstance(vm, idx);
    }
};

/// Used to get and push class instances to and from the stack as pointers
template<class T>
struct Var<T*, SQRAT_STD::enable_if_t<!is_callable_v<T*>>> {

    using ClassT = ClassType<SQRAT_STD::remove_const_t<T>>;
    T* value; ///< The actual value of get operations

    /// Attempts to get the value off the stack at idx as the given type
    Var(HSQUIRRELVM vm, SQInteger idx) : value(ClassT::GetInstance(vm, idx, true)) {
    }

    /// Direct extraction from HSQOBJECT -- stackless
    explicit Var(const HSQOBJECT &o) : value(ClassT::GetInstanceFromObj(o)) {}

    /// Called by Sqrat::PushVar to put a class object on the stack
    static void push(HSQUIRRELVM vm, T* value) {
        if (ClassT::hasClassData(vm))
          ClassT::PushNativeInstance(vm, const_cast<SQRAT_STD::remove_const_t<T>*>(value));
        else
          SQRAT_ASSERTF(0, "Class/typename was not bound");
    }

    static const char * getVarTypeName() { return ClassT::ClassName().c_str(); }

    static bool check_type(HSQUIRRELVM vm, SQInteger idx) {
        return ClassT::IsClassInstance(vm, idx);
    }
};


/// Used to get (as copies) and push (as references) class instances to and from the stack as a shared_ptr
template<class T> void PushVarR(HSQUIRRELVM vm, T& value);
template<class T>
struct Var<SQRAT_STD::shared_ptr<T> > {

    SQRAT_STD::shared_ptr<T> value; ///< The actual value of get operations

    /// Attempts to get the value off the stack at idx as the given type
    Var(HSQUIRRELVM vm, SQInteger idx) {
        if (sq_gettype(vm, idx) != OT_NULL) {
            Var<T> instance(vm, idx);
            value.reset(new T(instance.value));
        }
    }

    /// Called by Sqrat::PushVar to put a class object on the stack
    static void push(HSQUIRRELVM vm, const SQRAT_STD::shared_ptr<T>& value) {
        PushVarR(vm, *value);
    }
};


// Arithmetic types (integers and floats, excluding bool which has special handling)
template<typename T>
struct Var<T, SQRAT_STD::enable_if_t<SQRAT_STD::is_arithmetic_v<T> && !SQRAT_STD::is_same_v<T, bool>>> {
    T value;
    Var(HSQUIRRELVM vm, SQInteger idx) {
        if constexpr (SQRAT_STD::is_integral_v<T>)
            getAsInt<T>::getFromStack(vm, idx, value);
        else
            getAsFloat<T>::getFromStack(vm, idx, value);
    }
    explicit Var(const HSQOBJECT &o) {
        SQRAT_ASSERT(check_type(o));
        if constexpr (SQRAT_STD::is_integral_v<T>)
            value = static_cast<T>(sq_objtointeger(&o));
        else
            value = static_cast<T>(sq_objtofloat(&o));
    }
    static void push(HSQUIRRELVM vm, T value) {
        if constexpr (SQRAT_STD::is_integral_v<T>)
            sq_pushinteger(vm, static_cast<SQInteger>(value));
        else
            sq_pushfloat(vm, static_cast<SQFloat>(value));
    }
    static const char * getVarTypeName() { return SQRAT_STD::is_integral_v<T> ? "integer" : "float"; }
    static bool check_type(HSQUIRRELVM vm, SQInteger idx) { return sq_gettype(vm, idx) & SQOBJECT_NUMERIC; }
    static bool check_type(const HSQOBJECT &o) { return o._type & SQOBJECT_NUMERIC; }
};

// Reference/const-reference forwarder for arithmetic types (including bool)
template<class T>
struct Var<T&, SQRAT_STD::enable_if_t<SQRAT_STD::is_arithmetic_v<SQRAT_STD::remove_const_t<T>>>>
    : Var<SQRAT_STD::remove_const_t<T>> {
    using Var<SQRAT_STD::remove_const_t<T>>::Var;
};


///////////////////////////////////////
/// Enums
///////////////////////////////////////

template<typename T>
struct Var<T, SQRAT_STD::enable_if_t<SQRAT_STD::is_enum_v<T>>> {
    T value;
    Var(HSQUIRRELVM vm, SQInteger idx) {
        SQInteger intVal = 0;
        if (SQ_SUCCEEDED(sq_getinteger(vm, idx, &intVal)))
            value = static_cast<T>(intVal);
     }
    explicit Var(const HSQOBJECT &o) {
        SQRAT_ASSERT(check_type(o));
        value = static_cast<T>(sq_objtointeger(&o));
    }

     static void push(HSQUIRRELVM vm, T value) {
         sq_pushinteger(vm, static_cast<SQInteger>(value));
     }
    static const char * getVarTypeName() { return "enum"; }
    static bool check_type(HSQUIRRELVM vm, SQInteger idx) {
        return sq_gettype(vm, idx) == OT_INTEGER;
    }
    static bool check_type(const HSQOBJECT &o) { return o._type == OT_INTEGER; }
};

// Reference/const-reference forwarder for enum types
template<class T>
struct Var<T&, SQRAT_STD::enable_if_t<SQRAT_STD::is_enum_v<SQRAT_STD::remove_const_t<T>>>>
    : Var<SQRAT_STD::remove_const_t<T>> {
    using Var<SQRAT_STD::remove_const_t<T>>::Var;
};


/// Used to get and push bools to and from the stack
template<>
struct Var<bool> {
    bool value; ///< The actual value of get operations

    /// Attempts to get the value off the stack at idx as a bool
    Var(HSQUIRRELVM vm, SQInteger idx) {
        SQBool sqValue;
        sq_tobool(vm, idx, &sqValue);
        value = (sqValue != 0);
    }
    explicit Var(const HSQOBJECT &o) {
        SQRAT_ASSERT(check_type(o));
        value = sq_obj_is_true(&o) != 0;
    }

    /// Called by Sqrat::PushVar to put a bool on the stack
    static void push(HSQUIRRELVM vm, const bool& value) {
        sq_pushbool(vm, static_cast<SQBool>(value));
    }

    static const char * getVarTypeName() { return "bool"; }
    static bool check_type(HSQUIRRELVM /*vm*/, SQInteger /*idx*/) { return true; }
    static bool check_type(const HSQOBJECT &) { return true; }
};

// Trait: true for types with HSQOBJECT-based Var constructors (arithmetic + enum).
// Used by GetSlotValueImpl to dispatch the stackless extraction path at compile time.
template<class T>
inline constexpr bool has_direct_var_v = SQRAT_STD::is_arithmetic_v<T> || SQRAT_STD::is_enum_v<T>;

/// Var<char*> construction is deleted: it exposed non-const write access to VM-internal strings.
/// Use Var<const char*> or Var<string_view> instead. push() is kept for PushVar<char>(vm, ptr).
template<>
struct Var<char*> {
    Var() = delete;
    Var(HSQUIRRELVM, SQInteger) = delete;

    static void push(HSQUIRRELVM vm, const char* value, SQInteger len = -1) {
        sq_pushstring(vm, value, len);
    }

    static const char * getVarTypeName() { return "string"; }
    static bool check_type(HSQUIRRELVM vm, SQInteger idx) { return sq_gettype(vm, idx) == OT_STRING; }
};

/// Used to get and push strings as char arrays to and from the stack
template<>
struct Var<const char*> {
private:
    HSQOBJECT obj = {}; /* hold a reference to the object holding value during the Var struct lifetime*/
    HSQUIRRELVM v = nullptr;

public:
    const char* value = ""; ///< The actual value of get operations
    SQInteger valueLen = 0;

    /// Attempts to get the value off the stack at idx as a character array
    Var(HSQUIRRELVM vm, SQInteger idx) {
        v = vm;
        if (SQ_SUCCEEDED(sq_tostring(vm, idx))) {
            sq_getstackobj(vm, -1, &obj);
            sq_getstringandsize(vm, -1, &value, &valueLen);
            sq_addref(vm, &obj);
            sq_pop(vm,1);
        } else {
            sq_resetobject(&obj);
        }
    }

    Var(Var<const char *> const &rhs)
      : obj(rhs.obj)
      , v(rhs.v)
      , value(rhs.value)
      , valueLen(rhs.valueLen)
    {
      sq_addref(v, &obj);
    }

    Var<const char *> &operator=(Var<const char *> const &rhs)
    {
      if (&rhs == this)
        return *this;
      if (v && !sq_isnull(obj))
        sq_release(v, &obj);
      obj = rhs.obj;
      v = rhs.v;
      value = rhs.value;
      valueLen = rhs.valueLen;
      sq_addref(v, &obj);
      return *this;
    }

    ~Var()
    {
        if(v && !sq_isnull(obj)) {
            sq_release(v, &obj);
        }
    }

    /// Called by Sqrat::PushVar to put a character array on the stack
    static void push(HSQUIRRELVM vm, const char* value, SQInteger len = -1) {
        sq_pushstring(vm, value, len);
    }

    static const char * getVarTypeName() { return "string"; }
    static bool check_type(HSQUIRRELVM vm, SQInteger idx) { return sq_gettype(vm, idx) == OT_STRING; }
};

template<>
struct VarControlsValueLifeTime<const char*>
{
  enum {value = 1};
};

/// Used to get and push strings to and from the stack
template<>
struct Var<string> {
    string value; ///< The actual value of get operations

    /// Attempts to get the value off the stack at idx as a string
    Var(HSQUIRRELVM vm, SQInteger idx) {
        if (SQ_FAILED(sq_tostring(vm, idx)))
            return;
        const char* ret = nullptr;
        sq_getstring(vm, -1, &ret);
        value = string(ret, sq_getsize(vm, -1));
        sq_pop(vm,1);
    }

    /// Called by Sqrat::PushVar to put a string on the stack
    static void push(HSQUIRRELVM vm, const string& value) {
        sq_pushstring(vm, value.c_str(), value.size());
    }

    static const char * getVarTypeName() { return "string"; }
    static bool check_type(HSQUIRRELVM vm, SQInteger idx) { return sq_gettype(vm, idx) == OT_STRING; }
};

/// Const string references are handled as copies (strings are always copied)
template<>
struct Var<const string&> : Var<string> {
    using Var<string>::Var;
};


/// Used to get and push strings as string_view objects
template <>
struct Var<string_view>
{
private:
  HSQOBJECT obj = {}; /* hold a reference to the object holding value during the Var struct lifetime*/
  HSQUIRRELVM v = nullptr;

public:
  string_view value;

  Var(HSQUIRRELVM vm, SQInteger idx)
  {
    v = vm;
    if (SQ_SUCCEEDED(sq_tostring(vm, idx))) {
      sq_getstackobj(vm, -1, &obj);
      const char *strPtr = nullptr;
      SQInteger strLen = 0;
      sq_getstringandsize(vm, -1, &strPtr, &strLen);
      sq_addref(vm, &obj);
      sq_pop(vm, 1);
      value = string_view(strPtr, strLen);
    } else {
      sq_resetobject(&obj);
    }
  }

  ~Var()
  {
    if (v && !sq_isnull(obj))
      sq_release(v, &obj);
  }

  Var(Var<string_view> const &rhs)
    : obj(rhs.obj)
    , v(rhs.v)
    , value(rhs.value)
  {
    sq_addref(v, &obj);
  }

  Var<string_view> &operator=(Var<string_view> const &rhs)
  {
    if (&rhs == this)
      return *this;
    if (v && !sq_isnull(obj))
      sq_release(v, &obj);
    obj = rhs.obj;
    v = rhs.v;
    value = rhs.value;
    sq_addref(v, &obj);
    return *this;
  }

  static void push(HSQUIRRELVM vm, string_view sv)
  {
    sq_pushstring(vm, sv.data(), sv.size());
  }

  static const char *getVarTypeName()
  {
    return "string";
  }
  static bool check_type(HSQUIRRELVM vm, SQInteger idx)
  {
    return sq_gettype(vm, idx) == OT_STRING;
  }
};

template <>
struct VarControlsValueLifeTime<string_view> // Var holds reference to VM's string which is pointed by string_view
{
  enum
  {
    value = 1
  };
};


// Non-referencable type definitions
template <class T, class = void> struct is_referencable : public SQRAT_STD::true_type {};
template <class T>
struct is_referencable<T, SQRAT_STD::enable_if_t<SQRAT_STD::is_scalar<T>::value>> : public SQRAT_STD::false_type {};

#define SQRAT_MAKE_NONREFERENCABLE( type ) \
 template<> struct is_referencable<type> : public SQRAT_STD::false_type {};

SQRAT_MAKE_NONREFERENCABLE(string)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pushes a value on to a given VM's stack
///
/// \remarks
/// What this function does is defined by Sqrat::Var template specializations,
/// and thus you can create custom functionality for it by making new template specializations.
/// When making a custom type that is not referencable, you must use SQRAT_MAKE_NONREFERENCABLE( type )
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
inline void PushVar(HSQUIRRELVM vm, T* value) {
    Var<T*>::push(vm, value);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pushes a value on to a given VM's stack
///
/// \remarks
/// What this function does is defined by Sqrat::Var template specializations,
/// and thus you can create custom functionality for it by making new template specializations.
/// When making a custom type that is not referencable, you must use SQRAT_MAKE_NONREFERENCABLE( type )
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
inline void PushVar(HSQUIRRELVM vm, const T& value) {
    Var<T>::push(vm, value);
}


/// Pushes a reference on to a given VM's stack (some types cannot be referenced and will be copied instead)
template<class T>
inline void PushVarR(HSQUIRRELVM vm, T& value) {
    if constexpr (!SQRAT_STD::is_pointer_v<T> && is_referencable<SQRAT_STD::remove_cv_t<T>>::value) {
        Var<T&>::push(vm, value);
    } else if constexpr (SQRAT_STD::is_scalar_v<T>) {
        PushVar<T>(vm, value);
    } else {
        PushVar<const T&>(vm, value);
    }
}

namespace vargs
{
  template <typename... Args, size_t... Indeces>
  SQRAT_STD::tuple<Var<Args>...> make_vars_i(HSQUIRRELVM vm, int idx, SQRAT_STD::index_sequence<Indeces...>)
  {
    ((void)idx);
    ((void)vm);
    return SQRAT_STD::make_tuple(Var<Args>(vm, idx + Indeces)...);
  }

  template <typename... Args>
  SQRAT_STD::tuple<Var<Args>...> make_vars(HSQUIRRELVM vm, int idx)
  {
    return make_vars_i<Args...>(vm, idx, SQRAT_STD::index_sequence_for<Args...>());
  }

  template <typename T>
  bool check_var_types(HSQUIRRELVM vm, int idx)
  {
    if (!Var<T>::check_type(vm, idx)) {
      const char *argTypeName = "unknown";
      SQInteger prevTop = sq_gettop(vm);
      if (SQ_SUCCEEDED(sq_typeof(vm, idx)) && SQ_SUCCEEDED(sq_tostring(vm, -1))) {
        sq_getstring(vm, -1, &argTypeName);
      }

      char errMsg[128];
      snprintf(errMsg, sizeof(errMsg), "Wrong argument type, expected '%s', got '%s'",
                    Var<T>::getVarTypeName(), argTypeName);
      sq_settop(vm, prevTop);
      (void)sq_throwerror(vm, errMsg);
      return false;
    }
    return true;
  }

  template <typename Head, typename... Tail>
  bool check_var_types(HSQUIRRELVM vm, int idx, SQRAT_STD::enable_if_t<(sizeof...(Tail) > 0), bool> = false)
  {
    if (!check_var_types<Head>(vm, idx))
      return false;
    return check_var_types<Tail...>(vm, idx+1);
  }

  template <typename... T>
  bool check_var_types(HSQUIRRELVM /*vm*/,
                       int /*idx*/,
                       SQRAT_STD::enable_if_t<(sizeof...(T) == 0), bool> = false)
  {
    return true;
  }
}


// utility for checking types of SquirrelFunc arguments
template <typename... Args>
bool check_signature(HSQUIRRELVM vm, SQInteger start_stack_pos=1) {
    return vargs::check_var_types<Args...>(vm, start_stack_pos);
}


}  // namespace Sqrat

#endif
