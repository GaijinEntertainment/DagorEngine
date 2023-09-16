//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/type_traits.h>

template <typename T>
struct Finally
{
  static_assert(eastl::is_invocable_v<T>, "Finally accepts only valid lambda parameter");

public:
  explicit Finally(T &&func) : func(eastl::forward<T>(func)) {}
  ~Finally()
  {
    if (should_exit)
      func();
  }

  Finally(const Finally &) = delete;
  Finally &operator=(const Finally &) = delete;
  Finally(Finally &&) = delete;
  Finally &operator=(Finally &&) = delete;

  void release() { should_exit = false; }

private:
  T func;
  bool should_exit = true;
};

#define FINALLY_CONCAT_IMPL(x, y) x##y
#define FINALLY_CONCAT(x, y)      FINALLY_CONCAT_IMPL(x, y)
#define FINALLY(...)                           \
  Finally FINALLY_CONCAT(__finally_, __LINE__) \
  {                                            \
    __VA_ARGS__                                \
  }
