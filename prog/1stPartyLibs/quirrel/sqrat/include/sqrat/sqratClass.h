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

template<class C> struct InstanceToString;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Facilitates exposing a C++ class with no base class to Squirrel
///
/// \tparam C Class type to expose
/// \tparam A An allocator to use when instantiating and destroying class instances of this type in Squirrel
///
/// \remarks
/// DefaultAllocator is used if no allocator is specified. This should be sufficent for most classes,
/// but if specific behavior is desired, it can be overridden. If the class should not be instantiated from
/// Squirrel the NoConstructor allocator may be used. See NoCopy and CopyOnly too.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class C, class A = DefaultAllocator<C> >
class Class : public Object
{
private:

    static SQInteger cleanup_hook(SQUserPointer ptr, SQInteger size) {
        SQRAT_UNUSED(size);
        ClassData<C>** ud = reinterpret_cast<ClassData<C>**>(ptr);
        delete *ud;
        return 0;
    }

public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs the Class object
    ///
    /// A Class object doesnt do anything on its own.
    /// It must be told what methods and variables it contains.
    /// This is done using Class methods such as Class::Func and Class::Var.
    /// Then the Class must be exposed to Squirrel.
    /// This is usually done by calling TableBase::Bind on a RootTable with the Class.
    ///
    /// \param v           Squirrel virtual machine to create the Class for
    /// \param className   A necessarily unique name for the class that can appear in error messages
    /// \param createClass Should class type data be created? (almost always should be true - don't worry about it)
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Class(HSQUIRRELVM v, string && className, bool createClass = true) : Object(v) {
        if (createClass && !ClassType<C>::hasClassData(v)) {
            sq_pushregistrytable(v);
            sq_pushuserpointer(v, ClassesRegistryTable::slotKey());
            if (SQ_FAILED(sq_rawget(v, -2))) {
                sq_newtable(v);
                sq_pushuserpointer(v, ClassesRegistryTable::slotKey());
                sq_push(v, -2);
                sq_rawset(v, -4);
            }
            sq_pushuserpointer(v, ClassData<C>::type_id());
            ClassData<C>** ud = reinterpret_cast<ClassData<C>**>(sq_newuserdata(v, sizeof(ClassData<C>*)));
            *ud = new ClassData<C>;
            sq_setreleasehook(v, -1, &cleanup_hook);
            sq_rawset(v, -3);
            sq_pop(v, 2);

            ClassData<C>* cd = *ud;

            if (ClassType<C>::getStaticClassData().expired()) {
                cd->staticData.reset(new StaticClassData<C, void>);
                cd->staticData->copyFunc  = &A::Copy;
                cd->staticData->className = SQRAT_STD::move(className);
                cd->staticData->baseClass = NULL;

                ClassType<C>::getStaticClassData() = cd->staticData;
            } else {
                cd->staticData = ClassType<C>::getStaticClassData().lock();
            }

            HSQOBJECT& classObj = cd->classObj;
            sq_resetobject(&classObj);

            SQRAT_VERIFY(SQ_SUCCEEDED(sq_newclass(v, false)));
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(v, -1, &classObj)));
            sq_addref(v, &classObj); // must addref before the pop!
            sq_pop(v, 1);
            InitClass(cd);
        }
    }

    /// Gets the Squirrel object for this Class (copy)
    virtual HSQOBJECT GetObject() const {
        return ClassType<C>::getClassData(vm)->classObj;
    }

    /// Gets the Squirrel object for this Class (reference)
    virtual HSQOBJECT& GetObject() {
        return ClassType<C>::getClassData(vm)->classObj;
    }

