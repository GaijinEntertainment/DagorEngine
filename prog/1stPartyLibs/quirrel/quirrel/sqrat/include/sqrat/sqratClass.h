// Sqrat: altered version by Gaijin Games KFT
// SqratClass: Class Binding
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
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
//
//    3. This notice may not be removed or altered from any source
//    distribution.
//

#pragma once
#if !defined(_SQRAT_CLASS_H_)
#define _SQRAT_CLASS_H_

#include <squirrel.h>
#include <string.h>

#include "sqratObject.h"
#include "sqratClassType.h"
#include "sqratMemberMethods.h"
#include "sqratAllocator.h"
#include "sqratTypes.h"


namespace Sqrat
{

// Maps C++ types to SQNFT_* constants for sq_registernativefield.
template<typename T> struct NativeFieldTypeFor;
template<> struct NativeFieldTypeFor<float>    { enum { value = SQNFT_FLOAT32 }; };
template<> struct NativeFieldTypeFor<double>   { enum { value = SQNFT_FLOAT64 }; };
template<> struct NativeFieldTypeFor<int32_t>  { enum { value = SQNFT_INT32 }; };
template<> struct NativeFieldTypeFor<int64_t>  { enum { value = SQNFT_INT64 }; };
template<> struct NativeFieldTypeFor<bool>     { enum { value = SQNFT_BOOL }; };

template<class C> struct InstanceToString;


/// Facilitates exposing a C++ class with no base class to Squirrel
///
/// \tparam C Class type to expose
/// \tparam A An allocator to use when instantiating and destroying class instances of this type in Squirrel
///
/// \remarks
/// DefaultAllocator is used if no allocator is specified. This should be sufficent for most classes,
/// but if specific behavior is desired, it can be overridden. If the class should not be instantiated from
/// Squirrel the NoConstructor allocator may be used. See NoCopy and CopyOnly too.
template<class C, class A = DefaultAllocator<C> >
class Class
{
protected:
  HSQUIRRELVM vm = nullptr;

public:

    /// \param v           Squirrel virtual machine to create the Class for
    /// \param className   A necessarily unique name for the class that can appear in error messages
    /// \param createClass Should class type data be created? (almost always should be true - don't worry about it)
    Class(HSQUIRRELVM v, string && className, bool createClass = true) : vm(v) {
        if (createClass && !ClassType<C>::hasClassData(v)) {
            // Register static TypeTag (once per type, first VM wins)
            TypeTag<C>* tag = TypeTag<C>::get();
            TypeTagBase::allTypeTags().insert(tag);

            // Create per-VM ClassData
            ClassData<C>* cd = new ClassData<C>;
            cd->copyFunc = &A::Copy;
            cd->className = className;

            if (ClassData<C>::staticClassName.empty()) {
                ClassData<C>::staticClassName = className;
                tag->className = ClassData<C>::staticClassName.c_str();
            }

            ClassType<C>::setClassData(v, cd);
            SqratCleanup::getOrCreate(v)->add<C>(cd);

            HSQOBJECT& classObj = cd->classObj; //-V522
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_newclass(v, false)));
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(v, -1, &classObj)));
            sq_addref(v, &classObj); // must addref before the pop!
            sq_pop(v, 1);
            InitClass(cd);
        }
    }

    /// Gets the Squirrel object for this Class (copy)
    HSQOBJECT GetObject() const {
        return ClassType<C>::getClassData(vm)->classObj; //-V522
    }

    /// Gets the Squirrel object for this Class (reference)
    HSQOBJECT& GetObject() {
        return ClassType<C>::getClassData(vm)->classObj; //-V522
    }

