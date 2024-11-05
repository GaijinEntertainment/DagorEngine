// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMath.h>
#include <dag/dag_vector.h>

#include <backend/simdBitView.h>


namespace dabfg
{

template <class EnumType, class Alloc>
class SimdBitVector
{
public:
  SimdBitVector(size_t len, bool value = false) : length{len}, storage((len + SIMD_WORD_BIT_SIZE - 1) / SIMD_WORD_BIT_SIZE)
  {
    memset(storage.data(), value ? ~0 : 0, storage.size() * sizeof(SimdWord));
  }

  bool test(EnumType ei) const
  {
    const auto i = eastl::to_underlying(ei);
    const auto ptr = reinterpret_cast<const SimdSubWord *>(storage.data()) + i / SIMD_SUB_WORD_BIT_SIZE;
    SimdSubWord subword;
    memcpy(&subword, ptr, sizeof(SimdSubWord));
    const auto offset = i % SIMD_SUB_WORD_BIT_SIZE;
    return (subword >> offset) & 1;
  }

  void set(EnumType ei, bool value)
  {
    const auto i = eastl::to_underlying(ei);
    auto ptr = reinterpret_cast<SimdSubWord *>(storage.data()) + i / SIMD_SUB_WORD_BIT_SIZE;
    SimdSubWord subword;
    memcpy(&subword, ptr, sizeof(SimdSubWord));

    const auto offset = i % SIMD_SUB_WORD_BIT_SIZE;
    subword = (subword & ~(SimdSubWord{1} << offset)) | (static_cast<SimdSubWord>(value) << offset);
    memcpy(ptr, &subword, sizeof(SimdSubWord));
  }

  void operator|=(const SimdBitView<EnumType, true> &other) { view() |= other; }

  SimdBitView<EnumType, true> view() const { return SimdBitView<EnumType, true>(storage.data(), length); }
  SimdBitView<EnumType, false> view() { return SimdBitView<EnumType, false>(storage.data(), length); }

  operator SimdBitView<EnumType, true>() const { return view(); }
  operator SimdBitView<EnumType, false>() { return view(); }

private:
  size_t length;
  dag::Vector<SimdWord, Alloc> storage;
};

} // namespace dabfg
