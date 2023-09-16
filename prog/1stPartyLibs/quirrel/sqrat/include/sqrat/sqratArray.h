// Sqrat: altered version by Gaijin Games KFT
// SqratArray: Array Binding
//

//
// Copyright 2011 Alston Chen
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
#if !defined(_SQRAT_ARRAY_H_)
#define _SQRAT_ARRAY_H_

#include <squirrel.h>
#include <string.h>

#include "sqratObject.h"
#include "sqratFunction.h"
#include "sqratGlobalMethods.h"

namespace Sqrat {

class ArrayBase : public Object {
public:
    ArrayBase() {
    }

    ArrayBase(HSQUIRRELVM v) : Object(v) {
    }

    ArrayBase(const Object& obj) : Object(obj) {
    }

    ArrayBase(HSQOBJECT o, HSQUIRRELVM v) : Object(o, v) {
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a Table or Class to the specific element of Array (can be used to facilitate namespaces)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Bind(const SQInteger index, Object& object) {
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);
        sq_pushobject(vm, object.GetObject());
        sq_set(vm, -3);
        sq_pop(vm,1); // pop array
    }


    template<class V>
    ArrayBase& SetValue(const SQInteger index, const V& val) {
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);
        PushVar(vm, val);
        sq_set(vm, -3);
        sq_pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets an index in the Array to a specific instance (like a reference)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    ArrayBase& SetInstance(const SQInteger index, V* val) {
        BindInstance<V>(index, val, false);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Sets an index in the Array to a specific function
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class F>
    ArrayBase& Func(const SQInteger index, F method) {
        BindFunc<F>(index, method, SqGlobalThunk<F>(), 1+SqGetArgCount<F>());
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Returns the element at a given index
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    T GetValue(int index)
    {
        sq_pushobject(vm, obj);
        sq_pushinteger(vm, index);
        if (SQ_FAILED(sq_get_noerr(vm, -2))) {
            sq_pop(vm, 1);
            SQRAT_ASSERT(0); // Ensure that index is valid before calling this method
            return T();
        }

        Var<T> element(vm, -1);
        sq_pop(vm, 2);
        return element.value;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Gets a Function from an index in the Array
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Function GetFunction(const SQInteger index) {
        HSQOBJECT funcObj;
        sq_pushobject(vm, GetObject());
        sq_pushinteger(vm, index);
        if(SQ_FAILED(sq_get_noerr(vm, -2))) {
            sq_pop(vm, 1);
            return Function();
        }
        SQObjectType value_type = sq_gettype(vm, -1);
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE) {
            sq_pop(vm, 2);
            return Function();
        }

        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &funcObj)));
        Function ret(vm, GetObject(), funcObj); // must addref before the pop!
        sq_pop(vm, 2);
        return ret;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Fills a C array with the elements of the Array
    ///
    /// \param array C array to be filled
    /// \param size  The amount of elements to fill the C array with
    ///
    /// \tparam T Type of elements (fails if any elements in Array are not of this type)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    void GetArray(T* array, int size)
    {
        HSQOBJECT value = GetObject();
        sq_pushobject(vm, value);
        // Before calling this method ensure that size provided size matches array's one
        SQRAT_ASSERTF(size==sq_getsize(vm, -1), "array size mismatch (%d vs %d)", size, sq_getsize(vm, -1));

        sq_pushnull(vm);
        SQInteger i;
        while (SQ_SUCCEEDED(sq_next(vm, -2))) {
            sq_getinteger(vm, -2, &i);
            if (i >= size) break;
            Var<const T&> element(vm, -1); // TODO: handle error
            sq_pop(vm, 2);
            array[i] = element.value;
        }
        sq_pop(vm, 2); // pops the null iterator and the array object
    }

    template<class V>
    ArrayBase& Append(const V& val) {
        sq_pushobject(vm, GetObject());
        PushVar(vm, val);
        sq_arrayappend(vm, -2);
        sq_pop(vm,1); // pop array
        return *this;
    }

    template<class V>
    ArrayBase& Append(V* val) {
        sq_pushobject(vm, GetObject());
        PushVar(vm, val);
        sq_arrayappend(vm, -2);
        sq_pop(vm,1); // pop array
        return *this;
    }

