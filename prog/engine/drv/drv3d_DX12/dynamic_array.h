// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/span.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_globDef.h>


template <typename T>
class DynamicArray
{
  eastl::unique_ptr<T[]> ptr;
  size_t count = 0;

public:
  DynamicArray() = default;
  DynamicArray(const DynamicArray &) = delete;
  DynamicArray(DynamicArray &&) = default;
  DynamicArray(T *p, size_t sz) : ptr{p}, count{sz} {}
  DynamicArray(eastl::unique_ptr<T[]> &&p, size_t sz) : ptr{eastl::forward<eastl::unique_ptr<T[]>>(p)}, count{sz} {}
  explicit DynamicArray(size_t sz) : DynamicArray{eastl::make_unique<T[]>(sz), sz} {}
  DynamicArray &operator=(const DynamicArray &) = delete;
  DynamicArray &operator=(DynamicArray &&other)
  {
    eastl::swap(ptr, other.ptr);
    eastl::swap(count, other.count);
    return *this;
  }

  void adopt(T *new_ptr, size_t new_sz)
  {
    eastl::unique_ptr<T[]> newPtr{new_ptr};
    eastl::swap(ptr, newPtr);
    count = new_sz;
  }

  T *release()
  {
    count = 0;
    return ptr.release();
  }

  bool resize(size_t new_size)
  {
    if (count == new_size)
    {
      return false;
    }

    if (0 == new_size)
    {
      ptr.reset();
      count = 0;
      return true;
    }

    auto newBlock = eastl::make_unique<T[]>(new_size);
    if (!newBlock)
    {
      return false;
    }

    auto moveCount = min(count, new_size);
    for (uint32_t i = 0; i < moveCount; ++i)
    {
      newBlock[i] = eastl::move(ptr[i]);
    }
    eastl::swap(ptr, newBlock);
    count = new_size;
    return true;
  }

  T &operator[](size_t i) { return ptr[i]; }
  const T &operator[](size_t i) const { return ptr[i]; }
  size_t size() const { return count; }
  bool empty() const { return !ptr || 0 == count; }
  T *data() { return ptr.get(); }
  const T *data() const { return ptr.get(); }
  eastl::span<T> asSpan() { return {data(), size()}; }
  eastl::span<const T> asSpan() const { return {data(), size()}; }
  eastl::span<T> releaseAsSpan()
  {
    auto retValue = asSpan();
    release();
    return retValue;
  }
  static DynamicArray fromSpan(eastl::span<T> span) { return {span.data(), span.size()}; }
  T *begin() { return ptr.get(); }
  const T *begin() const { return ptr.get(); }
  const T *cbegin() const { return begin(); }
  T *end() { return begin() + size(); }
  const T *end() const { return begin() + size(); }
  const T *cend() const { return end(); }
};
