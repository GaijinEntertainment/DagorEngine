//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_assert.h>
#include <EASTL/utility.h>
#include <osApiWrappers/dag_atomic.h>
#include <supp/dag_define_KRNLIMP.h>


extern KRNLIMP bool g_destroy_init_on_demand_in_dtor;

template <typename T, bool ptr = true, typename B = bool>
struct InitOnDemand
{
public:
  InitOnDemand() = default;
  InitOnDemand(const InitOnDemand &) = delete;
  ~InitOnDemand()
  {
    if (g_destroy_init_on_demand_in_dtor)
      demandDestroy();
  }

  inline operator T *() const { return obj; }
  T &operator*() const { return *obj; }
  T *operator->() const { return obj; }
  T *get() const { return obj; }
  explicit operator bool() const { return obj != nullptr; }

  template <typename... Args>
  T *demandInit(Args &&...args)
  {
    if (!obj)
      obj = new T(eastl::forward<Args>(args)...);
    return obj;
  }
  void demandDestroy() { obj ? (delete obj, obj = nullptr) : (0); }

private:
  T *obj = nullptr;
};


#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable : 4582 4583) // warning C4582: 'InitOnDemand<...>::obj': constructor|destructor is not implicitly called
#endif

template <typename T, typename B>
struct InitOnDemand<T, false, B>
{
public:
  InitOnDemand() {}
  InitOnDemand(const InitOnDemand &) = delete;
  ~InitOnDemand() { demandDestroy(); }

  // Note: access before initialization or after deinitialization is not alloved because of type-safety
  // If you need explicit check use get() for that
#ifdef _DEBUG_TAB_
  operator T *() const
  {
    G_FAST_ASSERT(inited);
    return getObj();
  }
  T &operator*() const
  {
    G_FAST_ASSERT(inited);
    return *getObj();
  }
  T *operator->() const
  {
    G_FAST_ASSERT(inited);
    return getObj();
  }
#else
  operator T *() const { return getObj(); }
  T &operator*() const { return *getObj(); }
  T *operator->() const { return getObj(); }
#endif
  T *get() const { return ((bool)*this) ? getObj() : nullptr; }
  explicit operator bool() const
  {
    if constexpr (eastl::is_volatile_v<B>)
      return interlocked_relaxed_load(inited);
    else
      return inited;
  }

  template <typename... Args>
  T *demandInit(Args &&...args)
  {
    if (!inited)
    {
      new (objBuf, _NEW_INPLACE) T(eastl::forward<Args>(args)...);
      if constexpr (eastl::is_volatile_v<B>)
        interlocked_release_store(inited, true);
      else
        inited = true;
    }
    return getObj();
  }
  void demandDestroy()
  {
    if constexpr (eastl::is_volatile_v<B>)
    {
      if (!interlocked_exchange(inited, false))
        return;
    }
    else if (!eastl::exchange(inited, false))
      return;
    getObj()->~T();
  }

private:
  T *getObj() const { return const_cast<T *>(&obj); }

private:
  union
  {
    alignas(T) char objBuf[sizeof(T)];
    T obj;
  };
  B inited = false;
};

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif

#include <supp/dag_undef_KRNLIMP.h>
