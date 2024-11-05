//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// file implements relocatable vector for relocatable values
// with fixed_vector like semantics (allowing overflow).
// on the contrary to fixed_vector, total sizeof isn't bigger than
// sizeof(Counter) (+padding) + max(sizeof(T)*inplace_count, sizeof(void*))
//
// so, RelocatableFixedVector<int, 7, int> will be of 8*4 = 32 bytes
// so, RelocatableFixedVector<int16, 7, int16> will be of 8*2 = 16 bytes size
// as soon as we can't put inplace we will start allocate on heap
// min allocation size is max(16 bytes, (inplace_count + 1)*sizeof(T)),
// and we use exponential growth of pow of 2
// capacity on heap is >= min allocation size

#include <dag/dag_relocatable.h>
#include <util/dag_compilerDefs.h>
#include <EASTL/memory.h>
#include <EASTL/initializer_list.h>
#include <memory/dag_genMemAlloc.h>
#include <string.h> //memmove

#ifndef BOUNDS_CHECK_ASSERT_RETURN

#ifdef _DEBUG_TAB_
#include <debug/dag_assert.h>
#define BOUNDS_CHECK_ASSERT_RETURN  G_ASSERT_RETURN
#define BOUNDS_CHECK_ASSERTF_RETURN G_ASSERTF_RETURN
#define BOUNDS_CHECK_ASSERT         G_ASSERT
#define BOUNDS_CHECK_ASSERTF        G_ASSERTF
#define DAG_VALIDATE_IDX(idx)                                                                            \
  if (EASTL_UNLIKELY((unsigned)idx >= (unsigned)size()))                                                 \
    G_ASSERTF(0, "RelocatableFixedVector<T,%d%s>[%d] size=%d this=%p data=%p sizeof(T)=%d", static_size, \
      canOverflow ? ", canOverflow" : "", idx, size(), this, (void *)data(), sizeof(T));
#else
#define BOUNDS_CHECK_ASSERT_RETURN(...)
#define BOUNDS_CHECK_ASSERTF_RETURN(...)
#define BOUNDS_CHECK_ASSERT(...)
#define BOUNDS_CHECK_ASSERTF(...)
#define DAG_VALIDATE_IDX(...)
#endif

#endif

#define CHECK_RELOCATABLE() static_assert(dag::is_type_relocatable<T>::value, "currently non-relocatable types are not working");
#define CHECK_RELOCATABLE_OR_INPLACE() \
  static_assert(!canOverflow || dag::is_type_relocatable<T>::value, "currently non-relocatable types are not working");


#ifndef IF_CONSTEXPR
#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define IF_CONSTEXPR if constexpr
#else
#define IF_CONSTEXPR if
#endif
#endif

namespace dag
{

template <size_t padding_size>
struct PaddingStruct
{
  char padding[padding_size];
}; // ensure pointer alignment with padding
template <>
struct PaddingStruct<0>
{};

template <typename T1, typename T2>
constexpr size_t max_alignof()
{
  return alignof(T1) > alignof(T2) ? alignof(T1) : alignof(T2);
}

template <typename T, size_t inplace_count, bool allow_overflow = true, typename Allocator = MidmemAlloc, typename Counter = uint32_t,
  bool zero_init_scalars = true>
class alignas(max_alignof<T, void *>()) RelocatableFixedData // base class, allows shallow copy
{
public:
  typedef Allocator allocator_type;
  typedef T value_type;
  typedef T *pointer;
  typedef const T *const_pointer;
  typedef T &reference;
  typedef const T &const_reference;
  typedef value_type *iterator;
  typedef const value_type *const_iterator;
  typedef eastl::reverse_iterator<iterator> reverse_iterator;
  typedef eastl::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef Counter size_type;
  typedef ptrdiff_t difference_type;

  static_assert(alignof(void *) >= alignof(size_type), "alignof(Counter) is abnormally high");
  static constexpr size_type static_size = inplace_count;
  static constexpr bool canOverflow = allow_overflow;

  bool empty() const { return used() == 0; }
  size_type size() const { return used(); }
  size_type capacity() const { return isInplace() ? inplaceCapacity() : heapCapacity(); }