public:

    /// Assigns a static class slot a value
    template<class V>
    Class& SetStaticValue(const SQChar* name, const V& val) {
        BindValue<V>(name, val, true);
        return *this;
    }

    /// Assigns a class slot a value
    template<class V>
    Class& SetValue(const SQChar* name, const V& val) {
        BindValue<V>(name, val, false);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a class variable of type <V>
    ///
    /// \remarks
    /// If V is not a pointer or reference, then it must have a default constructor.
    /// See Sqrat::Class::Prop to work around this requirement
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    Class& Var(const SQChar* name, V C::* var) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);

        // Add the getter
        BindAccessor(name, &var, sizeof(var), &sqDefaultGet<C, V>, cd->getTable);

        // Add the setter
        BindAccessor(name, &var, sizeof(var), &sqDefaultSet<C, V>, cd->setTable);

        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a class variable without a setter
    ///
    /// \remarks
    /// If V is not a pointer or reference, then it must have a default constructor.
    /// See Sqrat::Class::Prop to work around this requirement
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    Class& ConstVar(const SQChar* name, V C::* var) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);

        // Add the getter
        BindAccessor(name, &var, sizeof(var), &sqDefaultGet<C, V>, cd->getTable);

        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Bind a class static variable
    ///
    /// \remarks
    /// If V is not a pointer or reference, then it must have a default constructor.
    /// See Sqrat::Class::Prop to work around this requirement
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<class V>
    Class& StaticVar(const SQChar* name, V* var) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);

        // Add the getter
        BindAccessor(name, &var, sizeof(var), &sqStaticGet<C, V>, cd->getTable);

        // Add the setter
        BindAccessor(name, &var, sizeof(var), &sqStaticSet<C, V>, cd->setTable);

        return *this;
    }

    /// Binds a class property
    template<class F1, class F2>
    Class& Prop(const SQChar* name, F1 getMethod, F2 setMethod) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);

        if(getMethod != NULL) {
            // Add the getter
            BindAccessor(name, &getMethod, sizeof(getMethod), SqMemberFunc<C, F1>(), cd->getTable);
        }

        if(setMethod != NULL) {
            // Add the setter
            BindAccessor(name, &setMethod, sizeof(setMethod), SqMemberFunc<C, F2>(), cd->setTable);
        }

        return *this;
    }

    /// Binds a class property (using global functions instead of member functions)
    template<class F1, class F2>
    Class& GlobalProp(const SQChar* name, F1 getMethod, F2 setMethod) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);

        if(getMethod != NULL) {
            // Add the getter
            BindAccessor(name, &getMethod, sizeof(getMethod), SqMemberGlobalThunk<F1>(), cd->getTable);
        }

        if(setMethod != NULL) {
            // Add the setter
            BindAccessor(name, &setMethod, sizeof(setMethod), SqMemberGlobalThunk<F2>(), cd->setTable);
        }

        return *this;
    }

    /// Binds a read-only class property
    template<class F>
    Class& Prop(const SQChar* name, F getMethod) {
        // Add the getter
        BindAccessor(name, &getMethod, sizeof(getMethod), SqMemberFunc<C, F>(), ClassType<C>::getClassData(vm)->getTable);

        return *this;
    }

    Class& SquirrelProp(const SQChar* name, SQFUNCTION getMethod, const SQChar *docstring=nullptr) {
        // getter
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->getTable); // Push table
        sq_pushstring(vm, name, -1);
        sq_newclosure(vm, getMethod, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, 1, "x")));
        if (docstring)
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm, 1); // Pop table

        return *this;
    }

    Class& SquirrelProp(const SQChar* name, SQFUNCTION getMethod, SQFUNCTION setMethod, const SQChar *docstring=nullptr) {
        // getter
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->getTable); // Push table
        sq_pushstring(vm, name, -1);
        sq_newclosure(vm, getMethod, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, 1, "x")));
        if (docstring)
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm, 1); // Pop table

        // setter
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->setTable); // Push table
        sq_pushstring(vm, name, -1);
        sq_newclosure(vm, setMethod, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setparamscheck(vm, 2, "x.")));
        if (docstring)
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm, 1); // Pop table

        return *this;
    }

    /// Binds a read-only class property (using a global function instead of a member function)
    template<class F>
    Class& GlobalProp(const SQChar* name, F getMethod) {
        // Add the getter
        BindAccessor(name, &getMethod, sizeof(getMethod), SqMemberGlobalThunk<F>(), ClassType<C>::getClassData(vm)->getTable);

        return *this;
    }

    /// Binds a class function
    template<class F>
    Class& Func(const SQChar* name, F method, const SQChar *docstring=nullptr) { //-V1071
        BindFunc(name, method, SqMemberFunc<C, F>(), 1+SqGetArgCount<F>(), false, docstring);
        return *this;
    }

    /// Binds a global function as a class function
    template<class F>
    Class& GlobalFunc(const SQChar* name, F method, const SQChar *docstring=nullptr) { //-V1071
        BindFunc(name, method, SqMemberGlobalThunk<F>(), SqGetArgCount<F>(), false, docstring);
        return *this;
    }

    /// Binds a static class function
    template<class F>
    Class& StaticFunc(const SQChar* name, F method, const SQChar *docstring=nullptr) { //-V1071
        BindFunc(name, method, SqGlobalThunk<F>(), 1+SqGetArgCount<F>(), false, docstring);
        return *this;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Binds a Squirrel function as defined by the Squirrel documentation as a class function
    ///
    /// \remarks
    /// Inside of the function, the class instance the function was called with will be at index 1 on the
    /// stack and all arguments will be after that index in the order they were given to the function.
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Class& SquirrelFunc(const SQChar* name, SQFUNCTION func, SQInteger nparamscheck, const SQChar *typemask=nullptr,
                        const SQChar *docstring=nullptr, SQInteger nfreevars=0, const Object *freevars=nullptr, //-V1071
                        FunctionPurity purity=FunctionPurity::SideEffects) {
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->classObj);
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
        sq_pop(vm, 1); // pop table

        return *this;
    }

    TableBase& SquirrelFuncDeclString(SQFUNCTION func, const SQChar *function_decl,
                                      const SQChar *docstring=nullptr, SQInteger nfreevars=0, const Object *freevars=nullptr) {
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->classObj);
        for (SQInteger i=0; i<nfreevars; ++i)
            sq_pushobject(vm, freevars[i].GetObject());

        SQRAT_VERIFY(SQ_SUCCEEDED(sq_new_closure_slot_from_decl_string(vm, func, nfreevars, function_decl, docstring)));
        sq_pop(vm,1); // pop table
        return *this;
    }


    /// Gets a Function from a name in the Class (returns null if failed)
    Function GetFunction(const SQChar* name) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);
        HSQOBJECT funcObj;
        sq_pushobject(vm, cd->classObj);
        sq_pushstring(vm, name, -1);

        if(SQ_FAILED(sq_get_noerr(vm, -2))) {
            sq_pop(vm, 1);
            return Function();
        }
        SQObjectType value_type = sq_gettype(vm, -1);
        if (value_type != OT_CLOSURE && value_type != OT_NATIVECLOSURE && value_type != OT_CLASS) {
            sq_pop(vm, 2);
            return Function();
        }

        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &funcObj)));
        Function ret(vm, cd->classObj, funcObj); // must addref before the pop!
        sq_pop(vm, 2);
        return ret;
    }

