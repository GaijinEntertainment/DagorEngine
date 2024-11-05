// Sqrat: altered version by Gaijin Games KFT
// SqratClassType: Type Translators
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
#if !defined(_SQRAT_CLASSTYPE_H_)
#define _SQRAT_CLASSTYPE_H_

#include <squirrel.h>
#include "sqratUtil.h"

namespace Sqrat
{


// The copy function for a class
typedef SQInteger (*COPYFUNC)(HSQUIRRELVM, SQInteger, const void*);

struct AbstractStaticClassData;

// Lookup static class data by type_info rather than a template because C++ cannot export generic templates
struct IntPtrHash { size_t operator()(const void *p) const { return uintptr_t(p) >> 2; } };
template <typename T = void> // dummy template for static var (in-function static generates ineffective, useless for us, thread-safe code)
class _ClassType_helper
{
public:
    static class_hash_map<const void*, weak_ptr<AbstractStaticClassData>, IntPtrHash> data;
    static weak_ptr<AbstractStaticClassData>& _getStaticClassData(const void* type) { return data[type]; }
    static Sqrat::hash_set<AbstractStaticClassData*> all_classes;
};
template<typename T>
class_hash_map<const void*, weak_ptr<AbstractStaticClassData>, IntPtrHash> _ClassType_helper<T>::data;

template<typename T>
Sqrat::hash_set<AbstractStaticClassData*> _ClassType_helper<T>::all_classes;



// Every Squirrel class instance made by Sqrat has its type tag set to a AbstractStaticClassData object that is unique per C++ class
struct AbstractStaticClassData {
    AbstractStaticClassData() {
        _ClassType_helper<>::all_classes.insert(this);
    }
    AbstractStaticClassData(const AbstractStaticClassData &) = delete;
    AbstractStaticClassData(AbstractStaticClassData &&) = default;
    AbstractStaticClassData& operator=(const AbstractStaticClassData &) = delete;
    AbstractStaticClassData& operator=(AbstractStaticClassData &&) = default;
    virtual ~AbstractStaticClassData() {
        _ClassType_helper<>::all_classes.erase(this);
    }
    virtual SQUserPointer Cast(SQUserPointer ptr, SQUserPointer classType) = 0;

    static bool isValidSqratTypeTag(SQUserPointer tag) {
        return isValidSqratClass(reinterpret_cast<AbstractStaticClassData *>(tag));
    }

    static bool isValidSqratClass(AbstractStaticClassData *asd) {
        return _ClassType_helper<>::all_classes.find(asd) != _ClassType_helper<>::all_classes.end();
    }

    static AbstractStaticClassData* FromObject(const HSQOBJECT *obj) {
        AbstractStaticClassData* actualType = nullptr;
        if (SQ_FAILED(sq_getobjtypetag(obj, (SQUserPointer*)&actualType)))
          return nullptr;
        return isValidSqratClass(actualType) ? actualType : nullptr;
    }

    AbstractStaticClassData* baseClass;
    string                   className;
    COPYFUNC                 copyFunc;
};


// StaticClassData keeps track of the nearest base class B and the class associated with itself C in order to cast C++ pointers to the right base class
template<class C, class B>
struct StaticClassData : public AbstractStaticClassData {
    virtual SQUserPointer Cast(SQUserPointer ptr, SQUserPointer classType) override {
        if (classType != this) {
            ptr = baseClass->Cast(static_cast<B*>(static_cast<C*>(ptr)), classType);
        }
        return ptr;
    }
};

template<class C> using InstancesMap = class_hash_map<C*, HSQOBJECT>;
template<class C> using InstancePtrAndMap = SQRAT_STD::pair<C*, shared_ptr<InstancesMap<C>> >;

// Every Squirrel class object created by Sqrat in every VM has its own unique ClassData object stored in the registry table of the VM
template<class C>
struct ClassData {
    HSQOBJECT classObj;
    HSQOBJECT getTable;
    HSQOBJECT setTable;
    shared_ptr<InstancesMap<C>> instances;
    shared_ptr<AbstractStaticClassData> staticData;

    static int type_id_helper;
    static void* type_id() { return &type_id_helper; }
};

template<class C> int ClassData<C>::type_id_helper = 0;


struct ClassesRegistryTable {
    static SQUserPointer slotKey() {
        static int slot_id_helper = 0;
        return &slot_id_helper;
    }
};

// Internal helper class for managing classes
template<class C>
class ClassType {
public:

