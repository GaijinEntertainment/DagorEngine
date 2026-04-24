//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_assert.h>
#include <EASTL/type_traits.h>

namespace dag
{
struct StridedSpanConstruct
{
  void *base_ptr = nullptr;
  // just a convenience helper member, could be added to base_ptr directly
  uint32_t base_offset_in_bytes = 0;
  uint32_t stride_in_bytes = 0;
  uint32_t count = 0;
};

struct StridedSpanConstConstruct
{
  const void *base_ptr = nullptr;
  // just a convenience helper member, could be added to base_ptr directly
  uint32_t base_offset_in_bytes = 0;
  uint32_t stride_in_bytes = 0;
  uint32_t count = 0;
};

template <typename T>
class StridedSpan
{
  static constexpr bool is_constant = eastl::is_const_v<T>;
  using ConstT = eastl::add_const_t<T>;
  using BasePtrType = eastl::conditional_t<is_constant, const uint8_t *, uint8_t *>;
  using ReferenceType = eastl::add_lvalue_reference_t<T>;
  using ConstTeferenceType = eastl::add_lvalue_reference_t<ConstT>;
  using PointerType = eastl::add_pointer_t<T>;
  using ConstPointerType = eastl::add_pointer_t<ConstT>;

  BasePtrType base_ptr = nullptr;
  uint32_t element_stride = 0;
  uint32_t count = 0;

public:
  // stuff to make it compatible to std/eastl
  using element_type = T;
  using value_type = eastl::remove_cv_t<T>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = PointerType;
  using const_pointer = ConstPointerType;
  using reference = ReferenceType;
  using const_reference = ConstTeferenceType;

  StridedSpan() = default;
  ~StridedSpan() = default;

  StridedSpan(const StridedSpan &) = default;
  StridedSpan &operator=(const StridedSpan &) = default;

  StridedSpan(eastl::conditional_t<is_constant, const void *, void *> bp, uint32_t sz, uint32_t strd) :
    base_ptr{reinterpret_cast<BasePtrType>(bp)}, element_stride{strd}, count{sz}
  {}
  StridedSpan(const StridedSpanConstruct &construct) :
    StridedSpan{
      reinterpret_cast<BasePtrType>(construct.base_ptr) + construct.base_offset_in_bytes, construct.count, construct.stride_in_bytes}
  {}
  StridedSpan(const StridedSpanConstConstruct &construct) :
    StridedSpan{
      reinterpret_cast<BasePtrType>(construct.base_ptr) + construct.base_offset_in_bytes, construct.count, construct.stride_in_bytes}
  {}

  StridedSpan &operator=(const StridedSpanConstruct &construct)
  {
    base_ptr = reinterpret_cast<BasePtrType>(construct.base_ptr) + construct.base_offset_in_bytes;
    element_stride = construct.stride_in_bytes;
    count = construct.count;
    return *this;
  }
  StridedSpan &operator=(const StridedSpanConstConstruct &construct)
  {
    base_ptr = reinterpret_cast<BasePtrType>(construct.base_ptr) + construct.base_offset_in_bytes;
    element_stride = construct.stride_in_bytes;
    count = construct.count;
    return *this;
  }

  // little hack to make conversion to const span a little easier
  explicit operator StridedSpanConstConstruct() const
  {
    return {
      .base_ptr = base_ptr,
      .base_offset_in_bytes = 0,
      .stride_in_bytes = element_stride,
      .count = count,
    };
  }

  template <typename O>
  StridedSpan(const StridedSpan<O> &other) : StridedSpan{static_cast<StridedSpanConstConstruct>(other)}
  {
    G_STATIC_ASSERT((eastl::is_same_v<T, eastl::add_const_t<O>>));
  }

  template <typename O>
  StridedSpan &operator=(const StridedSpan<O> &other)
  {
    G_STATIC_ASSERT((eastl::is_same_v<T, eastl::add_const_t<O>>));
    *this = static_cast<StridedSpanConstConstruct>(other);
    return *this;
  }

  ReferenceType at(uint32_t index) { return *reinterpret_cast<PointerType>(base_ptr + (element_stride * index)); }
  ConstTeferenceType at(uint32_t index) const { return *reinterpret_cast<ConstPointerType>(base_ptr + (element_stride * index)); }
  ReferenceType operator[](uint32_t index) { return at(index); }
  ConstTeferenceType operator[](uint32_t index) const { return at(index); }

