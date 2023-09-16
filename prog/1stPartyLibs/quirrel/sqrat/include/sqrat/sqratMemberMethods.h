// Sqrat: altered version by Gaijin Games KFT
// SqratMemberMethods: Member Methods
//

//
// Copyright (c) 2009 Brandon Jones
// Copyright 2011 Li-Cheng (Andy) Tai
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
#if !defined(_SQRAT_MEMBER_METHODS_H_)
#define _SQRAT_MEMBER_METHODS_H_

#include <squirrel.h>
#include "sqratTypes.h"

namespace Sqrat {


template<class C, class MemberFunc, class MemberFuncSig = member_function_signature_t<MemberFunc>>
struct SqMemberThunkGen;

template<class C, class MemberFunc, class R, class... A>
struct SqMemberThunkGen<C, MemberFunc, R(A...)>
{
  static SQInteger Func(HSQUIRRELVM vm)
  {
    if (sq_gettop(vm) != 2 + sizeof...(A))
      return sq_throwerror(vm, _SC("wrong number of parameters"));

    if (!vargs::check_var_types<A...>(vm, 2))
      return SQ_ERROR;

    MemberFunc *methodPtr;
    sq_getuserdata(vm, -1, (SQUserPointer *)&methodPtr, NULL);

    C *ptr = Var<C *>(vm, 1).value;
    if (!ptr)
      return sq_throwerror(vm, FormatTypeError(vm, 1, ClassType<C>::ClassName().c_str()).c_str());
    auto vars = vargs::make_vars<A...>(vm, 2);
    R ret = vargs::apply_member(ptr, *methodPtr, vars);
    PushVar(vm, ret);
    return 1;
  }
};

template<class C, class MemberFunc, class... A>
struct SqMemberThunkGen<C, MemberFunc, void(A...)>
{
  static SQInteger Func(HSQUIRRELVM vm)
  {
    if (sq_gettop(vm) != 2 + sizeof...(A))
      return sq_throwerror(vm, _SC("wrong number of parameters"));

    if (!vargs::check_var_types<A...>(vm, 2))
      return SQ_ERROR;

    MemberFunc *methodPtr;
    sq_getuserdata(vm, -1, (SQUserPointer *)&methodPtr, NULL);

    C *ptr = Var<C *>(vm, 1).value;
    if (!ptr)
      return sq_throwerror(vm, FormatTypeError(vm, 1, ClassType<C>::ClassName().c_str()).c_str());
    auto vars = vargs::make_vars<A...>(vm, 2);
    vargs::apply_member(ptr, *methodPtr, vars);
    return 0;
  }
};

//
// Member Function Resolvers
//

template<class C, class MemberFunc>
SQFUNCTION SqMemberFunc()
{
  return &SqMemberThunkGen<C, MemberFunc>::Func;
}


//
// Variable Get
//

template <class C, class V>
inline SQInteger sqDefaultGet(HSQUIRRELVM vm) {
    C* ptr = Var<C*>(vm, 1).value;
    if (!ptr)
      return sq_throwerror(vm, FormatTypeError(vm, 1, ClassType<C>::ClassName().c_str()).c_str());

    typedef V C::*M;
    M* memberPtr = NULL;
    sq_getuserdata(vm, -1, (SQUserPointer*)&memberPtr, NULL); // Get Member...
    M member = *memberPtr;

    PushVarR(vm, ptr->*member);

    return 1;
}

template <class C, class V>
inline SQInteger sqStaticGet(HSQUIRRELVM vm) {
    typedef V *M;
    M* memberPtr = NULL;
    sq_getuserdata(vm, -1, (SQUserPointer*)&memberPtr, NULL); // Get Member...
    M member = *memberPtr;

    PushVarR(vm, *member);

    return 1;
}

inline SQInteger sqVarGet(HSQUIRRELVM vm) {
    // Find the get method in the get table
    sq_push(vm, 2);
    if (SQ_FAILED(sq_rawget_noerr(vm, -2))) {
        sq_pushnull(vm);
        return sq_throwobject(vm);
    }

    // push 'this'
    sq_push(vm, 1);

    // Call the getter
    SQRESULT result = sq_call(vm, 1, true, SQTrue);
    return SQ_SUCCEEDED(result) ? 1 : SQ_ERROR;
}


//
// Variable Set
//

template <class C, class V>
inline SQInteger sqDefaultSet(HSQUIRRELVM vm) {
    C* ptr = Var<C*>(vm, 1).value;
    if (!ptr)
      return sq_throwerror(vm, FormatTypeError(vm, 1, ClassType<C>::ClassName().c_str()).c_str());

    typedef V C::*M;
    M* memberPtr = NULL;
    sq_getuserdata(vm, -1, (SQUserPointer*)&memberPtr, NULL); // Get Member...
    M member = *memberPtr;

    if (SQRAT_STD::is_pointer<V>::value || SQRAT_STD::is_reference<V>::value) {
        ptr->*member = Var<V>(vm, 2).value;
    } else {
        ptr->*member = Var<const V&>(vm, 2).value;
    }

    return 0;
}

template <class C, class V>
inline SQInteger sqStaticSet(HSQUIRRELVM vm) {
    typedef V *M;
    M* memberPtr = NULL;
    sq_getuserdata(vm, -1, (SQUserPointer*)&memberPtr, NULL); // Get Member...
    M member = *memberPtr;

    if (SQRAT_STD::is_pointer<V>::value || SQRAT_STD::is_reference<V>::value) {
        *member = Var<V>(vm, 2).value;
    } else {
        *member = Var<const V&>(vm, 2).value;
    }

    return 0;
}

inline SQInteger sqVarSet(HSQUIRRELVM vm) {
    // Find the set method in the set table
    sq_push(vm, 2);
    if (SQ_FAILED(sq_rawget(vm, -2))) {
        sq_pushnull(vm);
        return sq_throwobject(vm);
    }

    // push 'this'
    sq_push(vm, 1);
    sq_push(vm, 3);

    // Call the setter
    SQRESULT result = sq_call(vm, 2, false, SQTrue);
    return SQ_SUCCEEDED(result) ? 0 : SQ_ERROR;
}


}

#endif