protected:

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
        return ClassType<C>::CopyFunc()(vm, 1, other.value);
    }

    // Initialize the required data structure for the class
    void InitClass(ClassData<C>* cd) {
        cd->instances.reset(new InstancesMap<C>);

        // push the class
        sq_pushobject(vm, cd->classObj);

        // set the typetag of the class
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_settypetag(vm, -1, cd->staticData.get())));

        // add the default constructor
        this->BindConstructor(&A::New, 0);

        // add the set table (static)
        HSQOBJECT& setTable = cd->setTable;
        sq_resetobject(&setTable);
        sq_pushstring(vm, _SC("__setTable"), -1);
        sq_newtable(vm);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &setTable)));
        sq_addref(vm, &setTable);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, true)));

        // add the get table (static)
        HSQOBJECT& getTable = cd->getTable;
        sq_resetobject(&getTable);
        sq_pushstring(vm, _SC("__getTable"), -1);
        sq_newtable(vm);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &getTable)));
        sq_addref(vm, &getTable);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, true)));

        // override _set
        sq_pushstring(vm, _SC("_set"), -1);
        sq_pushobject(vm, setTable); // Push the set table as a free variable
        sq_newclosure(vm, &sqVarSet, 1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // override _get
        sq_pushstring(vm, _SC("_get"), -1);
        sq_pushobject(vm, getTable); // Push the get table as a free variable
        sq_newclosure(vm, &sqVarGet, 1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add weakref (apparently not provided by default)
        sq_pushstring(vm, _SC("weakref"), -1);
        sq_newclosure(vm, &Class::ClassWeakref, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _typeof
        sq_pushstring(vm, _SC("_typeof"), -1);
        sq_newclosure(vm, &Class::ClassTypeof, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _cloned
        sq_pushstring(vm, _SC("_cloned"), -1);
        sq_newclosure(vm, &Class::ClassCloned, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _tostring
        sq_pushstring(vm, _SC("_tostring"), -1);
        sq_newclosure(vm, InstanceToString<C>::Format, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // pop the class
        sq_pop(vm, 1);
    }

    // Helper function used to bind getters and setters
    inline void BindAccessor(const SQChar* name, void* var, size_t varSize, SQFUNCTION func, HSQOBJECT table) {
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

    // constructor binding
    Class& BindConstructor(SQFUNCTION method, SQInteger nParams) {
        // assert would be useful if there wasn't automatically registered default constructor
        //SQRAT_ASSERT(!Table(ClassType<C>::getClassData(vm)->classObj, vm).RawHasKey(_SC("constructor")));

        sq_pushobject(vm, ClassType<C>::getClassData(vm)->classObj);

        sq_pushstring(vm, _SC("constructor"), 11);
        sq_newclosure(vm, method, 0);
        sq_setparamscheck(vm, nParams+1, NULL);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        sq_pop(vm, 1);
        return *this;
    }

public:

    Class& Ctor() {
        return BindConstructor(A::iNew, 0);
    }

    template<class...Arg>
    Class& Ctor() {
        return BindConstructor(A::template iNew<Arg...>, sizeof...(Arg));
    }

    Class& SquirrelCtor(SQFUNCTION func, SQInteger nparamscheck=0, const SQChar *typemask=nullptr,
                        const SQChar *docstring=nullptr) {
        sq_pushobject(vm, ClassType<C>::getClassData(vm)->classObj);
        sq_pushstring(vm, _SC("constructor"), 11);
        sq_newclosure(vm, func, 0);
        sq_setparamscheck(vm, nparamscheck, typemask);
        if (docstring)
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_setnativeclosuredocstring(vm, -1, docstring)));
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));
        sq_pop(vm, 1);
        return *this;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Facilitates exposing a C++ class with a base class to Squirrel
///
/// \tparam C Class type to expose
/// \tparam B Base class type (must already be bound)
/// \tparam A An allocator to use when instantiating and destroying class instances of this type in Squirrel
///
/// \remarks
/// Classes in Squirrel are single-inheritance only, and as such Sqrat only allows for single inheritance as well.
///
/// \remarks
/// DefaultAllocator is used if no allocator is specified. This should be sufficent for most classes,
/// but if specific behavior is desired, it can be overridden. If the class should not be instantiated from
/// Squirrel the NoConstructor allocator may be used. See NoCopy and CopyOnly too.
///
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class C, class B, class A = DefaultAllocator<C> >
class DerivedClass : public Class<C, A>
{
  using Class<C, A>::vm;

private:

    static SQInteger cleanup_hook(SQUserPointer ptr, SQInteger size) {
        SQRAT_UNUSED(size);
        ClassData<C>** ud = reinterpret_cast<ClassData<C>**>(ptr);
        delete *ud;
        return 0;
    }

public:

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Constructs the DerivedClass object
    ///
    /// A DerivedClass object doesnt do anything on its own.
    /// It must be told what methods and variables it contains.
    /// This is done using Class methods such as Class::Func and Class::Var.
    /// Then the DerivedClass must be exposed to Squirrel.
    /// This is usually done by calling TableBase::Bind on a RootTable with the DerivedClass.
    ///
    /// \param v         Squirrel virtual machine to create the DerivedClass for
    /// \param className A necessarily unique name for the class that can appear in error messages
    ///
    /// \remarks
    /// You MUST bind the base class fully before constructing a derived class.
    ///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DerivedClass(HSQUIRRELVM v, string && className) : Class<C, A>(v, string(), false) {
        if (!ClassType<C>::hasClassData(v)) {
            sq_pushregistrytable(v);
            sq_pushuserpointer(v, ClassesRegistryTable::slotKey());
            if (SQ_FAILED(sq_rawget(v, -2))) {
                sq_newtable(v);
                sq_pushuserpointer(v, ClassesRegistryTable::slotKey());
                sq_push(v, -2);
                sq_rawset(v, -4);
            }
            sq_pushuserpointer(v, ClassData<C>::type_id());
            ClassData<C>** ud = reinterpret_cast<ClassData<C>**>(sq_newuserdata(v, sizeof(ClassData<C>*)));
            *ud = new ClassData<C>;
            sq_setreleasehook(v, -1, &cleanup_hook);
            sq_rawset(v, -3);
            sq_pop(v, 2);

            ClassData<B>* bd = ClassType<B>::getClassData(v);
            ClassData<C>* cd = *ud;

            if (ClassType<C>::getStaticClassData().expired()) {
                cd->staticData.reset(new StaticClassData<C, B>);
                cd->staticData->copyFunc  = &A::Copy;
                cd->staticData->className = SQRAT_STD::move(className);
                cd->staticData->baseClass = bd->staticData.get();

                ClassType<C>::getStaticClassData() = cd->staticData;
            } else {
                cd->staticData = ClassType<C>::getStaticClassData().lock();
            }

            HSQOBJECT& classObj = cd->classObj;
            sq_resetobject(&classObj);

            sq_pushobject(v, bd->classObj);
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_newclass(v, true)));
            SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(v, -1, &classObj)));
            sq_addref(v, &classObj); // must addref before the pop!
            sq_pop(v, 1);
            InitDerivedClass(cd, bd);
        }
    }

