// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_preprocessor.h>

namespace defer
{
namespace detail
{

template <typename TCallable>
class Defer
{
public:
  Defer(TCallable &&a_callable) : callable(a_callable) {}
  ~Defer() { callable(); }

private:
  TCallable callable;
};

} // namespace detail
} // namespace defer

#define DEFER_FUNC(func_)      defer::detail::Defer DAG_CONCAT(defer__, __COUNTER__)(func_)
#define DEFER(stmt_)           DEFER_FUNC([&]() { stmt_; })
#define DEFER_IN_METHOD(stmt_) DEFER_FUNC([&, this]() { stmt_; })