  // deliberately no data method, as it would break the spans whole concept

  uint32_t size() const { return count; }
  // it is valid to provide a stride of 0, which then allows to supply N values with just one T
  uint32_t size_bytes() const { return element_stride > 0 ? count * element_stride : sizeof(T); }
  // per element stride in bytes
  uint32_t stride_bytes() const { return element_stride; }
  // returns true when stride == sizeof(T)
  bool is_compact() const { return sizeof(T) == element_stride; }
  // return true when stride > sizeof(T)
  bool is_interleaved() const { return sizeof(T) < element_stride; }
  // return true when element_tride == 0
  bool is_single_instance() const { return 0 == element_stride; }

  class iterator
  {
  protected:
    StridedSpan *parent = nullptr;
    uint32_t index = 0;

    friend class const_iterator;

    iterator(StridedSpan *p, uint32_t i) : parent{p}, index{i} {}

  public:
    iterator() = default;
    ~iterator() = default;

    iterator(const iterator &) = default;
    iterator &operator=(const iterator &) = default;

    ReferenceType operator*() const { return parent->at(index); }
    PointerType operator->() const { return parent->at(index); }

    iterator &operator++()
    {
      ++index;
      return *this;
    }
    iterator operator++(int) const { return {parent, index + 1}; }
    iterator operator+(uint32_t value) const { return {parent, index + value}; }

    iterator &operator--()
    {
      --index;
      return *this;
    }
    iterator operator--(int) const { return {parent, index - 1}; }
    iterator operator-(uint32_t value) const { return {parent, index - value}; }
    bool operator==(const iterator &r) const { return parent == r.parent && index == r.index; }
    bool operator!=(const iterator &r) const { return !(*this == r); }
  };

  class const_iterator
  {
  protected:
    const StridedSpan *parent = nullptr;
    uint32_t index = 0;

    const_iterator(const StridedSpan *p, uint32_t i) : parent{p}, index{i} {}

  public:
    const_iterator() = default;
    ~const_iterator() = default;

    const_iterator(const const_iterator &) = default;
    const_iterator &operator=(const const_iterator &) = default;

    const_iterator(const iterator &other) : parent{other.parent}, index{other.index} {}

    const_iterator &operator=(const iterator &other)
    {
      parent = other.parent;
      index = other.index;
      return *this;
    }

    ConstTeferenceType operator*() const { return parent->at(index); }
    ConstPointerType operator->() const { return parent->at(index); }

    const_iterator &operator++()
    {
      ++index;
      return *this;
    }
    const_iterator operator++(int) const { return {parent, index + 1}; }
    const_iterator operator+(uint32_t value) const { return {parent, index + value}; }

    const_iterator &operator--()
    {
      --index;
      return *this;
    }
    const_iterator operator--(int) const { return {parent, index - 1}; }
    const_iterator operator-(uint32_t value) const { return {parent, index - value}; }
    bool operator==(const const_iterator &r) const { return parent == r.parent && index == r.index; }
    bool operator!=(const const_iterator &r) const { return !(*this == r); }
  };

  iterator begin() { return {this, 0}; }
  const_iterator begin() const { return {this, 0}; }
  const_iterator cbegin() const { return {this, 0}; }

  iterator end() { return {this, size()}; }
  const_iterator end() const { return {this, size()}; }
  const_iterator cend() const { return {this, size()}; }
};

template <typename T>
using StridedConstSpan = StridedSpan<eastl::add_const_t<T>>;

template <typename V, typename T>
inline StridedSpan<V> make_stride_span(T *ptr, uint32_t count, uint32_t ofs_in_bytes)
{
  return StridedSpanConstruct{
    .base_ptr = ptr,
    .base_offset_in_bytes = ofs_in_bytes,
    .stride_in_bytes = sizeof(T),
    .count = count,
  };
}

template <typename V, typename T>
inline StridedConstSpan<V> make_stride_span(const T *ptr, uint32_t count, uint32_t ofs_in_bytes)
{
  return StridedSpanConstConstruct{
    .base_ptr = ptr,
    .base_offset_in_bytes = ofs_in_bytes,
    .stride_in_bytes = sizeof(T),
    .count = count,
  };
}
} // namespace dag