    static inline ClassData<C>* getClassData(HSQUIRRELVM vm) {
        sq_pushregistrytable(vm);
        sq_pushuserpointer(vm, ClassesRegistryTable::slotKey());
        SQRESULT r = sq_rawget(vm, -2);
        SQRAT_ASSERT(SQ_SUCCEEDED(r)); // fails if getClassData is called when the data does not exist for the given VM yet (bind the class)
        sq_pushuserpointer(vm, ClassData<C>::type_id());

        r = sq_rawget(vm, -2);
        SQRAT_ASSERT(SQ_SUCCEEDED(r)); // fails if getClassData is called when the data does not exist for the given VM yet (bind the class)
        ClassData<C>** ud;
        sq_getuserdata(vm, -1, (SQUserPointer*)&ud, NULL);
        sq_pop(vm, 3);
        return *ud;
    }

    static weak_ptr<AbstractStaticClassData>& getStaticClassData() {
        return _ClassType_helper<>::_getStaticClassData(ClassData<C>::type_id());
    }

    static inline bool hasClassData(HSQUIRRELVM vm) {
        if (!getStaticClassData().expired()) {
            sq_pushregistrytable(vm);
            sq_pushuserpointer(vm, ClassesRegistryTable::slotKey());
            if (SQ_SUCCEEDED(sq_rawget(vm, -2))) {
                sq_pushuserpointer(vm, ClassData<C>::type_id());
                if (SQ_SUCCEEDED(sq_rawget(vm, -2))) {
                    sq_pop(vm, 3);
                    return true;
                }
                sq_pop(vm, 1);
            }
            sq_pop(vm, 1);
        }
        return false;
    }

    static inline AbstractStaticClassData*& BaseClass() {
        SQRAT_ASSERT(getStaticClassData().expired() == false); // fails because called before a Sqrat::Class for this type exists
        return getStaticClassData().lock()->baseClass;
    }

    static inline const string& ClassName() {
        SQRAT_ASSERT(getStaticClassData().expired() == false); // fails because called before a Sqrat::Class for this type exists
        return getStaticClassData().lock()->className;
    }

    static inline COPYFUNC& CopyFunc() {
        SQRAT_ASSERT(getStaticClassData().expired() == false); // fails because called before a Sqrat::Class for this type exists
        return getStaticClassData().lock()->copyFunc;
    }

    static bool IsObjectOfClass(const HSQOBJECT *obj)
    {
        AbstractStaticClassData* actualType;
        if (SQ_FAILED(sq_getobjtypetag(obj, (SQUserPointer*)&actualType)))
            return false;
        if (!actualType || !AbstractStaticClassData::isValidSqratClass(actualType))
            return false;
        AbstractStaticClassData* thisClass = getStaticClassData().lock().get();
        for (AbstractStaticClassData *cls = actualType; cls; cls = cls->baseClass)
            if (cls == thisClass)
                return true;
        return false;
    }

    static SQInteger HookRemoveInstanceFromMap(SQUserPointer ptr, SQInteger size) {
        SQRAT_UNUSED(size);
        InstancePtrAndMap<C> *instance = reinterpret_cast<InstancePtrAndMap<C>*>(ptr);
        instance->second->erase(instance->first);
        delete instance;
        return 0;
    }

    static SQInteger HookDeleteManagedInstance(SQUserPointer ptr, SQInteger size) {
        SQRAT_UNUSED(size);
        InstancePtrAndMap<C> *instance = reinterpret_cast<InstancePtrAndMap<C>*>(ptr);
        instance->second->erase(instance->first);
        delete instance->first;
        delete instance;
        return 0;
    }

