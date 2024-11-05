//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_bitFlagsMask.h>
#include <debug/dag_assert.h>
#include <math/dag_adjpow2.h>
#include <EASTL/utility.h>


namespace rendinst
{

// Only first 12 bits are used for flags, the next 4 bits are used for forced LOD
enum class LayerFlag : uint16_t
{
  Opaque = 0x001,
  Transparent = 0x002,
  Decals = 0x004,
  Distortion = 0x008,
  ForGrass = 0x010,
  RendinstClipmapBlend = 0x020,
  RendinstHeightmapPatch = 0x080,
  NotExtra = 0x100,
  NoSeparateAlpha = 0x200,
  Stages = Decals | Transparent | Opaque | NotExtra,

  ALL_FLAGS =
    Opaque | Transparent | Decals | Distortion | ForGrass | RendinstClipmapBlend | RendinstHeightmapPatch | NotExtra | NoSeparateAlpha
};
using LayerFlags = BitFlagsMask<LayerFlag>;
BITMASK_DECLARE_FLAGS_OPERATORS(LayerFlag)

inline constexpr uint32_t get_layer_index(LayerFlag layer)
{
  // TODO: it would be nice to assert that only a single flag was passed,
  // but we don't really have constexpr-compatible assertions yet.
  return get_const_log2(eastl::to_underlying(layer));
}

namespace detail
{
constexpr size_t LAYER_FORCE_LOD_SHIFT = 12;
}

inline LayerFlags make_forced_lod_layer_flags(int lod)
{
  G_FAST_ASSERT(lod >= 0 && lod + 1 < (1 << 4));
  return static_cast<LayerFlag>((lod + 1) << detail::LAYER_FORCE_LOD_SHIFT);
}

inline constexpr int get_forced_lod(LayerFlags flags) { return (flags.asInteger() >> detail::LAYER_FORCE_LOD_SHIFT) - 1; }

} // namespace rendinst

template <>
struct BitFlagsTraits<rendinst::LayerFlag>
{
  static constexpr auto allFlags = rendinst::LayerFlag::ALL_FLAGS;
};

constexpr BitFlagsMask<rendinst::LayerFlag> operator~(rendinst::LayerFlag bit) { return ~make_bitmask(bit); }
