#pragma once
#include <generic/dag_fixedVectorSet.h>

template <int bytes>
struct SizeTypeBytes
{
  typedef uint32_t size_type_t;
};
template <>
struct SizeTypeBytes<2>
{
  typedef uint16_t size_type_t;
};
template <>
struct SizeTypeBytes<1>
{
  typedef uint8_t size_type_t;
};


template <class RelocatableKey, size_t inplace_count = 16 / sizeof(RelocatableKey),
  class SizeType = typename SizeTypeBytes<sizeof(RelocatableKey)>::size_type_t>
using InplaceKeyContainer = dag::RelocatableFixedVector<RelocatableKey, inplace_count, true, MidmemAlloc, SizeType>;

template <class RelocatableKey, size_t inplace_count = 16 / sizeof(RelocatableKey),
  class SizeType = typename SizeTypeBytes<sizeof(RelocatableKey)>::size_type_t>
using InplaceKeySet = dag::FixedVectorSet<RelocatableKey, inplace_count, true, eastl::use_self<RelocatableKey>, MidmemAlloc, SizeType>;