protected:

    void InitDerivedClass(ClassData<C>* cd, ClassData<B>* bd) {
        cd->instances.reset(new InstancesMap<C>);

        // push the class
        sq_pushobject(vm, cd->classObj);

        // set the typetag of the class
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_settypetag(vm, -1, cd->staticData.get())));

        // add the default constructor
        this->BindConstructor(&A::New, 0);

        // clone the base classes set table (static)
        HSQOBJECT& setTable = cd->setTable;
        sq_resetobject(&setTable);
        sq_pushobject(vm, bd->setTable);
        sq_pushstring(vm, _SC("__setTable"), -1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_clone(vm, -2)));
        sq_remove(vm, -3);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &setTable)));
        sq_addref(vm, &setTable);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, true)));

        // clone the base classes get table (static)
        HSQOBJECT& getTable = cd->getTable;
        sq_resetobject(&getTable);
        sq_pushobject(vm, bd->getTable);
        sq_pushstring(vm, _SC("__getTable"), -1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_clone(vm, -2)));
        sq_remove(vm, -3);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &getTable)));
        sq_addref(vm, &getTable);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, true)));

        // override _set
        sq_pushstring(vm, _SC("_set"), -1);
        sq_pushobject(vm, setTable); // Push the set table as a free variable
        sq_newclosure(vm, sqVarSet, 1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // override _get
        sq_pushstring(vm, _SC("_get"), -1);
        sq_pushobject(vm, getTable); // Push the get table as a free variable
        sq_newclosure(vm, sqVarGet, 1);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add weakref (apparently not provided by default)
        sq_pushstring(vm, _SC("weakref"), -1);
        sq_newclosure(vm, &Class<C, A>::ClassWeakref, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _typeof
        sq_pushstring(vm, _SC("_typeof"), -1);
        sq_newclosure(vm, &Class<C, A>::ClassTypeof, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _cloned
        sq_pushstring(vm, _SC("_cloned"), -1);
        sq_newclosure(vm, &Class<C, A>::ClassCloned, 0);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_newslot(vm, -3, false)));

        // add _tostring
        sq_pushstring(vm, _SC("_tostring"), -1);
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
