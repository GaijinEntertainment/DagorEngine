// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "tagged_type.h"


// Various type-safe strong aliases for ints.
namespace drv3d_dx12
{

struct MipMapIndexTag;
using MipMapIndex = TaggedIndexType<uint8_t, MipMapIndexTag>;
using MipMapRange = TaggedRangeType<MipMapIndex>;
using MipMapCount = TaggedCountType<MipMapIndex>;

struct SubresourceIndexTag;
using SubresourceIndex = TaggedIndexType<uint32_t, SubresourceIndexTag>;
using SubresourceRange = TaggedRangeType<SubresourceIndex>;
using SubresourceCount = TaggedCountType<SubresourceIndex>;

struct ArrayLayerIndexTag;
using ArrayLayerIndex = TaggedIndexType<uint16_t, ArrayLayerIndexTag>;
using ArrayLayerRange = TaggedRangeType<ArrayLayerIndex>;
using ArrayLayerCount = TaggedCountType<ArrayLayerIndex>;

struct FormatPlaneIndexTag;
using FormatPlaneIndex = TaggedIndexType<uint8_t, FormatPlaneIndexTag>;
using FormatPlaneRange = TaggedRangeType<FormatPlaneIndex>;
using FormatPlaneCount = TaggedCountType<FormatPlaneIndex>;

class SubresourcePerFormatPlaneCount
{
public:
  using ValueType = SubresourceIndex::ValueType;

private:
  ValueType value{};

  constexpr SubresourcePerFormatPlaneCount(ValueType v) : value{v} {}

public:
  constexpr SubresourcePerFormatPlaneCount() = default;
  ~SubresourcePerFormatPlaneCount() = default;

  SubresourcePerFormatPlaneCount(const SubresourcePerFormatPlaneCount &) = default;
  SubresourcePerFormatPlaneCount &operator=(const SubresourcePerFormatPlaneCount &) = default;

  static constexpr SubresourcePerFormatPlaneCount make(ValueType v) { return {v}; }
  static constexpr SubresourcePerFormatPlaneCount make(MipMapCount mip, ArrayLayerCount layers)
  {
    return {ValueType(mip.count() * layers.count())};
  }
  static constexpr SubresourcePerFormatPlaneCount make(ArrayLayerCount layers, MipMapCount mip)
  {
    return {ValueType(mip.count() * layers.count())};
  }

  constexpr ValueType count() const { return value; }

  operator DagorSafeArg() const { return {count()}; }
};

inline SubresourcePerFormatPlaneCount operator*(const MipMapCount &l, const ArrayLayerCount &r)
{
  return SubresourcePerFormatPlaneCount::make(l, r);
}

inline SubresourcePerFormatPlaneCount operator*(const ArrayLayerCount &l, const MipMapCount &r)
{
  return SubresourcePerFormatPlaneCount::make(l, r);
}

// Per plane subres count times the plane index yields the subres range of the plane index
inline SubresourceRange operator*(const SubresourcePerFormatPlaneCount &l, const FormatPlaneIndex &r)
{
  return SubresourceRange::make(l.count() * r.index(), l.count());
}

// To keep it associative index times per plane yields also the subres range of the plane index
inline SubresourceRange operator*(const FormatPlaneIndex &l, const SubresourcePerFormatPlaneCount &r)
{
  return SubresourceRange::make(r.count() * l.index(), r.count());
}

// Per plane subres count times the plane count yields the total subres count
inline SubresourceCount operator*(const SubresourcePerFormatPlaneCount &l, const FormatPlaneCount &r)
{
  return SubresourceCount::make(l.count() * r.count());
}

inline SubresourceCount operator*(const FormatPlaneCount &r, const SubresourcePerFormatPlaneCount &l)
{
  return SubresourceCount::make(l.count() * r.count());
}

inline SubresourceIndex calculate_subresource_index(MipMapIndex mip, ArrayLayerIndex array, MipMapCount mips_per_array)
{
  return SubresourceIndex::make(mip.index() + (array.index() * mips_per_array.count()));
}

inline SubresourceIndex operator+(const SubresourceIndex &l, const SubresourcePerFormatPlaneCount &r)
{
  return SubresourceIndex::make(l.index() + r.count());
}

} // namespace drv3d_dx12
