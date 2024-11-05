// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/vector.h>


template <typename T>
class DerivedSpan
{
  using BytePointerType = typename eastl::conditional<eastl::is_const<T>::value, const uint8_t *, uint8_t *>::type;
  BytePointerType uBase = nullptr;
  size_t uSize = 0;
  size_t uCount = 0;
  BytePointerType atIndex(size_t i) const { return &uBase[i * uSize]; }

public:
  DerivedSpan() = default;
  DerivedSpan(const DerivedSpan &) = default;
  template <typename U>
  DerivedSpan(U *u_base, size_t u_count) : uBase{reinterpret_cast<BytePointerType>(u_base)}, uSize{sizeof(U)}, uCount{u_count}
  {
    static_assert(eastl::is_base_of<T, U>::value, "U is invalid type");
  }
  template <typename U>
  DerivedSpan(const dag::Vector<U> &u_base) : DerivedSpan{u_base.data(), u_base.size()}
  {}
  class Iterator
  {
    BytePointerType uBase = nullptr;
    size_t uSize = 0;

  public:
    Iterator() = default;
    Iterator(const Iterator &) = default;
    Iterator(BytePointerType u_base, size_t u_size) : uBase{u_base}, uSize{u_size} {}

    friend bool operator==(const Iterator &l, const Iterator &r) { return l.uBase == r.uBase; }
    friend bool operator!=(const Iterator &l, const Iterator &r) { return !(l == r); }

    Iterator &operator++()
    {
      uBase += uSize;
      return *this;
    }
    Iterator operator++(int) const
    {
      auto other = *this;
      ++other;
      return other;
    }

    Iterator &operator--()
    {
      uBase -= uSize;
      return *this;
    }
    Iterator operator--(int) const
    {
      auto other = *this;
      --other;
      return other;
    }
    T &operator*() const { return *reinterpret_cast<T *>(uBase); }
  };

  Iterator begin() const { return {uBase, uSize}; }
  Iterator cbegin() const { return begin(); }
  Iterator end() const { return {atIndex(uCount), uSize}; }
  Iterator cend() const { return end(); }
  size_t size() const { return uCount; }
  T *data() const { return reinterpret_cast<T *>(uBase); }
};
