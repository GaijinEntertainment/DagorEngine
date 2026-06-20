//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_finally.h>
#include <EASTL/type_traits.h>

#if DAGOR_EXCEPTIONS_ENABLED

#include <debug/dag_debug.h>
#include <debug/dag_except.h>
#include <EASTL/utility.h>
#include <exception>

#endif


namespace detail
{

#if DAGOR_EXCEPTIONS_ENABLED
template <typename F>
struct SuppressExceptionsWrapper
{
  F func;

  void operator()()
  {
    try
    {
      func();
    }
    catch (const DagorException &e)
    {
      char stackInfo[8192];
      ::stackhlp_get_call_stack(stackInfo, sizeof(stackInfo), e.getStackPtr(), 32);
      logerr("FinallySafe: DagorException caught: %s\n%s", e.excDesc, stackInfo);
    }
    catch (const std::exception &e)
    {
      logerr("FinallySafe: std::exception caught: %s", e.what());
    }
    catch (...)
    {
      logerr("FinallySafe: unknown exception caught");
    }
  }
};
#endif

template <typename F>
static auto wrap_and_suppress_exceptions(F &&func)
{
#if DAGOR_EXCEPTIONS_ENABLED
  return SuppressExceptionsWrapper<eastl::decay_t<F>>{eastl::forward<F>(func)};
#else
  return eastl::forward<F>(func);
#endif
}

} // namespace detail


template <typename T>
class FinallySafe
{
public:
  explicit FinallySafe(T &&func) : cleanup(detail::wrap_and_suppress_exceptions(eastl::move(func))) {}
  void release() { cleanup.release(); }

private:
  using TWrapped = eastl::decay_t<decltype(detail::wrap_and_suppress_exceptions(eastl::declval<T>()))>;
  Finally<TWrapped> cleanup;
};
