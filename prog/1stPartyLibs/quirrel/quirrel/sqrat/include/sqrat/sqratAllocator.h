// Sqrat: altered version by Gaijin Games KFT
// SqratAllocator: Custom Class Allocation/Deallocation
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
#if !defined(_SQRAT_ALLOCATOR_H_)
#define _SQRAT_ALLOCATOR_H_

#include <squirrel.h>
#include <sqstdaux.h>
#include <string.h>

#include "sqratObject.h"
#include "sqratTypes.h"

namespace Sqrat {

namespace vargs
{
  template<typename T>
  T&  extract(Var<T>& var)
  {
    return  var.value;
  }

  template<typename T>
  T  extract(T&& var)
  {
    return  var;
  }

  template <class C, class Tuple, size_t... Indexes>
  C *apply_ctor_helper(SQRAT_STD::index_sequence<Indexes...>, Tuple &&args)
  {
    (void)args; // 'args' is unused in case of empty 'Indexes'
    return new C(extract(SQRAT_STD::get<Indexes>(args))...);
  }

  template <class C, class Tuple>
  C *apply_ctor(Tuple &&tup)
  {
    constexpr auto argsN = SQRAT_STD::tuple_size<SQRAT_STD::decay_t<Tuple>>::value;
    return apply_ctor_helper<C>(SQRAT_STD::make_index_sequence<argsN>(), tup);
  }

  template <class C, class Tuple, size_t... Indexes>
  void apply_placement_ctor_helper(void *mem, SQRAT_STD::index_sequence<Indexes...>, Tuple &&args)
  {
    (void)args;
    new (mem) C(extract(SQRAT_STD::get<Indexes>(args))...);
  }

  template <class C, class Tuple>
  void apply_placement_ctor(void *mem, Tuple &&tup)
  {
    constexpr auto argsN = SQRAT_STD::tuple_size<SQRAT_STD::decay_t<Tuple>>::value;
    apply_placement_ctor_helper<C>(mem, SQRAT_STD::make_index_sequence<argsN>(), tup);
  }
}

template <class T, bool b>
struct NewC
{
    T* p;
    NewC()
    {
       p = new T();
    }
};

template <class T>
struct NewC<T, false>
{
    T* p;
    NewC()
    {
       p = 0;
    }
};

template<class C>
class DefaultAllocator {

public:

    static SQInteger New(HSQUIRRELVM vm) {
        if constexpr (SQRAT_STD::is_default_constructible<C>::value && sizeof(C) <= 256 && alignof(C) <= SQ_ALIGNMENT) {
            C* ptr = nullptr;
            sq_getinstanceup(vm, 1, (SQUserPointer*)&ptr, nullptr);
            if (ptr) {
                new (ptr) C();
                sq_setreleasehook(vm, 1, &ClassType<C>::DestructInline);
                return 0;
            }
        }
        ClassType<C>::SetManagedInstance(vm, 1, NewC<C, SQRAT_STD::is_default_constructible<C>::value >().p);
        return 0;
    }

    static SQInteger iNew(HSQUIRRELVM vm) {
        return New(vm);
    }

    template <typename...A>
    static SQInteger iNew(HSQUIRRELVM vm) {
        if (!vargs::check_var_types<A...>(vm, 2))
            return SQ_ERROR;
        auto vars = vargs::make_vars<A...>(vm, 2);
        if constexpr (sizeof(C) <= 256 && alignof(C) <= SQ_ALIGNMENT) {
            C* ptr = nullptr;
            sq_getinstanceup(vm, 1, (SQUserPointer*)&ptr, nullptr);
            if (ptr) {
                vargs::apply_placement_ctor<C>(ptr, SQRAT_STD::move(vars));
                sq_setreleasehook(vm, 1, &ClassType<C>::DestructInline);
                return 0;
            }
        }
        C *inst = vargs::apply_ctor<C>(vars);
        ClassType<C>::SetManagedInstance(vm, 1, inst);
        return 0;
    }

public:

    // Copy using inline userdata (placement new into SQInstance's inline space)
    // or heap allocation as fallback for large types.
    static SQInteger Copy(HSQUIRRELVM vm, SQInteger idx, const void* value) {
        if constexpr (sizeof(C) <= 256 && alignof(C) <= SQ_ALIGNMENT) {
            // Inline path: _userpointer already points to inline space from sq_createinstance
            C* ptr = nullptr;
            sq_getinstanceup(vm, idx, (SQUserPointer*)&ptr, nullptr);
            if (ptr) {
                new (ptr) C(*static_cast<const C*>(value));
                sq_setreleasehook(vm, idx, &ClassType<C>::DestructInline);
                return 0;
            }
        }
        // Fallback: heap allocation
        ClassType<C>::SetManagedInstance(vm, idx, new C(*static_cast<const C*>(value)));
        return 0;
    }
};

template<class C>
class NoConstructor {
public:

    static SQInteger New(HSQUIRRELVM vm) {
        return sqstd_throwerrorf(vm, "Construction of %s is not allowed", ClassType<C>::ClassName().c_str());
    }

    static SQInteger Copy(HSQUIRRELVM vm, SQInteger idx, const void* value) {
        SQRAT_UNUSED(vm);
        SQRAT_UNUSED(idx);
        SQRAT_UNUSED(value);
        return sqstd_throwerrorf(vm, "Cloning of %s is not allowed", ClassType<C>::ClassName().c_str());
    }
};

template<class C>
class CopyOnly {
public:

    static SQInteger New(HSQUIRRELVM vm) {
        return sqstd_throwerrorf(vm, "Construction of %s is not allowed", ClassType<C>::ClassName().c_str());
    }

    static SQInteger Copy(HSQUIRRELVM vm, SQInteger idx, const void* value) {
        if constexpr (sizeof(C) <= 256 && alignof(C) <= SQ_ALIGNMENT) {
            C* ptr = nullptr;
            sq_getinstanceup(vm, idx, (SQUserPointer*)&ptr, nullptr);
            if (ptr) {
                new (ptr) C(*static_cast<const C*>(value));
                sq_setreleasehook(vm, idx, &ClassType<C>::DestructInline);
                return 0;
            }
        }
        ClassType<C>::SetManagedInstance(vm, idx, new C(*static_cast<const C*>(value)));
        return 0;
    }
};


template<class C>
class NoCopy {
public:

    static SQInteger New(HSQUIRRELVM vm) {
        ClassType<C>::SetManagedInstance(vm, 1, NewC<C, SQRAT_STD::is_default_constructible<C>::value >().p);
        return 0;
    }

    static SQInteger iNew(HSQUIRRELVM vm) {
        return New(vm);
    }

    template <typename...A>
    static SQInteger iNew(HSQUIRRELVM vm) {
        if (!vargs::check_var_types<A...>(vm, 2))
            return SQ_ERROR;
        auto vars = vargs::make_vars<A...>(vm, 2);
        C *inst = vargs::apply_ctor<C>(vars);
        ClassType<C>::SetManagedInstance(vm, 1, inst);
        return 0;
    }

    static SQInteger Copy(HSQUIRRELVM vm, SQInteger idx, const void* value) {
        SQRAT_UNUSED(vm);
        SQRAT_UNUSED(idx);
        SQRAT_UNUSED(value);
        return sqstd_throwerrorf(vm, "Cloning of %s is not allowed", ClassType<C>::ClassName().c_str());
    }
};

}

#endif