    template<class V>
    ArrayBase& Insert(const SQInteger destpos, const V& val) {
        sq_pushobject(vm, GetObject());
        PushVar(vm, val);
        sq_arrayinsert(vm, -2, destpos);
        sq_pop(vm,1); // pop array
        return *this;
    }

    template<class V>
    ArrayBase& Insert(const SQInteger destpos, V* val) {
        sq_pushobject(vm, GetObject());
        PushVar(vm, val);
        sq_arrayinsert(vm, -2, destpos);
        sq_pop(vm,1); // pop array
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Removes the last element from the Array and returns it (returns null if failed)
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Object Pop() {
        HSQOBJECT slotObj;
        sq_pushobject(vm, GetObject());
        if(SQ_FAILED(sq_arraypop(vm, -1, true))) {
            sq_pop(vm, 1);
            return Object(); // Return a NULL object
        } else {
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &slotObj)));
            Object ret(slotObj, vm);
            sq_pop(vm, 2);
            return ret;
        }
    }

    ArrayBase& Remove(const SQInteger itemidx) {
        sq_pushobject(vm, GetObject());
        sq_arrayremove(vm, -1, itemidx);
        sq_pop(vm,1); // pop array
        return *this;
    }

    ArrayBase& Resize(const SQInteger newsize) {
        sq_pushobject(vm, GetObject());
        sq_arrayresize(vm, -1, newsize);
        sq_pop(vm,1); // pop array
        return *this;
    }

    ArrayBase& Reverse() {
        sq_pushobject(vm, GetObject());
        sq_arrayreverse(vm, -1);
        sq_pop(vm,1); // pop array
        return *this;
    }

    SQInteger Length() const {
        sq_pushobject(vm, obj);
        SQInteger r = sq_getsize(vm, -1);
        sq_pop(vm, 1);
        return r;
    }

    bool Clear() {
        sq_pushobject(vm, GetObject());
        bool ok = SQ_SUCCEEDED(sq_clear(vm, -1));
        sq_pop(vm, 1); // pop array
        return ok;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents an array in Squirrel
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Array : public ArrayBase {
public:
    Array() {
    }

    Array(HSQUIRRELVM v, const SQInteger size = 0) : ArrayBase(v) {
        SQRAT_ASSERT(v);
        sq_newarray(vm, size);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm,-1,&obj)));
        sq_addref(vm, &obj);
        sq_pop(vm,1);
    }

    Array(const Object& obj) : ArrayBase(obj) {
    }

    Array(HSQOBJECT o, HSQUIRRELVM v) : ArrayBase(o, v) {
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Array instances to and from the stack as references (arrays are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<Array> {

    Array value; ///< The actual value of get operations

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Attempts to get the value off the stack at idx as an Array
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Var(HSQUIRRELVM vm, SQInteger idx) {
        HSQOBJECT obj;
        sq_resetobject(&obj);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm,idx,&obj)));
        value = Array(obj, vm);
        SQObjectType value_type = sq_gettype(vm, idx);
        if (value_type != OT_ARRAY && value_type != OT_NULL) {
            SQRAT_ASSERTF(0, FormatTypeError(vm, idx, _SC("array")).c_str());
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Called by Sqrat::PushVar to put an Array reference on the stack
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void push(HSQUIRRELVM vm, const Array& value) {
        HSQOBJECT obj;
        sq_resetobject(&obj);
        obj = value.GetObject();
        sq_pushobject(vm,obj);
    }

    static const SQChar * getVarTypeName() { return _SC("array"); }
    static bool check_type(HSQUIRRELVM vm, SQInteger idx) {
        return sq_gettype(vm, idx) == OT_ARRAY || sq_gettype(vm, idx) == OT_NULL;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Array instances to and from the stack as references (arrays are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<Array&> : Var<Array> {Var(HSQUIRRELVM vm, SQInteger idx) : Var<Array>(vm, idx) {}};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used to get and push Array instances to and from the stack as references (arrays are always references in Squirrel)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<>
struct Var<const Array&> : Var<Array> {Var(HSQUIRRELVM vm, SQInteger idx) : Var<Array>(vm, idx) {}};

}

#endif
