//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daScript/daScript.h>
#include <daScript/simulate/aot.h>

namespace bind_dascript
{
das::StackAllocator &get_shared_stack();
}

template <typename ReturnType>
struct InvokeDas
{
  template <typename... ArgType>
  static __forceinline ReturnType invoke(das::Context *context, das::Func *func_ptr, ArgType... arg)
  {
    ReturnType ret{};
    context->tryRestartAndLock();
    if (!context->ownStack)
    {
      das::SharedStackGuard guard(*context, bind_dascript::get_shared_stack());
      das::das_try_recover(
        context,
        [&]() { ret = das::das_invoke_function<ReturnType>::template invoke<ArgType...>(context, nullptr, *func_ptr, arg...); },
        [&]() {
          context->stackWalk(&context->exceptionAt, false, false);
          logerr("unhandled exception: %s", context->getException());
        });
    }
    else
      das::das_try_recover(
        context,
        [&]() { ret = das::das_invoke_function<ReturnType>::template invoke<ArgType...>(context, nullptr, *func_ptr, arg...); },
        [&]() {
          context->stackWalk(&context->exceptionAt, false, false);
          logerr("unhandled exception: %s", context->getException());
        });
    context->unlock();
    return ret;
  }
};

template <>
struct InvokeDas<void>
{
  template <typename... ArgType>
  static __forceinline void invoke(das::Context *context, das::Func *func_ptr, ArgType... arg)
  {
    context->tryRestartAndLock();
    if (!context->ownStack)
    {
      das::SharedStackGuard guard(*context, bind_dascript::get_shared_stack());
      das::das_try_recover(
        context, [&]() { das::das_invoke_function<void>::invoke<ArgType...>(context, nullptr, *func_ptr, arg...); },
        [&]() {
          context->stackWalk(&context->exceptionAt, false, false);
          logerr("unhandled exception: %s", context->getException());
        });
    }
    else
      das::das_try_recover(
        context, [&]() { das::das_invoke_function<void>::invoke<ArgType...>(context, nullptr, *func_ptr, arg...); },
        [&]() {
          context->stackWalk(&context->exceptionAt, false, false);
          logerr("unhandled exception: %s", context->getException());
        });
    context->unlock();
  }
};