  const value_type *data() const { return isInplace() ? inplaceData() : heap.data; }
  const_iterator begin() const { return data(); }
  const_iterator end() const { return begin() + used(); }
  const_iterator cbegin() const { return begin(); }
  const_iterator cend() const { return end(); }

  value_type &front()
  {
    DAG_VALIDATE_IDX(0);
    return *data();
  }
  value_type &back()
  {
    DAG_VALIDATE_IDX(0);
    return data()[used() - 1];
  }
  const value_type &front() const
  {
    DAG_VALIDATE_IDX(0);
    return *data();
  }
  const value_type &back() const
  {
    DAG_VALIDATE_IDX(0);
    return data()[used() - 1];
  }

protected:
#pragma pack(push, 1)
  struct AllocatorAndCount : public Allocator
  {
    size_type count = 0;
  } mAllocatorAndCount;

  static constexpr int ALLOC_CNT_SZ = sizeof(mAllocatorAndCount);
  static_assert((ALLOC_CNT_SZ % alignof(size_type)) == 0, "struct Allocator has abnormal size/alignment");

  struct Heap : PaddingStruct<(32 * sizeof(value_type *) - ALLOC_CNT_SZ - sizeof(size_type)) % sizeof(value_type *)>
  {
    size_type capacity;
    value_type *data;
  };
  struct Storage : PaddingStruct<(32 * alignof(T) - ALLOC_CNT_SZ) % alignof(T)>
  {
    char data[inplace_count * sizeof(value_type)];
  };

  union
  {
    Storage inplaceStor;
    Heap heap;
  };
#pragma pack(pop)

  static __forceinline bool is_inplace(size_t cnt)
  {
    return !canOverflow || cnt <= inplace_count; //-V560 A part of conditional expression is always false: !canOverflow.
  }
  bool isHeap() const { return !is_inplace(used()); }
  bool isInplace() const { return is_inplace(used()); }

  size_type &used() { return mAllocatorAndCount.count; }
  size_type used() const { return mAllocatorAndCount.count; }
  size_type heapCapacity() const { return heap.capacity; }
  static size_type inplaceCapacity() { return (size_type)inplace_count; }

  const allocator_type &get_allocator() const { return mAllocatorAndCount; }
  allocator_type &get_allocator() { return mAllocatorAndCount; }

  value_type *inplaceData() { return (value_type *)inplaceStor.data; }
  const value_type *inplaceData() const { return (const value_type *)inplaceStor.data; }

  value_type *data() { return isInplace() ? inplaceData() : heap.data; }
  iterator begin() { return data(); }
  iterator end() { return begin() + used(); }
  static constexpr size_t max(size_t a, size_t b) { return a > b ? a : b; }

  void doInsertAt(size_t at, size_t add_cnt, value_type *dst, const value_type *src)
  {
    memmove((void *)&dst[at + add_cnt], (const void *)&src[at], sizeof(value_type) * (size() - at));
  }

  DAGOR_NOINLINE
  value_type *insertAt(size_t at, size_t add_cnt, value_type *dst, const value_type *src)
  {
    memmove((void *)dst, (const void *)src, sizeof(value_type) * at);
    doInsertAt(at, add_cnt, dst, src);
    return dst;
  }

  size_t calcUsedSize() const
  {
    return isHeap() ? offsetof(RelocatableFixedData, heap) + sizeof(heap) : uintptr_t((void *)end()) - uintptr_t((void *)this);
  }

  template <bool z>
  static typename eastl::enable_if<z, value_type *>::type defCtor(void *m)
  {
    return new (m, _NEW_INPLACE) value_type();
  } // this ctor zero-fills POD scalars
  template <bool z>
  static typename eastl::disable_if<z, value_type *>::type defCtor(void *m)
  {
    return new (m, _NEW_INPLACE) value_type;
  } // this ctor leaves POD scalars as if (but calls default ctor for non-POD)
  static void defInitN(void *m, size_t n)
  {
    for (value_type *dest = (value_type *)m; n > 0; n--, dest++)
      defCtor<zero_init_scalars>(dest);
  }
};

template <typename T, size_t inplace_count, bool allow_overflow = true, typename Allocator = MidmemAlloc, typename Counter = uint32_t,
  bool zero_init_scalars = true>
class RelocatableFixedVector : public RelocatableFixedData<T, inplace_count, allow_overflow, Allocator, Counter, zero_init_scalars>
{
public:
  typedef RelocatableFixedData<T, inplace_count, allow_overflow, Allocator, Counter, zero_init_scalars> base_type;
  typedef base_type shallow_copy_t;

