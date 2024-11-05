// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>
#include <vecmath/dag_vecMath.h>
#include <generic/dag_span.h>


namespace dabfg
{

using SimdWord = vec4i;
inline constexpr size_t SIMD_WORD_BIT_SIZE = sizeof(SimdWord) * CHAR_BIT;
using SimdSubWord = uint32_t;
inline constexpr size_t SIMD_SUB_WORD_BIT_SIZE = sizeof(SimdSubWord) * CHAR_BIT;

template <class EnumType, bool isConst>
class SimdBitView
{
  using Word = eastl::conditional_t<isConst, const SimdWord, SimdWord>;

  template <class, bool>
  friend class SimdBitView;

public:
  SimdBitView(Word *data, size_t len) : length{len}, storage(data, (len + SIMD_WORD_BIT_SIZE - 1) / SIMD_WORD_BIT_SIZE) {}
  template <class DUMMY = void, class = eastl::enable_if_t<isConst, DUMMY>>
  SimdBitView(const SimdBitView<EnumType, false> &other) : length{other.length}, storage{other.storage}
  {}

  size_t size() const { return length; }

  SimdBitView<EnumType, isConst> first(size_t count)
  {
    G_FAST_ASSERT(count <= length);
    return SimdBitView<EnumType, isConst>(storage.data(), count);
  }

  template <class DUMMY = void, class = eastl::enable_if_t<!isConst, DUMMY>>
  void operator|=(const SimdBitView<EnumType, true> &other)
  {
    G_FAST_ASSERT(other.length <= length);
    const auto srcEndPtr = other.storage.data() + other.storage.size();
    const SimdWord *srcPtr = other.storage.data();
    for (SimdWord *dstPtr = storage.data(); srcPtr != srcEndPtr; ++srcPtr, ++dstPtr)
      *dstPtr = v_ori(*dstPtr, *srcPtr);
  }

private:
  size_t length;
  dag::Span<Word> storage;
};

} // namespace dabfg
