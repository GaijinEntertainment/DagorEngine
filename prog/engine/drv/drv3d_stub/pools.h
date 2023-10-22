#pragma once

#include <generic/dag_tab.h>
#include <osApiWrappers/dag_critSec.h>

static WinCritSec dummy_crit;

template <class T>
class GenDynPool
{
  WinCritSec pool_crit;

public:
  GenDynPool() : ent(midmem), entUuIdx(midmem) {}
  ~GenDynPool() { clear_all_ptr_items(ent); }

  T *add()
  {
    WinAutoLock lock(pool_crit);
    if (!entUuIdx.size())
    {
      T *tex = new T;
      ent.push_back(tex);
      return tex;
    }

    const int idx = entUuIdx.back();
    entUuIdx.pop_back();
    return ent[idx];
  }

  void del(T *e)
  {
    WinAutoLock lock(pool_crit);
    for (int i = ent.size() - 1; i >= 0; i--)
      if (ent[i] == e)
      {
        entUuIdx.push_back(i);
        return;
      }
  }

  inline void reserve(int cnt)
  {
    WinAutoLock lock(pool_crit);
    ent.reserve(cnt - ent.size());
    entUuIdx.reserve(cnt - entUuIdx.size());
  }
  inline int entCnt() { return ent.size() - entUuIdx.size(); }
  inline int entMax() { return ent.size(); }

protected:
  Tab<T *> ent;
  Tab<int> entUuIdx;
};


template <class T>
class GenConstPool
{
  WinCritSec pool_crit;

public:
  GenConstPool() : ent(midmem), entUuIdx(midmem) {}

  T *add()
  {
    WinAutoLock lock(pool_crit);
    if (!entUuIdx.size())
      return NULL;

    int idx = entUuIdx.back();
    entUuIdx.pop_back();
    return &ent[idx];
  }

  bool del(T *e)
  {
    WinAutoLock lock(pool_crit);
    for (int i = ent.size() - 1; i >= 0; i--)
      if (&ent[i] == e)
      {
        entUuIdx.push_back(i);
        return true;
      }
    return false;
  }

  inline void init(int size)
  {
    WinAutoLock lock(pool_crit);
    ent.resize(size);
    entUuIdx.resize(size);
    for (int j(0), i = size - 1; i >= 0; i--)
      entUuIdx[i] = j++;
  }

  inline bool freeSpace() { return entUuIdx.size() > 0; }

  inline void reserve(int cnt)
  {
    WinAutoLock lock(pool_crit);
    ent.reserve(cnt - ent.size());
    entUuIdx.reserve(cnt - entUuIdx.size());
  }
  inline int entCnt() { return ent.size() - entUuIdx.size(); }
  inline int entMax() { return ent.size(); }

protected:
  Tab<T> ent;
  Tab<int> entUuIdx;
};


#define DECLARE_TEX_CREATOR(T)                          \
  static T *create()                                    \
  {                                                     \
    T *tex = texConstPool.add();                        \
    if (tex)                                            \
      return tex;                                       \
                                                        \
    if (!useTexDynPool)                                 \
      fatal("texture max count reached");               \
                                                        \
    return texDynPool.add();                            \
  }                                                     \
  static void dispose(T *p)                             \
  {                                                     \
    if (!texConstPool.del(p))                           \
      texDynPool.del(p);                                \
  }                                                     \
                                                        \
  static void initPool(int size)                        \
  {                                                     \
    texConstPool.init(size);                            \
  }                                                     \
  static void reserve(int cnt)                          \
  {                                                     \
    texDynPool.reserve(cnt - texConstPool.entMax());    \
  }                                                     \
  static int entCnt()                                   \
  {                                                     \
    return texConstPool.entCnt() + texDynPool.entCnt(); \
  }                                                     \
  static int entMax()                                   \
  {                                                     \
    return texConstPool.entMax() + texDynPool.entMax(); \
  }                                                     \
                                                        \
protected:                                              \
  static GenConstPool<T> texConstPool;                  \
  static GenDynPool<T> texDynPool


#define STATIC_TEX_POOL_DECL(T)    \
  GenConstPool<T> T::texConstPool; \
  GenDynPool<T> T::texDynPool


#define DECLARE_BUF_CREATOR(T)                                         \
  static T *create(int sz, bool allocate, int flags, const char *name) \
  {                                                                    \
    T *p = dynPool.add();                                              \
    p->size = sz;                                                      \
    p->flags = flags;                                                  \
    p->allocBuf(allocate);                                             \
    p->setResName(name);                                               \
    return p;                                                          \
  }                                                                    \
  static void dispose(T *p)                                            \
  {                                                                    \
    p->freeBuf();                                                      \
    dynPool.del(p);                                                    \
  }                                                                    \
                                                                       \
  static void reserve(int cnt)                                         \
  {                                                                    \
    dynPool.reserve(cnt);                                              \
  }                                                                    \
  static int entCnt()                                                  \
  {                                                                    \
    return dynPool.entCnt();                                           \
  }                                                                    \
  static int entMax()                                                  \
  {                                                                    \
    return dynPool.entMax();                                           \
  }                                                                    \
                                                                       \
protected:                                                             \
  static GenDynPool<T> dynPool


#define STATIC_BUF_POOL_DECL(T) GenDynPool<T> T::dynPool
