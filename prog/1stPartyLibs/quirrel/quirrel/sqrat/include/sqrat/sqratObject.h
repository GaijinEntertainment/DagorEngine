// Sqrat: altered version by Gaijin Games KFT
// SqratObject: Referenced Quirrel Object Wrapper
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
#if !defined(_SQRAT_OBJECT_H_)
#define _SQRAT_OBJECT_H_

#include <squirrel.h>
#include <string.h>

#include "sqratAllocator.h"
#include "sqratTypes.h"
#include "sqratUtil.h"

namespace Sqrat {

class Table;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// The base class for classes that represent Squirrel objects
///
/// \remarks
/// All Object and derived classes MUST be destroyed before calling sq_close or your application will crash when exiting.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Object {
protected:

    HSQUIRRELVM vm;
    HSQOBJECT obj;

public:

    Object(HSQUIRRELVM v) : vm(v){
        sq_resetobject(&obj);
    }

    Object() : vm(0) {
        sq_resetobject(&obj);
    }

    Object(const Object& so) : vm(so.vm), obj(so.obj) {
      sq_addref(vm, &obj);
    }

    Object(Object && so) : vm(so.vm), obj(so.obj) {
        sq_resetobject(&so.obj);
    }

    Object(const Object &&)=delete;

    Object(HSQOBJECT o, HSQUIRRELVM v) : vm(v), obj(o) {
      sq_addref(vm, &obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs an Object from a known/bound type
    ///
    /// \param t        If t is pointer to a C++ class instance then it has to be bound already, otherwise it's ref to value of known type
    /// \param v        VM that the object will exist in
    ///
    /// \tparam T       Type
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <class T, SQRAT_STD::enable_if_t<!SQRAT_STD::is_arithmetic_v<T>, bool> = false>
    Object(const T &t, HSQUIRRELVM v) : vm(v) {
        Var<T>::push(vm, t);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &obj)));
        sq_addref(vm, &obj);
        sq_poptop(vm);
    }

    Object(const char *str, HSQUIRRELVM v, SQInteger str_len = -1) : vm(v) {
        if (str) {
            sq_pushstring(vm, str, str_len);
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &obj)));
            sq_addref(vm, &obj);
            sq_poptop(vm);
        }
        else
            sq_resetobject(&obj);
    }

    Object(char *str, HSQUIRRELVM v, SQInteger str_len = -1) : vm(v) {
        if (str) {
            sq_pushstring(vm, str, str_len);
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &obj)));
            sq_addref(vm, &obj);
            sq_poptop(vm);
        }
        else
            sq_resetobject(&obj);
    }

    template <class T, SQRAT_STD::enable_if_t<SQRAT_STD::is_arithmetic_v<T>, bool> = false>
    Object(T t, HSQUIRRELVM v) : vm(v) {
        sq_resetobject(&obj);
        if constexpr (SQRAT_STD::is_same_v<T, bool>) {
            obj._type = OT_BOOL;
            obj._unVal.nInteger = t ? 1 : 0;
        } else if constexpr (SQRAT_STD::is_floating_point_v<T>) {
            obj._type = OT_FLOAT;
            obj._unVal.fFloat = (SQFloat)t;
        } else {
            obj._type = OT_INTEGER;
            obj._unVal.nInteger = (SQInteger)t;
        }
    }

    ~Object() {
        Release();
    }

    Object& operator=(const Object& so) {
        if (&so != this)
        {
          Release();
          vm = so.vm;
          obj = so.obj;
          sq_addref(vm, &obj);
        }
        return *this;
    }

    Object& operator=(Object && so) {
        if (&so != this)
        {
          Release();
          vm = so.vm;
          obj = so.obj;
          sq_resetobject(&so.obj);
        }
        return *this;
    }

    HSQUIRRELVM& GetVM() {
        return vm;
    }

    HSQUIRRELVM GetVM() const {
        return vm;
    }

    SQObjectType GetType() const {
        return GetObject()._type;
    }

    bool IsNull() const {
        return sq_isnull(GetObject());
    }

    const HSQOBJECT &GetObject() const {
        return obj;
    }

    HSQOBJECT& GetObject() {
        return obj;
    }

    operator HSQOBJECT&() {
        return GetObject();
    }

    void Release() {
        sq_release(vm, &obj);
        sq_resetobject(&obj);
    }

    bool IsEqual(const Sqrat::Object &so) const {
        if (IsNull() && so.IsNull()) // Nulls may have uninitialized vm
            return true;
        if (GetVM() != so.GetVM())
            return false;

        return sq_obj_is_equal(vm, &obj, &so.obj);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value of a slot from the object
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <bool raw>
    Object GetSlotImpl(const char* slot, int slen) const {
        HSQOBJECT slotObj;
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, slot, slen);

        SQRESULT res = raw ? sq_rawget(vm, -2) : sq_get(vm, -2);
        if(SQ_FAILED(res)) {
            sq_pop(vm, 1);
            return Object(vm); // Return a NULL object
        } else {
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &slotObj)));
            Object ret(slotObj, vm); // must addref before the pop!
            sq_pop(vm, 2);
            return ret;
        }
    }

    Object GetSlot(const char* slot) const {
        return GetSlotImpl<false>(slot, strlen(slot));
    }

    Object RawGetSlot(const char* slot) const {
        return GetSlotImpl<true>(slot, strlen(slot));
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value of an index from the object
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <bool raw>
    Object GetSlotImpl(SQInteger index) const {
        HSQOBJECT slotObj;
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);

        SQRESULT res = raw ? sq_rawget(vm, -2) : sq_get(vm, -2);
        if(SQ_FAILED(res)) {
            sq_pop(vm, 1);
            return Object(vm); // Return a NULL object
        } else {
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &slotObj)));
            Object ret(slotObj, vm); // must addref before the pop!
            sq_pop(vm, 2);
            return ret;
        }
    }

    Object GetSlot(SQInteger index) const {
        return GetSlotImpl<false>(index);
    }

    Object RawGetSlot(SQInteger index) const {
        return GetSlotImpl<true>(index);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value of an key from the object
    /// \param slot Key (of any type) of the slot
    /// \return An Object representing the value of the slot (can be a null object if nothing was found)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <bool raw>
    Object GetSlotImpl(const Object& slot) const {
        SQRAT_ASSERT(slot.IsNull() || slot.GetVM() == vm);
        HSQOBJECT res;
        if (SQ_FAILED(sq_obj_get(vm, &obj, &slot.obj, &res, raw)))
            return Object(vm);
        Object ret(res, vm);
        sq_poptop(vm);
        return ret;
    }

    Object GetSlot(const Object& slot) const {
        return GetSlotImpl<false>(slot);
    }

    Object RawGetSlot(const Object& slot) const {
        return GetSlotImpl<true>(slot);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Casts the object to a certain C++ type
    /// \tparam T Type to cast to
    /// \return A copy of the value of the Object with the given type
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <class T>
    T Cast() const {
        static_assert(VarControlsValueLifeTime<T>::value == 0,
                      "direct cast to T failed due to value is bound to Var<T>. use GetVar() instead");
        return GetVar<T>().value;
    }

    template<class T>
    Var<T>  GetVar() const
    {
        sq_pushobject(vm, GetObject());
        Var<T> ret(vm, -1);
        sq_pop(vm, 1);
        return ret;
    }

    /// Gets object slot value as a certain C++ type
    template<class T, bool raw>
    T GetSlotValueImpl(const char* slot, int slen, T def_val) const {
        static_assert(VarControlsValueLifeTime<T>::value == 0,
                      "direct cast to T failed due to value is bound to Var<T>");
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, slot, slen);

        SQRESULT res = raw ? sq_rawget(vm, -2) : sq_get(vm, -2);
        if (SQ_FAILED(res)) {
            sq_pop(vm, 1);
            return def_val;
        } else if (!Var<T>::check_type(vm, -1)) {
            sq_pop(vm, 2);
            return def_val;
        } else {
            T ret = Var<T>(vm, -1).value;
            sq_pop(vm, 2);
            return ret;
        }
    }

    template<class T>
    T GetSlotValue(const char* slot, T def_val) const {
        return GetSlotValueImpl<T, false>(slot, strlen(slot), def_val);
    }

    template<class T>
    T RawGetSlotValue(const char* slot, T def_val) const {
        return GetSlotValueImpl<T, true>(slot, strlen(slot), def_val);
    }

    template<class T, bool raw>
    T GetSlotValueImpl(SQInteger slot, T def_val) const {
        static_assert(VarControlsValueLifeTime<T>::value == 0,
                      "direct cast to T failed due to value is bound to Var<T>");
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, slot);

        SQRESULT res = raw ? sq_rawget(vm, -2) : sq_get(vm, -2);
        if (SQ_FAILED(res)) {
            sq_pop(vm, 1);
            return def_val;
        } else if (!Var<T>::check_type(vm, -1)) {
            sq_pop(vm, 2);
            return def_val;
        } else {
            T ret = Var<T>(vm, -1).value;
            sq_pop(vm, 2);
            return ret;
        }
    }

    template<class T>
    T GetSlotValue(SQInteger slot, T def_val) const {
        return GetSlotValueImpl<T, false>(slot, def_val);
    }

    template<class T>
    T RawGetSlotValue(SQInteger slot, T def_val) const {
        return GetSlotValueImpl<T, true>(slot, def_val);
    }

    /// Gets object slot value as a certain C++ type
    template<class T, bool raw>
    T GetSlotValueImpl(const Object &key, T def_val) const {
        SQRAT_ASSERT(key.IsNull() || key.GetVM() == vm);
        static_assert(VarControlsValueLifeTime<T>::value == 0,
                      "direct cast to T failed due to value is bound to Var<T>");
        HSQOBJECT res;
        if (SQ_FAILED(sq_obj_get(vm, &obj, &key.obj, &res, raw)))
          return def_val;

        // sq_obj_get already pushed result to VM stack
        if constexpr (has_direct_var_v<T>) {
            T ret = Var<T>::check_type(res) ? Var<T>(res).value : def_val;
            sq_pop(vm, 1);
            return ret;
        } else {
            if (!Var<T>::check_type(vm, -1)) {
                sq_pop(vm, 1);
                return def_val;
            }
            T ret = Var<T>(vm, -1).value;
            sq_pop(vm, 1);
            return ret;
        }
    }

    template<class T>
    T GetSlotValue(const Object &key, T def_val) const {
        return GetSlotValueImpl<T, false>(key, def_val);
    }

    template<class T>
    T RawGetSlotValue(const Object &key, T def_val) const {
        return GetSlotValueImpl<T, true>(key, def_val);
    }

    template <class T>
    inline Object operator[](T slot) {
        return GetSlot(slot);
    }

    template <class T>
    const Object operator[](T slot) const {
        return GetSlot(slot);
    }

    SQInteger GetSize() const {
        return sq_obj_getsize(&obj);
    }

    struct iterator;
    bool Next(iterator& iter) const;

    void FreezeSelf() {
        obj._flags |= SQOBJ_FLAG_IMMUTABLE;
    }

    void UnfreezeSelf() {
        obj._flags &= ~SQOBJ_FLAG_IMMUTABLE;
    }

    const SQRAT_STD::string_view GetString(const SQRAT_STD::string_view def_val = {}) const {
        const char *str = sq_objtostring(&obj);
        if (!str)
            return def_val;
        return SQRAT_STD::string_view(str, sq_obj_getsize(&obj));
    }