public:

    /// Binds a class variable of type V (getter + setter)
    template<class V>
    Class& Var(const char* name, V C::* var) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);
        BindAccessor(name, &var, sizeof(var), &sqDefaultGet<C, V>, cd->getTable); //-V522
        BindAccessor(name, &var, sizeof(var), &sqDefaultSet<C, V>, cd->setTable); //-V522
        return *this;
    }

    /// Binds a class variable without a setter (read-only)
    template<class V>
    Class& ConstVar(const char* name, V C::* var) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);
        BindAccessor(name, &var, sizeof(var), &sqDefaultGet<C, V>, cd->getTable); //-V522
        return *this;
    }

    /// Binds a native field for direct VM access (no metamethod dispatch).
    /// Only works for primitive types (float, double, int32_t, int64_t, bool)
    /// on classes with inline userdata.
    template<class V>
    Class& NativeVar(const char* name, V C::* member) {
        static_assert(sizeof(C) <= 256 && alignof(C) <= SQ_ALIGNMENT,
                      "NativeVar requires type to fit in inline userdata");
        ClassData<C>* cd = ClassType<C>::getClassData(vm);
        sq_pushobject(vm, cd->classObj); //-V522
        alignas(C) char buf[sizeof(C)] = {};
        C* dummy = reinterpret_cast<C*>(buf);
        SQInteger offset = (SQInteger)((const char*)&(dummy->*member) - (const char*)dummy);
        SQRESULT res = sq_registernativefield(vm, -1, name, offset, NativeFieldTypeFor<V>::value);
        SQRAT_UNUSED(res);
        SQRAT_ASSERTF(SQ_SUCCEEDED(res),
            "NativeVar('%s') failed: call UseInlineUserdata() before registering native fields", name);
        sq_pop(vm, 1);
        return *this;
    }

    template<class V>
    Class& StaticVar(const char* name, V* var) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);
        BindAccessor(name, &var, sizeof(var), &sqStaticGet<C, V>, cd->getTable); //-V522
        BindAccessor(name, &var, sizeof(var), &sqStaticSet<C, V>, cd->setTable); //-V522
        return *this;
    }

    template<class F1, class F2>
    Class& Prop(const char* name, F1 getMethod, F2 setMethod) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);
        if(getMethod != NULL)
            BindAccessor(name, &getMethod, sizeof(getMethod), SqMemberFunc<C, F1>(), cd->getTable); //-V522
        if(setMethod != NULL)
            BindAccessor(name, &setMethod, sizeof(setMethod), SqMemberFunc<C, F2>(), cd->setTable); //-V522
        return *this;
    }

    template<class F1, class F2>
    Class& GlobalProp(const char* name, F1 getMethod, F2 setMethod) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);
        if(getMethod != NULL)
            BindAccessor(name, &getMethod, sizeof(getMethod), SqMemberGlobalThunk<F1>(), cd->getTable); //-V522
        if(setMethod != NULL)
            BindAccessor(name, &setMethod, sizeof(setMethod), SqMemberGlobalThunk<F2>(), cd->setTable); //-V522
        return *this;
    }

    template<class F>
    Class& Prop(const char* name, F getMethod) {
        BindAccessor(name, &getMethod, sizeof(getMethod), SqMemberFunc<C, F>(), ClassType<C>::getClassData(vm)->getTable); //-V522
        return *this;
    }

    Class& SquirrelProp(const char* name, SQFUNCTION getMethod, const char *docstring=nullptr) {
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->getTable); //-V522
        sq_pushstring(vm, name, -1);
        sq_newclosure(vm, getMethod, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, 1, "x")));
        if (docstring)
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm, 1);
        return *this;
    }

    Class& SquirrelProp(const char* name, SQFUNCTION getMethod, SQFUNCTION setMethod, const char *docstring=nullptr) {
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->getTable); //-V522
        sq_pushstring(vm, name, -1);
        sq_newclosure(vm, getMethod, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, 1, "x")));
        if (docstring)
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm, 1);

        sq_pushobject(vm, ClassType<C>::getClassData(vm)->setTable);
        sq_pushstring(vm, name, -1);
        sq_newclosure(vm, setMethod, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, 2, "x.")));
        if (docstring)
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm, 1);
        return *this;
    }

    /// Binds a read-only class property (using a global function instead of a member function)
    template<class F>
    Class& GlobalProp(const char* name, F getMethod) {
        BindAccessor(name, &getMethod, sizeof(getMethod), SqMemberGlobalThunk<F>(), ClassType<C>::getClassData(vm)->getTable); //-V522
        return *this;
    }

    /// Binds a class function
    template<class F>
    Class& Func(const char* name, F method, const char *docstring=nullptr) { //-V1071
        BindFunc(name, method, SqMemberFunc<C, F>(), 1+SqGetArgCount<F>(), false, docstring);
        return *this;
    }

    /// Binds a global function as a class function
    template<class F>
    Class& GlobalFunc(const char* name, F method, const char *docstring=nullptr) { //-V1071
        BindFunc(name, method, SqMemberGlobalThunk<F>(), SqGetArgCount<F>(), false, docstring);
        return *this;
    }

    /// Binds a static class function
    template<class F>
    Class& StaticFunc(const char* name, F method, const char *docstring=nullptr) { //-V1071
        BindFunc(name, method, SqGlobalThunk<F>(), 1+SqGetArgCount<F>(), false, docstring);
        return *this;
    }

    /// Binds a raw Squirrel function as a class function.
    /// The class instance will be at index 1, arguments after that.
    Class& SquirrelFunc(const char* name, SQFUNCTION func, SQInteger nparamscheck, const char *typemask=nullptr,
                        const char *docstring=nullptr, SQInteger nfreevars=0, const Object *freevars=nullptr, //-V1071
                        FunctionPurity purity=FunctionPurity::SideEffects) {
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->classObj); //-V522
        sq_pushstring(vm, name, -1);
        for (SQInteger i=0; i<nfreevars; ++i)
            sq_pushobject(vm, freevars[i].GetObject());
        sq_newclosure(vm, func, nfreevars);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, nparamscheck, typemask)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosurename(vm, -1, name)));
        if (purity == FunctionPurity::Pure)
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_mark_pure_inplace(vm, -1)));
        if (docstring)
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm, 1);
        return *this;
    }

    Class& SquirrelFuncDeclString(SQFUNCTION func, const char *function_decl,
                                  const char *docstring=nullptr, SQInteger nfreevars=0, const Object *freevars=nullptr) {
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->classObj); //-V522
        for (SQInteger i=0; i<nfreevars; ++i)
            sq_pushobject(vm, freevars[i].GetObject());
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_new_closure_slot_from_decl_string(vm, func, nfreevars, function_decl, docstring)));
        sq_pop(vm,1);
        return *this;
    }

    /// Gets a Function from a name in the Class (returns null if failed)
    Function GetFunction(const char* name) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);
        HSQOBJECT funcObj;
        sq_pushobject(vm, cd->classObj); //-V522
        sq_pushstring(vm, name, -1);
        if(SQ_FAILED(sq_get(vm, -2))) {
            sq_pop(vm, 1);
            return Function();
        }
        SQObjectType value_type = sq_gettype(vm, -1);
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE && value_type != OT_CLASS) {
            sq_pop(vm, 2);
            return Function();
        }
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &funcObj)));
        Function ret(vm, cd->classObj, funcObj);
        sq_pop(vm, 2);
        return ret;
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
      sq_pop(vm,1);
    }

    static SQInteger ClassWeakref(HSQUIRRELVM vm) {
        sq_weakref(vm, -1);
        return 1;
    }

    static SQInteger ClassTypeof(HSQUIRRELVM vm) {
        sq_pushstring(vm, ClassType<C>::ClassName().c_str(), -1);
        return 1;
    }

    static SQInteger ClassCloned(HSQUIRRELVM vm) {
        Sqrat::Var<const C*> other(vm, 2);
        return ClassType<C>::CopyFunc(vm)(vm, 1, other.value);
    }

    // Initialize the required data structure for the class
    void InitClass(ClassData<C>* cd) {
        // push the class
        sq_pushobject(vm, cd->classObj);

        // set the typetag to static TypeTag (VM-independent type identity)
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_settypetag(vm, -1, TypeTag<C>::get())));

        // add the default constructor
        this->BindConstructor(&A::New, 0);

        // add the set table (static)
        HSQOBJECT& setTable = cd->setTable;
        sq_pushstring(vm, "__setTable", -1);
        sq_newtable(vm);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &setTable)));
        sq_addref(vm, &setTable);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, true)));

        // add the get table (static)
        HSQOBJECT& getTable = cd->getTable;
        sq_pushstring(vm, "__getTable", -1);
        sq_newtable(vm);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &getTable)));
        sq_addref(vm, &getTable);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, true)));

        // override _set
        sq_pushstring(vm, "_set", -1);
        sq_pushobject(vm, setTable); // Push the set table as a free variable
        sq_newclosure(vm, &sqVarSet, 1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // override _get
        sq_pushstring(vm, "_get", -1);
        sq_pushobject(vm, getTable); // Push the get table as a free variable
        sq_newclosure(vm, &sqVarGet, 1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add weakref
        sq_pushstring(vm, "weakref", -1);
        sq_newclosure(vm, &Class::ClassWeakref, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _typeof
        sq_pushstring(vm, "_typeof", -1);
        sq_newclosure(vm, &Class::ClassTypeof, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _cloned
        sq_pushstring(vm, "_cloned", -1);
        sq_newclosure(vm, &Class::ClassCloned, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _tostring
        sq_pushstring(vm, "_tostring", -1);
        sq_newclosure(vm, InstanceToString<C>::Format, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // pop the class
        sq_pop(vm, 1);
    }

    inline void BindAccessor(const char* name, void* var, size_t varSize, SQFUNCTION func, HSQOBJECT table) {
        // Push the get or set table
        sq_pushobject(vm, table);
        sq_pushstring(vm, name, -1);

        // Push the variable offset as a free variable
        SQUserPointer varPtr = sq_newuserdata(vm, static_cast<SQUnsignedInteger>(varSize));
        memcpy(varPtr, var, varSize);

        // Create the accessor function
        sq_newclosure(vm, func, 1);

        // Add the accessor to the table
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // Pop get/set table
        sq_pop(vm, 1);
    }

    Class& BindConstructor(SQFUNCTION method, SQInteger nParams) {
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->classObj); //-V522
        sq_pushstring(vm, "constructor", 11);
        sq_newclosure(vm, method, 0);
        sq_setparamscheck(vm, nParams+1, NULL);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm, 1);
        return *this;
    }

public:

    /// Enable inline userdata for this class. Instances will embed the C++ object
    /// directly in SQInstance memory, avoiding a heap allocation. Only for small types.
    /// Must be called before any instances are created (before the class is locked).
    Class& UseInlineUserdata() {
        static_assert(sizeof(C) <= 256 && alignof(C) <= SQ_ALIGNMENT,
            "Type is too large or has non-standard alignment for inline userdata");
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->classObj); //-V522
        sq_setclassudsize(vm, -1, sizeof(C));
        sq_pop(vm, 1);
        return *this;
    }

    Class& Ctor() {
        return BindConstructor(A::iNew, 0);
    }

    template<class...Arg>
    Class& Ctor() {
        return BindConstructor(A::template iNew<Arg...>, sizeof...(Arg));
    }

    Class& SquirrelCtor(SQFUNCTION func, SQInteger nparamscheck=0, const char *typemask=nullptr,
                        const char *docstring=nullptr) {
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->classObj); //-V522
        sq_pushstring(vm, "constructor", 11);
        sq_newclosure(vm, func, 0);
        sq_setparamscheck(vm, nparamscheck, typemask);
        if (docstring)
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm, 1);
        return *this;
    }
};