  // Name lookup is unable to find these names in a dependent base class,
  // so we re-define them here. Note that `using typename base_type::...;`
  // does not work on MSVC due to a compiler bug.
  typedef typename base_type::value_type value_type;
  typedef typename base_type::pointer pointer;
  typedef typename base_type::const_pointer const_pointer;
  typedef typename base_type::reference reference;
  typedef typename base_type::const_reference const_reference;
  typedef typename base_type::iterator iterator;
  typedef typename base_type::const_iterator const_iterator;
  typedef typename base_type::reverse_iterator reverse_iterator;
  typedef typename base_type::const_reverse_iterator const_reverse_iterator;
  typedef typename base_type::size_type size_type;
  typedef typename base_type::difference_type difference_type;

  using base_type::back;
  using base_type::begin;
  using base_type::calcUsedSize;
  using base_type::canOverflow;
  using base_type::capacity;
  using base_type::cbegin;
  using base_type::cend;
  using base_type::data;
  using base_type::empty;
  using base_type::end;
  using base_type::front;
  using base_type::get_allocator;
  using base_type::size;
  using base_type::static_size;

  ~RelocatableFixedVector() { clear(); }
  RelocatableFixedVector() = default;
  RelocatableFixedVector(size_type sz) { resize(sz); }
  RelocatableFixedVector(RelocatableFixedVector &&a)
  {
    CHECK_RELOCATABLE();
    memcpy(this, &a, a.calcUsedSize());
    a.used() = 0;
  }
  template <typename V, typename VT = typename V::value_type, typename U = T,
    typename B = typename eastl::enable_if<eastl::is_same<VT const, U const>::value, bool>::type>
  explicit RelocatableFixedVector(const V &v)
  {
    allocateAndCopy(v.size(), v.data());
  }

  RelocatableFixedVector(std::initializer_list<value_type> ilist) { allocateAndCopy(eastl::size(ilist), eastl::data(ilist)); }

  RelocatableFixedVector &operator=(RelocatableFixedVector &&a);
  RelocatableFixedVector(const RelocatableFixedVector &a) { allocateAndCopy(a.size(), a.data()); }
  RelocatableFixedVector &operator=(const RelocatableFixedVector &a)
  {
    if (&a == this)
      return *this;
    clear();
    allocateAndCopy(a.size(), a.data());
    return *this;
  }
  template <typename V, typename VT = typename V::value_type, typename U = T,
    typename B = typename eastl::enable_if<eastl::is_same<VT const, U const>::value, bool>::type>
  RelocatableFixedVector &operator=(const V &v)
  {
    clear();
    allocateAndCopy(v.size(), v.data());
    return *this;
  }
  void swap(RelocatableFixedVector &a);

  void clear()
  {
    for (auto e = end() - 1, b = begin() - 1; e != b; --e)
      e->~T();
    if (base_type::isHeap())
      deallocate(base_type::heap.data);
    base_type::used() = 0;
  }
  void resize(size_type size);
  void *push_back_uninitialized(size_t add_count = 1);
  void *insert_uninitialized(const_iterator at, size_t add_count = 1);

