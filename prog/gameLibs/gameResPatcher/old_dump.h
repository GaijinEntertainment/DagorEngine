// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#if _TARGET_64BIT
#error this compatibility module is used to load old 32-bit dumps from 32-bit code only
#endif

#include <generic/dag_span.h>

template <class T>
class PatchableTab32 : protected dag::Span<T>
{
public:
  using typename dag::Span<T>::value_type;
  using typename dag::Span<T>::iterator;
  using typename dag::Span<T>::const_iterator;
  using typename dag::Span<T>::reference;
  using typename dag::Span<T>::const_reference;
  using dag::Span<T>::operator[];
  using dag::Span<T>::at;
  using dag::Span<T>::begin;
  using dag::Span<T>::cbegin;
  using dag::Span<T>::end;
  using dag::Span<T>::cend;
  using dag::Span<T>::front;
  using dag::Span<T>::back;
  using dag::Span<T>::data;
  using dag::Span<T>::size;
  using dag::Span<T>::capacity;
  using dag::Span<T>::empty;

  PatchableTab32() {}
  ~PatchableTab32() {}

  void patch(void *base)
  {
    if (dag::Span<T>::dcnt)
      dag::Span<T>::dptr = (T *)(ptrdiff_t(dag::Span<T>::dptr) + ptrdiff_t(base));
  }

  void init(void *base, intptr_t cnt)
  {
    dag::Span<T>::dptr = (T *)base;
    dag::Span<T>::dcnt = cnt;
  }

private:
  PatchableTab32(const PatchableTab32 &ft);
  PatchableTab32 &operator=(const PatchableTab32 &ft);
};

template <class T>
class PatchablePtr32
{
public:
  PatchablePtr32() {}
  ~PatchablePtr32() {}

  T *get() const { return p; }
  operator T *() const { return p; }

  const T *operator->() const { return p; }
  T *operator->() { return p; }

  void patch(void *base) { p = int(ptrdiff_t(p) & 0xFFFFFFFF) >= 0 ? (T *)((ptrdiff_t(p) & 0xFFFFFFFF) + ptrdiff_t(base)) : NULL; }

  void operator=(const T *new_p) { p = (T *)new_p; }

private:
  T *p;
};

struct RoNameMap32
{
public:
  PatchableTab32<PatchablePtr32<const char>> map;

  void patchData(void *base)
  {
    map.patch(base);
    for (int i = 0; i < map.size(); i++)
      map[i].patch(base);
  }
};

struct VirtualRomFsData32
{
  RoNameMap32 files;
  PatchableTab32<PatchableTab32<const char>> data;
};

struct DxpRec32
{
  int ofs;
  void *tex;
  int texId;
  int packedDataSize;
};
struct DxpDump32
{
  RoNameMap32 texNames;
  PatchableTab32<ddsx::Header> texHdr;
  PatchableTab32<DxpRec32> texRec;
};
