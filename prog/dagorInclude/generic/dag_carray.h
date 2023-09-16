//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <string.h>
#include <generic/dag_span.h>

#ifdef _DEBUG_TAB_
#define CARRAY_VALIDATE_IDX(idx)          \
  if (DAGOR_UNLIKELY((unsigned)idx >= N)) \
    span_index_assert("carray", idx, N, this, (const void *)&dptr, sizeof(T));
#else
#define CARRAY_VALIDATE_IDX(idx) ((void)0)
#endif

template <typename T, unsigned N>
class carray
{
public:
  typedef T value_type;
  typedef T *iterator;
  typedef const T *const_iterator;
  typedef T &reference;
  typedef const T &const_reference;

  typedef int (*typed_cmp_func_t)(const T *, const T *);
  static const unsigned static_size = N;

  /// Index operator
  __forceinline T &operator[](uint32_t idx)
  {
    CARRAY_VALIDATE_IDX(idx);
    return *(data() + idx);
  }

  /// Const index operator
  __forceinline const T &operator[](uint32_t idx) const
  {
    CARRAY_VALIDATE_IDX(idx);
    return *(data() + idx);
  }

  /// Explicit indexing
  const T &at(uint32_t idx) const
  {
    CARRAY_VALIDATE_IDX(idx);
    return *(data() + idx);
  }
  T &at(uint32_t idx)
  {
    CARRAY_VALIDATE_IDX(idx);
    return *(data() + idx);
  }

  T *data() { return dptr; }
  const T *data() const { return dptr; }

  static constexpr uint32_t size() { return N; }
  static constexpr uint32_t capacity() { return N; }

  //
  // STL compatibility
  //

  iterator begin() { return data(); }
  const_iterator begin() const { return data(); }
  const_iterator cbegin() const { return data(); }

  iterator end() { return data() + N; }
  const_iterator end() const { return data() + N; }
  const_iterator cend() const { return data() + N; }

  reference front() { return *data(); }
  const_reference front() const { return *data(); }
  reference back() { return *(data() + N - 1); }
  const_reference back() const { return *(data() + N - 1); }

  T dptr[N];
};

#ifdef _DEBUG_TAB_
#undef CARRAY_VALIDATE_IDX
#endif
