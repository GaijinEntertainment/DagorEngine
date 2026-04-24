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


// VM-independent type identity. Static storage, one per C++ type, lives forever.
// Used as SQClass::_typetag. Carries the inheritance chain and cast function.
struct TypeTagBase {
    TypeTagBase* base = nullptr;
    SQUserPointer (*castToBase)(SQUserPointer) = nullptr;
    const char* className = nullptr; // set once during first registration
    bool hasDerived = false;

    // Validates that a type tag pointer is a Sqrat TypeTag.
    // Entries are added at registration and never removed (safe to read after init).
    static hash_set<TypeTagBase*>& allTypeTags() {
        static hash_set<TypeTagBase*> s; //-V1096
        return s;
    }
    static bool isSqratTypeTag(const void* tag) {
        return tag && allTypeTags().count(
            const_cast<TypeTagBase*>(static_cast<const TypeTagBase*>(tag)));
    }
};

template<class C>
struct TypeTag : TypeTagBase {
    static TypeTag instance;
    static TypeTag* get() { return &instance; }
};
template<class C> TypeTag<C> TypeTag<C>::instance;


// Per-VM, per-type class data. Stored as userpointer in the VM registry.
// Only accessed on cold paths (Push, class registration, instance tracking).
template<class C> using InstancesMap = class_hash_map<C*, HSQOBJECT>;

template<class C>
struct ClassData {
    HSQOBJECT classObj;
    HSQOBJECT getTable;
    HSQOBJECT setTable;
    COPYFUNC copyFunc = nullptr;
    string className;
    InstancesMap<C>* instances = nullptr;

    // Static class name, set once during first registration. Same for all VMs.
    static string staticClassName;

    ClassData() {
        sq_resetobject(&classObj);
        sq_resetobject(&getTable);
        sq_resetobject(&setTable);
    }

    ~ClassData() {
        delete instances;
    }
};

template<class C> string ClassData<C>::staticClassName;


// Cleanup sentinel -- one per VM, deletes all ClassData entries on VM close.
// Stored as a userdata in the registry so its release hook fires during sq_close.
struct SqratCleanup {
    struct Entry {
        void* ptr;
        void (*deleter)(void*);
    };
    SQRAT_STD::vector<Entry> entries;

    static SQUserPointer key() {
        static int k = 0; //-V1096
        return &k;
    }

    template<class C>
    void add(ClassData<C>* cd) {
        entries.push_back({cd, [](void* p) { delete static_cast<ClassData<C>*>(p); }});
    }

    static SQInteger release(HSQUIRRELVM, SQUserPointer ptr, SQInteger) {
        SqratCleanup** pSelf = reinterpret_cast<SqratCleanup**>(ptr);
        SqratCleanup* self = *pSelf;
        for (auto& e : self->entries)
            e.deleter(e.ptr);
        delete self;
        return 0;
    }

    static SqratCleanup* getOrCreate(HSQUIRRELVM vm) {
        // Look up existing sentinel in registry
        HSQOBJECT registry;
        sq_getregistrytableobj(vm, &registry);

        HSQOBJECT sentinelKey;
        sq_resetobject(&sentinelKey);
        sentinelKey._type = OT_USERPOINTER;
        sentinelKey._unVal.pUserPointer = key();

        HSQOBJECT result;
        if (SQ_SUCCEEDED(sq_obj_get(vm, &registry, &sentinelKey, &result, true))) {
            SqratCleanup** pp;
            sq_obj_getuserdata(&result, (SQUserPointer*)&pp, nullptr);
            SqratCleanup *ret = *pp;
            sq_poptop(vm);
            return ret;
        }

        // Create new sentinel
        sq_pushregistrytable(vm);
        sq_pushuserpointer(vm, key());
        SqratCleanup** pp = reinterpret_cast<SqratCleanup**>(sq_newuserdata(vm, sizeof(SqratCleanup*)));
        *pp = new SqratCleanup();
        sq_setreleasehook(vm, -1, &SqratCleanup::release);
        sq_rawset(vm, -3);
        sq_pop(vm, 1);
        return *pp;
    }
};


// Iterative cast walk using TypeTagBase inheritance chain
inline SQUserPointer CastToTarget(SQUserPointer ptr, TypeTagBase* actual, TypeTagBase* target) {
    while (actual != target) {
        if (!actual || !actual->castToBase)
            return nullptr;
        ptr = actual->castToBase(ptr);
        actual = actual->base;
    }
    return ptr;
}


// Internal helper class for managing classes
template<class C>
class ClassType {

public:
    // Registry lookup for per-VM ClassData. Uses TypeTag address as key.
    // Only needed on cold paths (Push, class registration, tracking).
    static inline ClassData<C>* getClassData(HSQUIRRELVM vm) {
        HSQOBJECT registry;
        sq_getregistrytableobj(vm, &registry);

        HSQOBJECT key;
        sq_resetobject(&key);
        key._type = OT_USERPOINTER;
        key._unVal.pUserPointer = TypeTag<C>::get();

        HSQOBJECT result;
        if (SQ_FAILED(sq_obj_get(vm, &registry, &key, &result, true)))
            return nullptr;

        ClassData<C> *ret = static_cast<ClassData<C>*>(result._unVal.pUserPointer);
        sq_poptop(vm);
        return ret;
    }