  iterator insert(const_iterator at, value_type &&v)
  {
    if (EASTL_LIKELY(data() > &v || data() + capacity() <= &v))
      return new (insert_uninitialized(at), _NEW_INPLACE) value_type((value_type &&) v);
    value_type tmp((value_type &&) v);
    return new (insert_uninitialized(at), _NEW_INPLACE) value_type((value_type &&) tmp);
  }
  iterator insert(const_iterator at, const value_type &v) { return new (insert_uninitialized(at), _NEW_INPLACE) value_type(v); }
  iterator insert(const_iterator at, const value_type *first, const value_type *last);
  iterator insert(const_iterator at, size_t n, const value_type &v)
  {
    size_t idx = at - begin();
    value_type *dest = (value_type *)insert_uninitialized(at, n);
    BOUNDS_CHECK_ASSERT_RETURN(dest, nullptr);
    for (size_t i = n; i > 0; i--, dest++)
      new (dest, _NEW_INPLACE) value_type(v);
    return begin() + idx;
  }
  iterator insert_default(const_iterator at, size_t n)
  {
    value_type *dest = (value_type *)insert_uninitialized(at, n);
    BOUNDS_CHECK_ASSERT_RETURN(dest, nullptr);
    base_type::defInitN(dest, n);
    return dest;
  }
  template <class... Args>
  iterator emplace(const_iterator at, Args &&...args)
  {
    return new (insert_uninitialized(at), _NEW_INPLACE) value_type(eastl::forward<Args>(args)...);
  }

  value_type &push_back() { return *base_type::template defCtor<zero_init_scalars>(push_back_uninitialized()); }
  value_type &push_back(const value_type &v) { return *(new (push_back_uninitialized(), _NEW_INPLACE) value_type(v)); }
  value_type &push_back(value_type &&v) { return *(new (push_back_uninitialized(), _NEW_INPLACE) value_type((value_type &&) v)); }
  iterator append_default(size_t n)
  {
    value_type *dest = (value_type *)push_back_uninitialized(n);
    BOUNDS_CHECK_ASSERT_RETURN(dest, nullptr);
    base_type::defInitN(dest, n);
    return dest;
  }
  template <class... Args>
  value_type &emplace_back(Args &&...args)
  {
    return *new (push_back_uninitialized(), _NEW_INPLACE) value_type(eastl::forward<Args>(args)...);
  }

  iterator erase(const_iterator at, const_iterator till);
  iterator erase(const_iterator at) { return erase(at, at + 1); }
  iterator erase_unsorted(const_iterator at, const_iterator till);
  iterator erase_unsorted(const_iterator at) { return erase_unsorted(at, at + 1); }
  void pop_back()
  {
    BOUNDS_CHECK_ASSERT(size() != 0);
    data()[--base_type::used()].~value_type();
  }
  void assign(size_type n, const value_type &value);
  void assign(const value_type *s, const value_type *e);
  void shrink_to_fit()
  {
    if (base_type::isHeap() && size() < base_type::heapCapacity())
      reallocateToCapacity(size());
  }
  void reserve(size_t v)
  {
    IF_CONSTEXPR (!canOverflow)
      if (v > base_type::inplaceCapacity())
      {
        BOUNDS_CHECK_ASSERTF(v <= base_type::inplaceCapacity(), "reserve(%u) inplaceCapacity=%u", v, base_type::inplaceCapacity());
        v = base_type::inplaceCapacity();
      }
    if (base_type::isHeap() && v > base_type::heapCapacity()) // we can't reserve not heap-allocated
      reallocateToCapacity(v);
  }
  void set_capacity(size_t v)
  {
    if (size() > v)
      resize(v);
    if (base_type::isHeap()) // we can't reserve not heap-allocated
      reallocateToCapacity(v);
  }

  value_type &operator[](uint32_t idx)
  {
    DAG_VALIDATE_IDX(idx);
    return data()[idx];
  }
  const value_type &operator[](uint32_t idx) const
  {
    DAG_VALIDATE_IDX(idx);
    return data()[idx];
  }

  value_type &at(uint32_t n) { return this->operator[](n); }
  const value_type &at(uint32_t n) const { return this->operator[](n); }

