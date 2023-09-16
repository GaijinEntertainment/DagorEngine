// Sqrat: altered version by Gaijin Games KFT
// SqratGlobalMethods: Global Methods
//

//
// Copyright (c) 2009 Brandon Jones
// Copyirght 2011 Li-Cheng (Andy) Tai
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
#if !defined(_SQRAT_GLOBAL_METHODS_H_)
#define _SQRAT_GLOBAL_METHODS_H_

#include <squirrel.h>
#include "sqratTypes.h"

namespace Sqrat {

template<class Func, class Sig = get_callable_function_t<Func>>
class SqThunkGen;

template <class Callable, class... Args>
class SqThunkGen<Callable, void(Args...)>
{
public:
  template <SQInteger startIdx>
  static SQInteger Func(HSQUIRRELVM vm)
  {
    if (!vargs::check_var_types<Args...>(vm, startIdx))
      return SQ_ERROR;

    Callable *method;
    sq_getuserdata(vm, -1, (SQUserPointer *)&method, NULL);
    auto vars = vargs::make_vars<Args...>(vm, startIdx);
    vargs::apply(*method, vars);
    return 0;
  }
};

template <class Callable, class R, class... Args>
class SqThunkGen<Callable, R(Args...)>
{
public:
  template <SQInteger startIdx>
  static SQInteger Func(HSQUIRRELVM vm)
  {
    if (!vargs::check_var_types<Args...>(vm, startIdx))
      return SQ_ERROR;

    Callable *method;
    sq_getuserdata(vm, -1, (SQUserPointer *)&method, NULL);
    auto vars = vargs::make_vars<Args...>(vm, startIdx);
    PushVar(vm, vargs::apply(*method, vars));
    return 1;
  }
};

template<class Callable>
SQFUNCTION SqGlobalThunk()
{
  return &SqThunkGen<Callable>::template Func<2>;
}

template<class Callable>
SQFUNCTION SqMemberGlobalThunk()
{
  return &SqThunkGen<Callable>::template Func<1>;
}

}

#endif
