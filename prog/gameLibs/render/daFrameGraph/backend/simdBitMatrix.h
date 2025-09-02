// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMath.h>
#include <dag/dag_vector.h>
#include <EASTL/string.h>

#include <backend/simdBitView.h>


namespace dafg
{

template <class T, class A>
class SimdBitVector;

template <class EnumType, class Alloc>
class SimdBitMatrix
{
  // TODO: it is likely that we will only ever have triangular matrices
  // in any of our algorithms, so it should be possible to optimize this
  // for that case by not storing the upper half of the matrix.
  // But that makes the math for indexing a lot more complicated,
  // so we'll leave that for later.
public:
  SimdBitMatrix(size_t sideLen, bool value = false) :
    sideLength{sideLen}, rowSizeInWords((sideLen + SIMD_WORD_BIT_SIZE - 1) / SIMD_WORD_BIT_SIZE), storage(rowSizeInWords * sideLen)
  {
    memset(storage.data(), value ? ~0 : 0, storage.size() * sizeof(SimdWord));
  }

  void setDiagonal()
  {
    for (size_t i = 0; i < sideLength; ++i)
    {
      const auto ptr = storage.data() + i * rowSizeInWords + i / SIMD_WORD_BIT_SIZE;
      const auto o = i % SIMD_WORD_BIT_SIZE;
      // Icky and ugly, as we don't have bit manipulation intrinsics
      // available on all our hardware yet. So we manually shift bits to
      // construct a vector with the o-th bit set to 1. and the rest
      // being 0.
      // clang-format off
      v_sti(ptr, v_make_vec4i(
                   o < 32  ? 1 << (o -  0) : 0,
        32 <= o && o < 64  ? 1 << (o - 32) : 0,
        64 <= o && o < 96  ? 1 << (o - 64) : 0,
        96 <= o            ? 1 << (o - 96) : 0));
      // clang-format on
    }
  }

  bool test(EnumType ey, EnumType ex) const
  {
    const auto x = eastl::to_underlying(ex);
    const auto y = eastl::to_underlying(ey);
    const auto ptr = reinterpret_cast<const SimdSubWord *>(storage.data() + y * rowSizeInWords) + x / SIMD_SUB_WORD_BIT_SIZE;
    SimdSubWord subword;
    memcpy(&subword, ptr, sizeof(SimdSubWord));

    const auto offset = x % SIMD_SUB_WORD_BIT_SIZE;
    return (subword >> offset) & 1;
  }

  void set(EnumType ey, EnumType ex, bool value)
  {
    const auto x = eastl::to_underlying(ex);
    const auto y = eastl::to_underlying(ey);
    auto ptr = reinterpret_cast<SimdSubWord *>(storage.data() + y * rowSizeInWords) + x / SIMD_SUB_WORD_BIT_SIZE;
    SimdSubWord subword;
    memcpy(&subword, ptr, sizeof(SimdSubWord));

    const auto offset = x % SIMD_SUB_WORD_BIT_SIZE;
    subword = (subword & ~(SimdSubWord{1} << offset)) | (static_cast<SimdSubWord>(value) << offset);
    memcpy(ptr, &subword, sizeof(SimdSubWord));
  }

  SimdBitView<EnumType, true> row(EnumType ei) const
  {
    const auto i = eastl::to_underlying(ei);
    G_FAST_ASSERT(i < sideLength);
    return SimdBitView<EnumType, true>(storage.data() + i * rowSizeInWords, sideLength);
  }

  SimdBitView<EnumType, false> row(EnumType ei)
  {
    const auto i = eastl::to_underlying(ei);
    G_FAST_ASSERT(i < sideLength);
    return SimdBitView<EnumType, false>(storage.data() + i * rowSizeInWords, sideLength);
  }

  eastl::string dump() const
  {
    eastl::string result;
    result.reserve(sideLength * sideLength);
    for (size_t i = 0; i < sideLength; ++i)
    {
      result.append_sprintf("%3zu ", i);
      for (size_t j = 0; j < sideLength; ++j)
        result.append(1, j <= i && test(static_cast<EnumType>(i), static_cast<EnumType>(j)) ? '#' : '_');
      result.append(1, '\n');
    }
    return result;
  }

private:
  size_t sideLength;
  size_t rowSizeInWords;
  dag::Vector<SimdWord, Alloc> storage;
};

} // namespace dafg