  // this copy is a very weak ref.
  // If data is allocated on heap, copy valid only as long, as heap hasn't been reallocated. If it was allocated inplace - it will be
  // 'old' data, even if inserted however, it is arguably even better than using vector. It will reallocate in less cases (as long as
  // data is inplace, it is just "old", but valid data)
  shallow_copy_t getShallowCopy() const { return (shallow_copy_t) * this; }

protected:
  static constexpr size_t min_allocate_size_bytes = 16;                         // none of allocators can allocate less than 16 bytes
  static constexpr size_t min_allocate_size_mask = min_allocate_size_bytes - 1; // none of allocators can allocate less than 16 bytes
  static size_t minHeapSize(size_t required)
  {
    return base_type::max(required, min_allocate_size_bytes / sizeof(value_type)); // no reason to allocate less than 16 bytes
  }
  static size_t growthSize(size_t currentCapacity, size_t newCapacity)
  {
    return base_type::max(size_t(currentCapacity + (currentCapacity + 1) / 2), newCapacity); // always use 3/2 growth, (on a bigger
                                                                                             // values pow2 is better, but we suppose
                                                                                             // to not use 4k elements+)
  }
  void allocateAndCopy(size_type count, const value_type *p) // no Free! internal function to avoid copy-paste!
  {
    IF_CONSTEXPR (!canOverflow)
      if (count > base_type::inplaceCapacity())
      {
        BOUNDS_CHECK_ASSERTF(count <= base_type::inplaceCapacity(), "allocateAndCopy(count=%u) inplaceCapacity=%u", count,
          base_type::inplaceCapacity());
        count = base_type::inplaceCapacity();
      }

    value_type *dst = base_type::inplaceData();
    if (!base_type::is_inplace(count))
      dst = base_type::heap.data = allocateCapacity(base_type::heap.capacity = count);

    base_type::used() = count;
    for (auto end = dst + count; dst != end; ++dst, ++p)
      new (dst, _NEW_INPLACE) value_type(*p); // zero initializing scalars
  }

  void deallocate(value_type *p) { base_type::mAllocatorAndCount.deallocate(p, 0); }
  value_type *allocateCapacity(size_type capacity)
  {
    return (value_type *)base_type::mAllocatorAndCount.allocate(capacity * sizeof(value_type));
  }
  value_type *reallocateToCapacity(value_type *p, size_type capacity)
  {
    return (value_type *)base_type::mAllocatorAndCount.realloc(p, capacity * sizeof(value_type));
  }