/// Facilitates exposing a C++ class with a base class to Squirrel
///
/// \tparam C Class type to expose
/// \tparam B Base class type (must already be bound)
/// \tparam A An allocator to use when instantiating and destroying class instances of this type in Squirrel
///
/// \remarks
/// Classes in Squirrel are single-inheritance only, and as such Sqrat only allows for single inheritance as well.
/// You MUST bind the base class fully before constructing a derived class.
template<class C, class B, class A = DefaultAllocator<C> >
class DerivedClass : public Class<C, A>
{
  using Class<C, A>::vm;

public:

    DerivedClass(HSQUIRRELVM v, string && className) : Class<C, A>(v, string(), false) {
        if (!ClassType<C>::hasClassData(v)) {
            // Initialize TypeTag inheritance chain (once per type)
            TypeTag<C>* tag = TypeTag<C>::get();
            tag->base = TypeTag<B>::get();
            tag->castToBase = [](SQUserPointer p) -> SQUserPointer {
                return static_cast<B*>(static_cast<C*>(p));
            };
            TypeTag<B>::get()->hasDerived = true;
            TypeTagBase::allTypeTags().insert(tag);

            // Create per-VM ClassData
            ClassData<C>* cd = new ClassData<C>;
            cd->copyFunc = &A::Copy;
            cd->className = className;

            if (ClassData<C>::staticClassName.empty()) {
                ClassData<C>::staticClassName = className;
                tag->className = ClassData<C>::staticClassName.c_str();
            }

            ClassData<B>* bd = ClassType<B>::getClassData(v);

            ClassType<C>::setClassData(v, cd);
            SqratCleanup::getOrCreate(v)->add<C>(cd);

            HSQOBJECT& classObj = cd->classObj;
            sq_pushobject(v, bd->classObj); //-V522
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_newclass(v, true)));
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(v, -1, &classObj)));
            sq_addref(v, &classObj); // must addref before the pop!
            sq_pop(v, 1);
            InitDerivedClass(cd, bd);
        }
    }