    static inline bool hasClassData(HSQUIRRELVM vm) {
        return getClassData(vm) != nullptr;
    }

    static inline const string& ClassName() {
        return ClassData<C>::staticClassName;
    }

    static inline const string& ClassName(HSQUIRRELVM vm) {
        ClassData<C>* cd = getClassData(vm);
        SQRAT_ASSERT(cd);
        return cd->className; //-V522
    }

    static inline COPYFUNC CopyFunc(HSQUIRRELVM vm) {
        ClassData<C>* cd = getClassData(vm);
        SQRAT_ASSERT(cd);
        return cd->copyFunc; //-V522
    }

    // Type checks using static TypeTag -- zero registry lookups
    static bool IsObjectOfClass(const HSQOBJECT *obj) {
        TypeTagBase* actualTag = nullptr;
        if (SQ_FAILED(sq_getobjtypetag(obj, (SQUserPointer*)&actualTag)))
            return false;
        if (!TypeTagBase::isSqratTypeTag(actualTag))
            return false;
        for (TypeTagBase* t = actualTag; t; t = t->base)
            if (t == TypeTag<C>::get())
                return true;
        return false;
    }

    // Release hooks.
    // During sq_close's GC chain walk, vm can be nullptr (_root_vm is already
    // nulled) and ClassData is already deleted, so map erase is skipped.

    // Owned: Sqrat created this object, delete it on GC.
    static SQInteger ReleaseOwned(HSQUIRRELVM vm, SQUserPointer ptr, SQInteger) {
        if (vm) {
            ClassData<C>* cd = getClassData(vm);
            if (cd && cd->instances)
                cd->instances->erase(static_cast<C*>(ptr));
        }
        delete static_cast<C*>(ptr);
        return 0;
    }

    // Borrowed: C++ owns this object, just remove from map on GC.
    static SQInteger ReleaseBorrowed(HSQUIRRELVM vm, SQUserPointer ptr, SQInteger) {
        if (vm) {
            ClassData<C>* cd = getClassData(vm);
            if (cd && cd->instances)
                cd->instances->erase(static_cast<C*>(ptr));
        }
        return 0;
    }

    // Inline: object lives inside SQInstance memory, just destruct.
    static SQInteger DestructInline(HSQUIRRELVM, SQUserPointer ptr, SQInteger) {
        static_cast<C*>(ptr)->~C();
        return 0;
    }