  value_type *reallocateToCapacity(size_type new_capacity)
  {
    CHECK_RELOCATABLE_OR_INPLACE();
    base_type::heap.capacity = new_capacity;
    return base_type::heap.data =
             (value_type *)base_type::mAllocatorAndCount.realloc(base_type::heap.data, new_capacity * sizeof(value_type));
  }
};

template <typename T, size_t N, bool O, typename A, typename C, bool Z>
inline void RelocatableFixedVector<T, N, O, A, C, Z>::assign(const value_type *s, const value_type *e)
{
  const size_t newCount = e - s;
  value_type *cData = data();
  if (newCount > capacity())
  {
    BOUNDS_CHECK_ASSERT(newCount > base_type::inplaceCapacity());
    clear();
    cData = base_type::heap.data = allocateCapacity(base_type::heap.capacity = newCount);
  }
  const size_t oldCount = size();
  for (auto i = data(), ie = data() + (newCount < oldCount ? newCount : oldCount); i != ie; ++i, ++s)
    *i = *s;
  if (newCount > size())
    for (auto i = cData + oldCount, ie = cData + newCount; i != ie; ++i, ++s)
      new (i, _NEW_INPLACE) value_type(*s); // unitialized copy
  else
    for (auto i = cData + newCount, ie = cData + oldCount; i != ie; ++i)
      i->~value_type();
  base_type::used() = newCount;
}

template <typename T, size_t N, bool O, typename A, typename C, bool Z>
inline void RelocatableFixedVector<T, N, O, A, C, Z>::assign(size_type newCount, const value_type &v)
{
  value_type *cData = data();
  if (newCount > capacity())
  {
    BOUNDS_CHECK_ASSERT(newCount > base_type::inplaceCapacity());
    clear();
    cData = base_type::heap.data = allocateCapacity(base_type::heap.capacity = newCount);
  }
  const size_t oldCount = size();

  for (auto i = cData, ie = cData + (newCount < oldCount ? newCount : oldCount); i != ie; ++i)
    *i = v;
  if (newCount > oldCount)
    for (auto i = cData + oldCount, ie = cData + newCount; i != ie; ++i)
      new (i, _NEW_INPLACE) value_type(v); // unitialized copy
  else
    for (auto i = cData + newCount, ie = cData + oldCount; i != ie; ++i)
      i->~value_type();
  base_type::used() = newCount;
}

template <typename T, size_t N, bool O, typename A, typename C, bool Z>
inline RelocatableFixedVector<T, N, O, A, C, Z> &RelocatableFixedVector<T, N, O, A, C, Z>::operator=(RelocatableFixedVector &&a)
{
  if (this == &a)
    return *this;
  clear();
  CHECK_RELOCATABLE();
  memcpy(this, &a, a.calcUsedSize());
  a.used() = 0;
  return *this;
}

template <typename T, size_t N, bool O, typename A, typename C, bool Z>
void RelocatableFixedVector<T, N, O, A, C, Z>::swap(RelocatableFixedVector &a)
{
  if (this == &a)
    return;
  char tmp_stor[sizeof(RelocatableFixedVector)];
  size_t this_sz = this->calcUsedSize(), a_sz = a.calcUsedSize();
  // G_FAST_ASSERT(this_sz <= sizeof(tmp_stor) && a_sz <= sizeof(tmp_stor));
  CHECK_RELOCATABLE();
  if (a_sz <= this_sz)
  {
    memcpy(tmp_stor, this, this_sz);
    memcpy(this, &a, a_sz);
    memcpy(&a, tmp_stor, this_sz);
  }
  else
  {
    memcpy(tmp_stor, &a, a_sz);
    memcpy(&a, this, this_sz);
    memcpy(this, tmp_stor, a_sz);
  }
}


template <typename T, size_t N, bool O, typename A, typename C, bool Z>
inline void RelocatableFixedVector<T, N, O, A, C, Z>::resize(size_type newCount)
{
  if (size() == newCount)
    return;
  IF_CONSTEXPR (!canOverflow)
    if (newCount > base_type::inplaceCapacity())
    {
      BOUNDS_CHECK_ASSERTF(newCount <= base_type::inplaceCapacity(), "resize(%u) inplaceCapacity=%u", newCount,
        base_type::inplaceCapacity());
      newCount = base_type::inplaceCapacity();
    }
  value_type *s = base_type::data();
  for (auto i = s + newCount, e = s + size(); i < e; ++i)
    i->~value_type();

  IF_CONSTEXPR (canOverflow)
  {
    const size_type copyCount = newCount < size() ? newCount : size();
    const bool wasInplace = base_type::isInplace();
    const bool willBeInplace = base_type::is_inplace(newCount);
    CHECK_RELOCATABLE_OR_INPLACE()
    if (wasInplace != willBeInplace)
    {
      value_type *newData = willBeInplace ? base_type::inplaceData() : allocateCapacity(newCount);
      memcpy((void *)newData, (void *)s, copyCount * sizeof(value_type));
      if (willBeInplace)
      {
        deallocate(s);
      }
      else
      {
        // has to be done after memcpy!
        base_type::heap.data = newData;
        base_type::heap.capacity = newCount;
      }
      s = newData;
    }
    else if (!willBeInplace && base_type::heapCapacity() < newCount)
      s = reallocateToCapacity(newCount);
  }

  if (newCount > size())
    base_type::defInitN(s + size(), newCount - size());
  base_type::used() = newCount;
}

template <typename T, size_t N, bool O, typename A, typename C, bool Z>
inline void *RelocatableFixedVector<T, N, O, A, C, Z>::push_back_uninitialized(size_t add_count)
{
  value_type *s = base_type::data();
  const size_t idx = base_type::used();
  const size_t newCount = base_type::used() + add_count;
  BOUNDS_CHECK_ASSERTF_RETURN(canOverflow || newCount <= base_type::inplaceCapacity(), nullptr,
    "push_back(num=%d) while size()=%u and inplaceCapacity=%u", add_count, base_type::used(), base_type::inplaceCapacity());
  IF_CONSTEXPR (!canOverflow)
  {
#ifndef _DEBUG_TAB_ // otherwise it was checked in BOUNDS_CHECK_ASSERTF_RETURN
    if (newCount > base_type::inplaceCapacity())
      return nullptr;
#endif
  }
  else
  {
    CHECK_RELOCATABLE_OR_INPLACE()
    const bool wasInplace = base_type::isInplace();
    const bool willBeInplace = base_type::is_inplace(newCount);
    if (wasInplace != willBeInplace)
    {
      auto newCapacity = minHeapSize(newCount);
      value_type *newHeapData = allocateCapacity(newCapacity);
      memcpy((void *)newHeapData, (void *)s, sizeof(value_type) * idx);
      base_type::heap.data = newHeapData;
      base_type::heap.capacity = newCapacity; // has to be performed in next string, to avoid aliasing
      s = base_type::heap.data;
    }
    else if (!willBeInplace && base_type::heapCapacity() < newCount)
      s = reallocateToCapacity(growthSize(base_type::heapCapacity(), newCount));
  }
  base_type::used() += add_count;
  return s + idx;
}


template <typename T, size_t N, bool O, typename A, typename C, bool Z>
inline void *RelocatableFixedVector<T, N, O, A, C, Z>::insert_uninitialized(const_iterator at, size_t add_count)
{
  value_type *s = base_type::data();
  const size_t idx = at - s;
  const size_t newCount = base_type::used() + add_count;

  BOUNDS_CHECK_ASSERTF_RETURN(size_type(idx) <= size(), (iterator)at, "idx=%u size()=%u", idx, size());
  BOUNDS_CHECK_ASSERTF_RETURN(size_type(base_type::used() + add_count) >= add_count, (void *)at, "size()=%u add_count=%u",
    base_type::used(), add_count);
  BOUNDS_CHECK_ASSERTF_RETURN(canOverflow || newCount <= base_type::inplaceCapacity(), nullptr,
    "insert(num=%u) while size()=%u inplaceCapacity=%u", add_count, base_type::used(), base_type::inplaceCapacity());
  IF_CONSTEXPR (!canOverflow)
  {
#ifndef _DEBUG_TAB_ // otherwise it was checked in BOUNDS_CHECK_ASSERTF_RETURN
    if (newCount > base_type::inplaceCapacity())
      return nullptr;
#endif
    base_type::doInsertAt(idx, add_count, s, s);
  }
  else
  {
    CHECK_RELOCATABLE();
    const bool wasInplace = base_type::isInplace();
    const bool willBeInplace = base_type::is_inplace(newCount);

    if (wasInplace != willBeInplace)
    {
      auto newCapacity = minHeapSize(newCount);
      base_type::heap.data = base_type::insertAt(idx, add_count, allocateCapacity(newCapacity), s);
      base_type::heap.capacity = newCapacity; // has to be performed in next string, to avoid aliasing
      s = base_type::heap.data;
    }
    else
    {
      if (!willBeInplace && base_type::heapCapacity() < newCount)
        s = reallocateToCapacity(growthSize(base_type::heapCapacity(), newCount));
      base_type::doInsertAt(idx, add_count, s, s);
    }
  }
  base_type::used() += add_count;
  return s + idx;
}

template <typename T, size_t N, bool O, typename A, typename C, bool Z>
inline typename RelocatableFixedVector<T, N, O, A, C, Z>::iterator RelocatableFixedVector<T, N, O, A, C, Z>::insert(const_iterator at,
  const value_type *first, const value_type *last)
{
  const size_t addCount = last - first;
  if (!addCount)
    return (iterator)at;
  if (first < data() + capacity() && last > data())
  {
    BOUNDS_CHECK_ASSERTF(first >= cend() || last <= cbegin(), "insert(%p, %p, %p) overlaps with begin=%p .. end=%p", at, first, last,
      data(), data() + capacity());
    return nullptr;
  }
  if (value_type *dest = (value_type *)insert_uninitialized(at, addCount))
  {
    for (value_type *i = dest, *e = i + addCount; i != e; ++i, ++first)
      new (i, _NEW_INPLACE) value_type(*first);
    return dest;
  }
  return nullptr;
}

template <typename T, size_t N, bool O, typename A, typename C, bool Z>
inline typename RelocatableFixedVector<T, N, O, A, C, Z>::iterator RelocatableFixedVector<T, N, O, A, C, Z>::erase(const_iterator at,
  const_iterator till)
{
  if (EASTL_UNLIKELY(at == till))
    return const_cast<iterator>(at);
  value_type *oldData = data();
  const size_t idx = at - oldData;
  const size_t delCount = till - at;
  const size_t newCount = size() - delCount;
  // bounds check
  BOUNDS_CHECK_ASSERTF_RETURN(delCount && size() >= delCount && idx + delCount <= size(), begin(), "idx=%u delCount=%u size()=%u", idx,
    delCount, size());
  const bool wasInplace = base_type::isInplace();
  const bool willBeInplace = base_type::is_inplace(newCount);
  for (value_type *od = oldData + idx, *ed = od + delCount; od < ed; od++)
    od->~value_type();

  value_type *newData = (wasInplace != willBeInplace) ? base_type::inplaceData() : oldData; // can only become inplace
  const size_t nextIdx = idx + delCount;
  CHECK_RELOCATABLE();
  if (oldData != newData) // begining of data
    memcpy((void *)newData, (void *)oldData, idx * sizeof(value_type));
  memmove((void *)(newData + idx), (void *)(oldData + nextIdx), (size() - nextIdx) * sizeof(value_type)); // tail of data

  if (wasInplace != willBeInplace) // begining of data
    deallocate(oldData);
  base_type::used() -= delCount;
  return newData + idx;
}

template <typename T, size_t N, bool O, typename A, typename C, bool Z>
inline typename RelocatableFixedVector<T, N, O, A, C, Z>::iterator RelocatableFixedVector<T, N, O, A, C, Z>::erase_unsorted(
  const_iterator at, const_iterator till)
{
  value_type *oldData = data();
  const size_t idx = at - oldData;
  const size_t delCount = till - at;
  BOUNDS_CHECK_ASSERTF_RETURN(delCount && size() >= delCount && idx + delCount <= size(), begin(), "idx=%u delCount=%u size()=%u", idx,
    delCount, size());
  const size_t newCount = size() - delCount;
  // bounds check
  const bool wasInplace = base_type::isInplace();
  const bool willBeInplace = base_type::is_inplace(newCount);

  CHECK_RELOCATABLE();
  value_type *newData = (wasInplace != willBeInplace) ? base_type::inplaceData() : oldData; // can only become inplace
  if (oldData != newData)                                                                   // begining of data
    memcpy((void *)newData, (void *)oldData, newCount * sizeof(value_type));

  bool no_overlap = (idx + delCount <= newCount);
  IF_CONSTEXPR (is_type_relocatable<value_type>::value)
  {
    for (value_type *od = oldData + idx, *ed = od + delCount; od < ed; od++)
      od->~value_type();
    if (no_overlap)
      memcpy((void *)(newData + idx), (void *)(oldData + newCount), sizeof(value_type) * delCount);
    else
      memcpy((void *)(newData + idx), (void *)(oldData + idx + delCount), sizeof(value_type) * (newCount - idx));
  }
  else if (no_overlap)
  {
    for (value_type *od = oldData + newCount, *ed = od + delCount, *nd = newData + idx; od < ed; od++, nd++)
    {
      *nd = (value_type &&) * od;
      od->~value_type();
    }
  }
  else
  {
    for (value_type *od = oldData + (delCount + idx), *nd = newData + idx, *ed = newData + newCount; nd < ed; od++, nd++)
    {
      *nd = (value_type &&) * od;
      od->~value_type();
    }
    for (value_type *od = oldData + newCount, *ed = oldData + (delCount + idx); od < ed; od++)
      od->~value_type();
  }

  if (wasInplace != willBeInplace) // begining of data
    deallocate(oldData);
  base_type::used() -= delCount;
  return newData + idx;
}


template <typename T, size_t N, bool O, typename A, typename C, bool Z>
struct is_type_relocatable<RelocatableFixedData<T, N, O, A, C, Z>, typename eastl::enable_if_t<is_type_relocatable<T>::value>>
  : public eastl::true_type
{};

template <typename T, size_t N, bool O, typename A, typename C, bool Z>
struct is_type_relocatable<RelocatableFixedVector<T, N, O, A, C, Z>, typename eastl::enable_if_t<is_type_relocatable<T>::value>>
  : public eastl::true_type
{};

} // namespace dag

#undef DAG_VALIDATE_IDX
#undef CHECK_RELOCATABLE
#undef CHECK_RELOCATABLE_OR_INPLACE