protected:
    template<class Func>
    void BindFunc(const char* name, Func func, SQFUNCTION func_thunk, SQInteger nparamscheck, bool staticVar = false, const char *docstring=nullptr)
    {
      sq_pushobject(vm, GetObject());
      sq_pushstring(vm, name, -1);

      SQUserPointer funcPtr = sq_newuserdata(vm, sizeof(func));
      new (funcPtr) Func(func);
      sq_setreleasehook(vm, -1, ImplaceFreeReleaseHook<Func>);

      sq_newclosure(vm, func_thunk, 1);
      if (nparamscheck > 0)
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, nparamscheck, nullptr)));
      SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosurename(vm, -1, name)));
      if (docstring)
          SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));

      SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
      sq_pop(vm,1); // pop table
    }

    template<class Func>
    void BindFunc(SQInteger index, Func func, SQFUNCTION func_thunk, SQInteger nparamscheck, bool staticVar = false, const char *docstring=nullptr)
    {
      sq_pushobject(vm, GetObject());
      sq_pushinteger(vm, index);

      SQUserPointer funcPtr = sq_newuserdata(vm, sizeof(func));
      new (funcPtr) Func(func);
      sq_setreleasehook(vm, -1, ImplaceFreeReleaseHook<Func>);

      sq_newclosure(vm, func_thunk, 1);
      if (nparamscheck > 0)
         SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, nparamscheck, nullptr)));
      if (docstring)
          SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));

      SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
      sq_pop(vm,1); // pop table
    }

    /// Set the value of a variable on the object. Changes to values set this way are not reciprocated
    template<class V>
