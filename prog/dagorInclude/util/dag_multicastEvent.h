//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_function.h>


// A proof token that you have bound a callback to a multicast event.
// As soon as the token is destroyed, your callback will be unbound.
class CallbackToken
{
  template <class, size_t, size_t>
  friend class MulticastEvent;

public:
  CallbackToken() = default;

  CallbackToken(CallbackToken &&other) : cleanuper{eastl::exchange(other.cleanuper, nullptr)}, event{other.event}, index{other.index}
  {}

  CallbackToken &operator=(CallbackToken &&other)
  {
    if (this == &other)
      return *this;

    destroy();
    cleanuper = eastl::exchange(other.cleanuper, nullptr);
    event = other.event;
    index = other.index;

    return *this;
  }

  ~CallbackToken() { destroy(); }

  explicit operator bool() { return cleanuper != nullptr; }

private:
  void destroy()
  {
    if (cleanuper)
      cleanuper(event, index);
  }

  void (*cleanuper)(void *, size_t) = nullptr;
  void *event = nullptr;
  size_t index = 0;
};


template <class Signature, size_t fixedCallbackCount = 4, size_t fixedCallbackSize = 64>
class MulticastEvent;


template <class... Args, size_t fixedCallbackCount, size_t fixedCallbackSize>
class MulticastEvent<void(Args...), fixedCallbackCount, fixedCallbackSize>
{
  size_t findFreeSlot()
  {
    if (!storage.has_overflowed())
      return storage.size();

    size_t result = 0;
    while (result < storage.size() && storage[result])
      ++result;

    return result;
  }

public:
  template <class F>
  [[nodiscard]] CallbackToken subscribe(F &&f)
  {
    const size_t index = findFreeSlot();

    if (index == storage.size())
      storage.emplace_back();

    storage[index] = eastl::forward<F>(f);

    CallbackToken result;
    result.cleanuper = +[](void *self, size_t idx) { reinterpret_cast<MulticastEvent *>(self)->storage[idx] = {}; };
    result.event = this;
    result.index = index;
    return result;
  }

  void fire(Args... args)
  {
    for (auto &cb : storage)
      if (cb)
        cb(std::forward<Args>(args)...);
  }

private:
  eastl::fixed_vector<eastl::fixed_function<fixedCallbackSize, void(Args...)>, fixedCallbackCount> storage;
};