    static bool PushNativeInstance(HSQUIRRELVM vm, C* ptr) {
        if (!ptr) {
            sq_pushnull(vm);
            return true;
        }

        ClassData<C>* cd = getClassData(vm);

        auto it = cd->instances->find(ptr);
        if (it != cd->instances->end()) {
            sq_pushobject(vm, it->second);
            return true;
        }

        sq_pushobject(vm, cd->classObj);
        if (SQ_FAILED(sq_createinstance(vm, -1))) {
          SQRAT_ASSERT(!"Failed to create class instance");
          sq_pop(vm, 1);
          return false;
        }

        sq_remove(vm, -2);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setinstanceup(vm, -1, new InstancePtrAndMap<C>(ptr, cd->instances))));
        sq_setreleasehook(vm, -1, &HookRemoveInstanceFromMap);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &((*cd->instances)[ptr]))));
        return true;
    }

    static bool PushInstanceCopy(HSQUIRRELVM vm, const C& value) {
        sq_pushobject(vm, getClassData(vm)->classObj);
        sq_createinstance(vm, -1);
        sq_remove(vm, -2);
        SQRESULT result = CopyFunc()(vm, -1, &value);
        SQRAT_UNUSED(result);
        SQRAT_ASSERT(SQ_SUCCEEDED(result)); // fails when trying to copy an object defined as non-copyable
        return true;
    }

    static C* GetInstance(HSQUIRRELVM vm, SQInteger idx, bool nullAllowed = false) {
        AbstractStaticClassData* classType = NULL;
        InstancePtrAndMap<C> * instance = NULL;
        if (hasClassData(vm)) /* type checking only done if the value has type data else it may be enum */
        {
            if (nullAllowed && sq_gettype(vm, idx) == OT_NULL) {
                return NULL;
            }

            classType = getStaticClassData().lock().get();

            if (SQ_FAILED(sq_getinstanceup(vm, idx, (SQUserPointer*)&instance, classType))) {
                SQRAT_ASSERTF(0, FormatTypeError(vm, idx, ClassName().c_str()).c_str());
                return NULL;
            }

            if (instance == NULL) {
                SQRAT_ASSERTF(0, _SC("got unconstructed native class (call base.constructor in the constructor of Squirrel classes that extend native classes)"));
                return NULL;
            }
        }
        else /* value is likely of integral type like enums, cannot return a pointer */
        {
            SQRAT_ASSERTF(sq_gettype(vm, idx) == OT_INTEGER, FormatTypeError(vm, idx, _SC("unknown")).c_str());
            return NULL;
        }
        AbstractStaticClassData* actualType;
        sq_gettypetag(vm, idx, (SQUserPointer*)&actualType);
        if (actualType == NULL) {
            SQInteger top = sq_gettop(vm);
            sq_getclass(vm, idx);
            while (actualType == NULL) {
                sq_getbase(vm, -1);
                sq_gettypetag(vm, -1, (SQUserPointer*)&actualType);
            }
            sq_settop(vm, top);
        }
        if (classType != actualType) {
            return static_cast<C*>(actualType->Cast(instance->first, classType));
        }
        return static_cast<C*>(instance->first);
    }


    static void SetManagedInstance(HSQUIRRELVM vm, SQInteger idx, C* ptr) {
        ClassData<C>* cd = ClassType<C>::getClassData(vm);
        sq_setinstanceup(vm, idx, new InstancePtrAndMap<C>(ptr, cd->instances));
        sq_setreleasehook(vm, idx, &HookDeleteManagedInstance);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, idx, &((*cd->instances)[ptr]))));
    }


    static bool IsClassInstance(const HSQOBJECT &ho, bool nullAllowed = false) {
        SQObjectType type = sq_type(ho);
        if (nullAllowed && type == OT_NULL)
            return true;
        if (type != OT_INSTANCE)
            return false;
        AbstractStaticClassData* classType = getStaticClassData().lock().get();
        AbstractStaticClassData* actualType = nullptr;
        if (SQ_FAILED(sq_getobjtypetag(&ho, (SQUserPointer*)&actualType)))
            return false;
        if (!AbstractStaticClassData::isValidSqratClass(actualType))
            return false;
        for (; actualType; actualType=actualType->baseClass) {
            if (actualType == classType) {
                return true;
            }
        }
        return false;
    }


    static bool IsClassInstance(HSQUIRRELVM vm, SQInteger idx, bool nullAllowed = false) {
        HSQOBJECT ho;
        sq_getstackobj(vm, idx, &ho);
        return IsClassInstance(ho, nullAllowed);
    }

    static SQInteger ToString(HSQUIRRELVM vm) {
        HSQOBJECT ho;
        sq_getstackobj(vm, 1, &ho);
        int l = snprintf(nullptr, 0, _SC("%s (0x%p)"), ClassName().c_str(), ho._unVal.pInstance);
        string str(l+1, '\0');
        snprintf(&str[0], str.size(), _SC("%s (0x%p)"), ClassName().c_str(), ho._unVal.pInstance);
        sq_pushstring(vm, str.c_str(), l);
        return 1;
    }
};

}

#endif