#ifdef _MSC_VER
    __declspec(noinline) // To force `SetValue` to be inlined for `strlen(name)`
#elif defined(__GNUC__)
    __attribute__((noinline))
#endif
    inline void BindValue(const char* name, int nlen, const V& val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, nlen);
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }

    template<class V>
    inline void BindValue(const  SQRAT_STD::string_view name, const V& val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name.data(), name.size());
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }

    template<class V>
    inline void BindValue(const SQInteger index, const V& val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }

    template<class V>
    inline void BindValue(const Object &key, const V& val, bool staticVar = false) {
        PushVar(vm, val);
        HSQOBJECT valObj;
        sq_getstackobj(vm, -1, &valObj);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_obj_newslot(vm, &obj, &key.obj, &valObj, staticVar)));
        sq_poptop(vm);
    }

    // Set the value of an instance on the object. Changes to values set this way are reciprocated back to the source instance
    template<class V>
    inline void BindInstance(const char* name, V* val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushstring(vm, name, -1);
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }

    template<class V>
    inline void BindInstance(const SQInteger index, V* val, bool staticVar = false) {
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);
        PushVar(vm, val);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
        sq_pop(vm,1); // pop table
    }

    template<class V>
    inline void BindInstance(const Object &key, V* val, bool staticVar = false) {
        PushVar(vm, val);
        HSQOBJECT valObj;
        sq_getstackobj(vm, -1, &valObj);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_obj_newslot(vm, &obj, &key.obj, &valObj, staticVar)));
        sq_poptop(vm);
    }
};