protected:

    void InitDerivedClass(ClassData<C>* cd, ClassData<B>* bd) {
        // push the class
        sq_pushobject(vm, cd->classObj);

        // set the typetag to static TypeTag (VM-independent type identity)
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_settypetag(vm, -1, TypeTag<C>::get())));

        // add the default constructor
        this->BindConstructor(&A::New, 0);

        // clone the base classes set table (static)
        HSQOBJECT& setTable = cd->setTable;
        sq_pushobject(vm, bd->setTable);
        sq_pushstring(vm, "__setTable", -1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_clone(vm, -2)));
        sq_remove(vm, -3);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &setTable)));
        sq_addref(vm, &setTable);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, true)));

        // clone the base classes get table (static)
        HSQOBJECT& getTable = cd->getTable;
        sq_pushobject(vm, bd->getTable);
        sq_pushstring(vm, "__getTable", -1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_clone(vm, -2)));
        sq_remove(vm, -3);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &getTable)));
        sq_addref(vm, &getTable);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, true)));

        // override _set
        sq_pushstring(vm, "_set", -1);
        sq_pushobject(vm, setTable); // Push the set table as a free variable
        sq_newclosure(vm, sqVarSet, 1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // override _get
        sq_pushstring(vm, "_get", -1);
        sq_pushobject(vm, getTable); // Push the get table as a free variable
        sq_newclosure(vm, sqVarGet, 1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add weakref
        sq_pushstring(vm, "weakref", -1);
        sq_newclosure(vm, &Class<C, A>::ClassWeakref, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _typeof
        sq_pushstring(vm, "_typeof", -1);
        sq_newclosure(vm, &Class<C, A>::ClassTypeof, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _cloned
        sq_pushstring(vm, "_cloned", -1);
        sq_newclosure(vm, &Class<C, A>::ClassCloned, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _tostring
        sq_pushstring(vm, "_tostring", -1);
        sq_newclosure(vm, InstanceToString<C>::Format, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // pop the class
        sq_pop(vm, 1);
    }
};


template<class C>
struct InstanceToString {
  static SQInteger Format(HSQUIRRELVM vm) {
    return ClassType<C>::ToString(vm);
  }
};



}

#endif
