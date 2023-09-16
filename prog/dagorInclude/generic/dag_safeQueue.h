//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_debug.h>

//
// Thread safe RingQueue template
//
template <class T>
class SafeRingQueue
{
public:
  SafeRingQueue(const char *nm, int sz, class IMemAlloc *m = midmem)
  {
    name = nm;
    buf = new (m) T *[sz];
    memset(buf, 0, sizeof(T *) * sz);

    size = sz;
    wrPos = 0;
    rdPos = 0;
    used = 0;
    flags = 0;
  }

  ~SafeRingQueue()
  {
    size = 0;
    wrPos = 0;
    rdPos = 0;
    used = 0;
    if (buf)
      delete buf;
    buf = NULL;
  }

  bool put(T *p)
  {
    if (flags & FLG_AbortIncoming)
      return false;

    if (used + 1 >= size)
    {
      mt_debug_ctx("%s [queue=%p] size=%d used=%d wrPos=%d rdPos=%d", name, this, size, used, wrPos, rdPos);
      return false;
    }

    buf[wrPos] = p;
    wrPos++;
    if (wrPos >= size)
      wrPos = 0;
    interlocked_increment(used);

    return true;
  }

  void putOrDelete(T *p)
  {
    if (!put(p))
      delete p;
  }
  void putOrDestroyt(T *p)
  {
    if (!put(p))
      p->destroy();
  }

  int getUsed() { return used; }

  T *get()
  {
    if (flags & FLG_HoldOutcoming)
      return NULL;

    if (!used)
      return NULL;

    T *p = buf[rdPos];
    rdPos++;
    if (rdPos >= size)
      rdPos = 0;
    interlocked_decrement(used);

    return p;
  }

  T *getBlocking(int time_out = 100)
  {
    while (used < 1 && time_out > 0)
    {
      sleep_msec(1);
      time_out--;
    }

    return get();
  }

  T *getInfinite()
  {
    T *p = getBlocking();
    while (!p)
      p = getBlocking();
    return p;
  }

  T *peek(int idx)
  {
    if (used < idx)
      return NULL;

    idx += rdPos;
    if (idx >= size)
      idx -= size;

    return buf[idx];
  }

  void enablePut(bool en) { (en) ? flags &= ~FLG_AbortIncoming : flags |= FLG_AbortIncoming; }
  void enableGet(bool en) { (en) ? flags &= ~FLG_HoldOutcoming : flags |= FLG_HoldOutcoming; }

  bool isPutEnabled() { return flags & FLG_AbortIncoming; }
  bool isGetEnabled() { return flags & FLG_HoldOutcoming; }

  void destroyItems()
  {
    int saved_flags = flags;
    T *p;

    flags &= ~(FLG_AbortIncoming | FLG_HoldOutcoming);
    do
    {
      p = get();
      if (p)
        p->destroy();
    } while (p);

    flags = saved_flags;
  }

  void deleteItems()
  {
    int saved_flags = flags;
    T *p;

    flags &= ~(FLG_AbortIncoming | FLG_HoldOutcoming);
    do
    {
      p = get();
      if (p)
        delete p;
    } while (p);

    flags = saved_flags;
  }

  void removeItems()
  {
    int saved_flags = flags;
    T *p;

    flags &= ~(FLG_AbortIncoming | FLG_HoldOutcoming);
    do
      p = get();
    while (p);

    flags = saved_flags;
  }

protected:
  const char *name;
  int size, flags;
  T **buf;
  volatile int used, wrPos, rdPos;

  enum
  {
    FLG_AbortIncoming = 0x0001,
    FLG_HoldOutcoming = 0x0002,
  };
};

template <class T>
class SafePtr
{
protected:
  T *ptr;
  WinCritSec refCC;

public:
  SafePtr(const SafePtr &p) { init(p.ptr); }
  SafePtr(T *p = NULL) { init(p); }
  ~SafePtr()
  {
    WinAutoLock lock(&refCC);
    T *t = ptr;
    ptr = NULL;
    (t) ? t->delRef() : (void)0;
  }

  /// increment reference count
  __forceinline void addRef()
  {
    WinAutoLock lock(&refCC);
    (ptr) ? ptr->addRef() : (void)0;
  }

  /// decrement reference count
  __forceinline void delRef()
  {
    WinAutoLock lock(&refCC);
    (ptr) ? ptr->delRef() : (void)0;
  }

  /// initialize pointer. Use if constructor was not called for some reason.
  __forceinline void init(T *p = NULL)
  {
    ptr = p;
    (p) ? p->addRef() : (void)0;
  }

  T *get() const { return ptr; }

  operator T *() const { return ptr; }
  T &operator*() const { return *ptr; }
  T *operator->() const { return ptr; }

  SafePtr &operator=(const SafePtr &p)
  {
    WinAutoLock lock(&refCC);

    T *t = ptr;
    ptr = p.ptr;
    addRef();
    if (t)
      t->delRef();

    return *this;
  }


  /// call destroy() on object and set pointer to NULL
  void destroy()
  {
    WinAutoLock lock(&refCC);
    if (ptr)
    {
      T *t = ptr;
      ptr = NULL;
      if (t)
      {
        t->destroy();
        t->delRef();
      }
    }
  }
};