// Optimized no-stack-push-and-pop versions
template<> inline int32_t Object::Cast() const {
    return sq_objtointeger(&obj);
}

template<> inline int64_t Object::Cast() const {
    return sq_objtointeger(&obj);
}

template<> inline float Object::Cast() const {
    return sq_objtofloat(&obj);
}

template<> inline double Object::Cast() const {
    return sq_objtofloat(&obj);
}

template<> inline bool Object::Cast() const {
    return sq_obj_is_true(&obj) != SQFalse;
}


template<>
#ifdef _MSC_VER
  __declspec(noinline) // To force `SetValue` to be inlined for `strlen(name)`
#elif defined(__GNUC__)
  __attribute__((noinline))
#endif
inline void Object::BindValue<int>(const char* name, int nlen, const int & val, bool staticVar /* = false */) {
    sq_pushobject(vm, GetObject());
    sq_pushstring(vm, name, nlen);
    PushVar<int>(vm, val);
    SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, staticVar)));
    sq_pop(vm,1); // pop table
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Object instances to and from the stack as references (Object is always a reference)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<Object> {

    Object value; ///< The actual value of get operations

    /// Attempts to get the value off the stack at idx as an Object
    Var(HSQUIRRELVM vm, SQInteger idx) {
        HSQOBJECT sqValue;
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, idx, &sqValue)));
        value = Object(sqValue, vm);
    }

    /// Called by Sqrat::PushVar to put an Object on the stack
    static void push(HSQUIRRELVM vm, const Object& value) {
        sq_pushobject(vm, value.GetObject());
    }

    static const char * getVarTypeName() { return "object"; }
    static bool check_type(HSQUIRRELVM /*vm*/, SQInteger /*idx*/) {
        return true;
    }
};

template<>
struct Var<Object&> : Var<Object> {Var(HSQUIRRELVM vm, SQInteger idx) : Var<Object>(vm, idx) {}};

template<>
struct Var<const Object&> : Var<Object> {Var(HSQUIRRELVM vm, SQInteger idx) : Var<Object>(vm, idx) {}};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Iterator for going over the slots in the object using Object::Next
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Object::iterator {
    friend class Object;

    const char* getName() {
        HSQOBJECT hKey = key.GetObject();
        return sq_objtostring(&hKey);
    }

    HSQOBJECT getKey() { return key.GetObject(); }
    HSQOBJECT getValue() { return value.GetObject(); }

private:
    Object    index, key, value;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to go through all the slots in an Object (same limitations as sq_next)
/// \param iter An iterator being used for going through the slots
/// \return Whether there is a next slot
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Object::Next(iterator& iter) const {
    sq_pushobject(vm, obj);
    sq_pushobject(vm, iter.index.GetObject());

    if (SQ_SUCCEEDED(sq_next(vm,-2))) {
        iter.index = Var<Object>(vm, -3).value;
        iter.key   = Var<Object>(vm, -2).value;
        iter.value = Var<Object>(vm, -1).value;

        sq_pop(vm, 4);
        return true;
    }
    else {
        sq_pop(vm, 2);
        return false;
    }
}

} // namespace Sqrat

#endif
