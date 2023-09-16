//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>

/// @addtogroup containers
/// @{

/// Template array that has method for patching pointer address.
/// Based on dag::Span. Intended for patching arrays in serialized dump of data.
template <class T>
class PatchableTab : protected dag::Span<T>
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

  PatchableTab() {}
  ~PatchableTab() {}

  PatchableTab(PatchableTab &&ft) = default;
  PatchableTab &operator=(PatchableTab &&ft) = default;

  /// Add specified base address to data address (pointer).
  void patch(void *base)
  {
#if _TARGET_64BIT
#if _TARGET_BE
    dag::Span<T>::dcnt = intptr_t(dag::Span<T>::dptr) & 0xFFFFFFFF;
    dag::Span<T>::dptr = dag::Span<T>::dcnt ? (T *)((ptrdiff_t(dag::Span<T>::dptr) >> 32) + ptrdiff_t(base)) : NULL;
#else
    dag::Span<T>::dcnt = intptr_t(dag::Span<T>::dptr) >> 32;
    dag::Span<T>::dptr = dag::Span<T>::dcnt ? (T *)((ptrdiff_t(dag::Span<T>::dptr) & 0xFFFFFFFF) + ptrdiff_t(base)) : NULL;
#endif
#else
    if (dag::Span<T>::dcnt)
      dag::Span<T>::dptr = (T *)(ptrdiff_t(dag::Span<T>::dptr) + ptrdiff_t(base));
#endif
  }

  /// Rebases already patched data address (useful for fast cloning)
  void rebase(void *newbase, const void *oldbase)
  {
    if (dag::Span<T>::dcnt)
      dag::Span<T>::dptr = (T *)(ptrdiff_t(dag::Span<T>::dptr) + ptrdiff_t(newbase) - ptrdiff_t(oldbase));
  }

  /// Explicitly init dag::Span (useful when when constructing PatchableTab not from load only)
  void init(void *base, intptr_t cnt)
  {
    dag::Span<T>::dptr = (T *)base;
    dag::Span<T>::dcnt = cnt;
  }

private:
  PatchableTab(const PatchableTab &ft);
  PatchableTab &operator=(const PatchableTab &ft);

#if !_TARGET_64BIT
  int _resv[2];
#endif
};
/// @}

#pragma pack(push, 4) // this pointers are not aligned at least in RoNameMap::map
template <class T>
class PatchablePtr
{
public:
  PatchablePtr() {} //-V730
  ~PatchablePtr() {}

  T *get() const { return p; }
  operator T *() const { return p; }

  const T *operator->() const { return p; }
  T *operator->() { return p; }

  /// reinterprets initial value as int (must be used only before patch!)
  int toInt() const { return int(intptr_t(p)); }

  /// Add specified base address to data address (pointer).
  void patch(void *base) { p = int(ptrdiff_t(p) & 0xFFFFFFFF) >= 0 ? (T *)((ptrdiff_t(p) & 0xFFFFFFFF) + ptrdiff_t(base)) : NULL; }

  /// Add specified base address to data address (pointer). When offset is 0, pointer is treated as NULL
  void patchNonNull(void *base)
  {
    p = int(ptrdiff_t(p) & 0xFFFFFFFF) > 0 ? (T *)((ptrdiff_t(p) & 0xFFFFFFFF) + ptrdiff_t(base)) : NULL;
  }

  /// Rebases already patched data address (useful for fast cloning)
  void rebase(void *newbase, const void *oldbase) { p = p ? (T *)(ptrdiff_t(p) + ptrdiff_t(newbase) - ptrdiff_t(oldbase)) : NULL; }

  /// Expicitly assigns new pointer
  void operator=(const T *new_p) { p = (T *)new_p; }
  void setPtr(const void *new_p) { p = (T *)new_p; }

#if _TARGET_64BIT
  void clearUpperBits() {}
#else
  void clearUpperBits() { _resv = 0; }
#endif

private:
  T *p;

#if !_TARGET_64BIT
  int _resv;
#endif
};
#pragma pack(pop)

#if _TARGET_64BIT
#define PATCHABLE_64BIT_PAD32(NM) int NM
#define PATCHABLE_32BIT_PAD32(NM)
#define PATCHABLE_DATA64(T, NM) T NM
enum
{
  PATCHABLE_64BIT_PAD32_SZ = 4,
  PATCHABLE_32BIT_PAD32_SZ = 0,
};
#else
#define PATCHABLE_64BIT_PAD32(NM)
#define PATCHABLE_32BIT_PAD32(NM) int NM
#define PATCHABLE_DATA64(T, NM) \
  T NM;                         \
  int _##NM##HDW;
enum
{
  PATCHABLE_64BIT_PAD32_SZ = 0,
  PATCHABLE_32BIT_PAD32_SZ = 4,
};
#endif
