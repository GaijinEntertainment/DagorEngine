//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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
  static constexpr unsigned static_size = N;

  /// Index operator
  __forceinline constexpr T &operator[](uint32_t idx)
  {
    CARRAY_VALIDATE_IDX(idx);
    return *(data() + idx);
  }

  /// Const index operator
  __forceinline constexpr const T &operator[](uint32_t idx) const
  {
    CARRAY_VALIDATE_IDX(idx);
    return *(data() + idx);
  }

  /// Explicit indexing
  constexpr const T &at(uint32_t idx) const
  {
    CARRAY_VALIDATE_IDX(idx);
    return *(data() + idx);
  }
  constexpr T &at(uint32_t idx)
  {
    CARRAY_VALIDATE_IDX(idx);
    return *(data() + idx);
  }

  constexpr T *data() { return dptr; }
  constexpr const T *data() const { return dptr; }

  static constexpr uint32_t size() { return N; }
  static constexpr uint32_t capacity() { return N; }

  //
  // STL compatibility
  //

  constexpr iterator begin() { return data(); }
  constexpr const_iterator begin() const { return data(); }
  constexpr const_iterator cbegin() const { return data(); }

  constexpr iterator end() { return data() + N; }
  constexpr const_iterator end() const { return data() + N; }
  constexpr const_iterator cend() const { return data() + N; }

  constexpr reference front() { return *data(); }
  constexpr const_reference front() const { return *data(); }
  constexpr reference back() { return *(data() + N - 1); }
  constexpr const_reference back() const { return *(data() + N - 1); }

  T dptr[N];
};

#ifdef _DEBUG_TAB_
#undef CARRAY_VALIDATE_IDX
#endif