    static bool PushNativeInstance(HSQUIRRELVM vm, C* ptr) {
        if (!ptr) {
            sq_pushnull(vm);
            return true;
        }

        ClassData<C>* cd = getClassData(vm);
        SQRAT_ASSERT(cd); // class must be registered for this VM
        if (!cd)
            return false;

        // Dedup: if this pointer is already known (owned or borrowed),
        // return the existing Quirrel instance. Prevents creating a
        // borrowed duplicate of an owned instance (which would dangle
        // after GC deletes the owned original).
        if (!cd->instances)
            cd->instances = new InstancesMap<C>();
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
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_setinstanceup(vm, -1, ptr)));
        sq_setreleasehook(vm, -1, &ReleaseBorrowed);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, -1, &(*cd->instances)[ptr])));
        return true;
    }

    static bool PushInstanceCopy(HSQUIRRELVM vm, const C& value) {
        ClassData<C>* cd = getClassData(vm);
        SQRAT_ASSERT(cd); // class must be registered for this VM
        if (!cd)
            return false;

        sq_pushobject(vm, cd->classObj);
        if (SQ_FAILED(sq_createinstance(vm, -1))) {
            SQRAT_ASSERT(!"Failed to create class instance");
            sq_pop(vm, 1);
            return false;
        }
        sq_remove(vm, -2);

        SQRESULT result = cd->copyFunc(vm, -1, &value);
        SQRAT_UNUSED(result);
        SQRAT_ASSERT(SQ_SUCCEEDED(result));
        return true;
    }

    // Hot path: zero registry lookups. Uses static TypeTag for type checking.
    static C* GetInstance(HSQUIRRELVM vm, SQInteger idx, bool nullAllowed = false) {
        if (nullAllowed && sq_gettype(vm, idx) == OT_NULL)
            return nullptr;

        C* ptr = nullptr;
        // TypeTag<C>::get() is a static address -- no registry lookup needed
        if (SQ_FAILED(sq_getinstanceup(vm, idx, (SQUserPointer*)&ptr, TypeTag<C>::get()))) {
            SQRAT_ASSERTF(0, FormatTypeError(vm, idx, ClassName().c_str()).c_str());
            return nullptr;
        }

        if (!ptr) {
            SQRAT_ASSERTF(0, "got unconstructed native class (call base.constructor in the constructor of Squirrel classes that extend native classes)");
            return nullptr;
        }

        // Cast if actual type differs from requested type (inheritance).
        // Skip entirely for leaf types (no derived classes) -- the common case.
        if (TypeTag<C>::get()->hasDerived) {
            TypeTagBase* actualTag = nullptr;
            sq_gettypetag(vm, idx, (SQUserPointer*)&actualTag);
            if (actualTag != TypeTag<C>::get()) {
                ptr = static_cast<C*>(CastToTarget(ptr, actualTag, TypeTag<C>::get()));
                SQRAT_ASSERT(ptr); // TypeTag chain must be consistent with SQClass chain
            }
        }

        return ptr;
    }


    // Stackless instance extraction from HSQOBJECT.
    // Returns nullptr on null/failure (no asserts -- callers report errors).
    static C* GetInstanceFromObj(const HSQOBJECT &o) {
        if (o._type == OT_NULL)
            return nullptr;

        SQUserPointer ptr = nullptr;
        if (SQ_FAILED(sq_obj_getinstanceup(&o, &ptr, TypeTag<C>::get())))
            return nullptr;

        if (!ptr)
            return nullptr;  // unconstructed instance

        if (TypeTag<C>::get()->hasDerived) {
            TypeTagBase *actualTag = nullptr;
            sq_getobjtypetag(&o, (SQUserPointer *)&actualTag);
            if (actualTag != TypeTag<C>::get()) {
                ptr = CastToTarget(ptr, actualTag, TypeTag<C>::get());
                SQRAT_ASSERT(ptr);
            }
        }

        return static_cast<C *>(ptr);
    }


    static void SetManagedInstance(HSQUIRRELVM vm, SQInteger idx, C* ptr) {
        sq_setinstanceup(vm, idx, ptr);
        ClassData<C>* cd = getClassData(vm);
        SQRAT_ASSERT(cd);
        if (!cd->instances) //-V522
            cd->instances = new InstancesMap<C>();
        sq_setreleasehook(vm, idx, &ReleaseOwned);
        SQRAT_VERIFY(SQ_SUCCEEDED(sq_getstackobj(vm, idx, &(*cd->instances)[ptr]))); //-V522
    }


    // For SquirrelCtors: allocate instance data, preferring inline if available.
    // Returns a default-constructed C*. Caller overwrites contents as needed.
    // Inline path: DestructInline hook, no map. Heap path: ReleaseOwned hook, map.
    static C* AllocInstanceData(HSQUIRRELVM vm, SQInteger idx) {
        if constexpr (sizeof(C) <= 256 && alignof(C) <= SQ_ALIGNMENT) {
            C* ptr = nullptr;
            sq_getinstanceup(vm, idx, (SQUserPointer*)&ptr, nullptr);
            if (ptr) {
                new (ptr) C();
                sq_setreleasehook(vm, idx, &DestructInline);
                return ptr;
            }
        }
        C* ptr = new C;
        SetManagedInstance(vm, idx, ptr);
        return ptr;
    }


    static bool IsClassInstance(const HSQOBJECT &ho) {
        SQObjectType type = sq_type(ho);
        if (type != OT_INSTANCE)
            return false;
        TypeTagBase* actualTag = nullptr;
        if (SQ_FAILED(sq_getobjtypetag(&ho, (SQUserPointer*)&actualTag)))
            return false;
        if (!TypeTagBase::isSqratTypeTag(actualTag))
            return false;
        for (TypeTagBase* t = actualTag; t; t = t->base)
            if (t == TypeTag<C>::get())
                return true;
        return false;
    }

    static bool IsClassInstance(HSQUIRRELVM vm, SQInteger idx) {
        HSQOBJECT ho;
        sq_getstackobj(vm, idx, &ho);
        return IsClassInstance(ho);
    }

    static SQInteger ToString(HSQUIRRELVM vm) {
        HSQOBJECT ho;
        sq_getstackobj(vm, 1, &ho);
        char buf[256];
        int l = snprintf(buf, sizeof(buf), "%s (%p)", ClassName().c_str(), ho._unVal.pInstance);
        if (l >= (int)sizeof(buf))
            l = (int)sizeof(buf) - 1;
        sq_pushstring(vm, buf, l);
        return 1;
    }


    // Store ClassData in registry as userpointer, keyed by TypeTag address
    static void setClassData(HSQUIRRELVM vm, ClassData<C>* cd) {
        sq_pushregistrytable(vm);
        sq_pushuserpointer(vm, TypeTag<C>::get());
        sq_pushuserpointer(vm, cd);
        sq_rawset(vm, -3);
        sq_pop(vm, 1);
    }
};

}

#endif
