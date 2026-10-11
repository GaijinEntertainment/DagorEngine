// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitset.h>
#include <EASTL/span.h>
#include <EASTL/optional.h>
#include <EASTL/string_view.h>
#include <math/dag_bits.h>
#include <util/dag_globDef.h>
#include <generic/dag_expected.h>
#include <drv/3d/dag_consts.h>


template <size_t N>
inline eastl::bitset<N> &or_bit(eastl::bitset<N> &s, size_t i, bool b = true)
{
  return s.set(i, s.test(i) || b);
}

template <size_t N>
inline char *append_literal(char *at, char *ed, const char (&lit)[N])
{
  auto d = static_cast<size_t>(ed - at);
  auto c = min(d, N - 1);
  memcpy(at, lit, c);
  return at + c;
}

template <size_t N>
inline char *append_or_mask_value_name(char *beg, char *at, char *ed, const char (&name)[N])
{
  if (beg != at)
  {
    at = append_literal(at, ed, " | ");
  }
  return append_literal(at, ed, name);
}

template <typename T>
inline T align_value(T value, T alignment)
{
  return (value + alignment - 1) & ~(alignment - 1);
}

template <size_t N>
inline eastl::span<const char> string_literal_span(const char (&sl)[N])
{
  return {sl, N - 1};
}

template <size_t N>
inline eastl::span<const wchar_t> string_literal_span(const wchar_t (&sl)[N])
{
  return {sl, N - 1};
}

// Applies function object 'f' to each bit index of each set bit in bit mask 'm'.
template <typename F>
inline void for_each_set_bit(uint32_t m, F f)
{
  while (0 != m)
  {
    uint32_t i = __bsf_unsafe(m);
    m ^= 1u << i;
    f(i);
  }
}

inline constexpr D3D12_BOX make_full_region_meta_box() { return {}; }

inline constexpr bool is_full_region_meta_box(const D3D12_BOX &box)
{
  return 0 == (box.left | box.top | box.front | box.right | box.bottom | box.back);
}

template <typename T, typename E>
inline eastl::optional<E> as_optional_error(const dag::Expected<T, E> &e)
{
  if (e.has_value())
  {
    return {};
  }
  return e.error();
}

template <typename T, typename E>
inline eastl::optional<E> as_optional_error(dag::Expected<T, E> &e)
{
  if (e.has_value())
  {
    return {};
  }
  return eastl::move(e.error());
}

// Returns not decoded flags
inline ResourceBarrier decode_to_strings(ResourceBarrier barrier, auto &&target)
{
  if (RB_NONE == barrier)
  {
    target(eastl::string_view{"RB_NONE"});
    return RB_NONE;
  }
  struct BarrierNameTableEntry
  {
    eastl::underlying_type_t<ResourceBarrier> value;
    eastl::string_view name;
  };
  // table order is important as bits get removed when matched, for values with overlapping bits, the values with more bits have to
  // come first.
  static const BarrierNameTableEntry table[] = {
#define DEF_BARRIER(name)          {name, #name}
#define DEF_BARRIER2(name1, name2) {name1, #name1 " / " #name2}
    DEF_BARRIER(RB_RO_GENERIC_READ_BUFFER),
    DEF_BARRIER(RB_RO_GENERIC_READ_TEXTURE),
    DEF_BARRIER(RB_STAGE_ALL_SHADERS),
    DEF_BARRIER(RB_STAGE_ALL_GRAPHICS),
    DEF_BARRIER(RB_SOURCE_STAGE_ALL_SHADERS),
    DEF_BARRIER(RB_SOURCE_STAGE_ALL_GRAPHICS),
    DEF_BARRIER(RB_RO_CONSTANT_DEPTH_STENCIL_TARGET),
    DEF_BARRIER(RB_ALIAS_TO_AND_DISCARD),
    DEF_BARRIER(RB_ALIAS_ALL),
    DEF_BARRIER(RB_STREAM_OUTPUT_COUNTER),
    DEF_BARRIER(RB_STREAM_OUTPUT),
    DEF_BARRIER(RB_FLUSH_RAYTRACE_ACCELERATION_BUILD_SCRATCH_USE),
    DEF_BARRIER(RB_RO_SHADER_BINDING_TABLE),
    DEF_BARRIER(RB_ALIAS_TO),
    DEF_BARRIER(RB_ALIAS_FROM),
    DEF_BARRIER(RB_SOURCE_STAGE_RAYTRACE),
    DEF_BARRIER(RB_SOURCE_STAGE_COMPUTE),
    DEF_BARRIER(RB_SOURCE_STAGE_PIXEL),
    DEF_BARRIER(RB_SOURCE_STAGE_VERTEX),
    DEF_BARRIER(RB_FLAG_DONT_PRESERVE_CONTENT),
    DEF_BARRIER(RB_FLUSH_UAV),
    DEF_BARRIER(RB_STAGE_RAYTRACE),
    DEF_BARRIER(RB_STAGE_COMPUTE),
    DEF_BARRIER(RB_STAGE_PIXEL),
    DEF_BARRIER(RB_STAGE_VERTEX),
    DEF_BARRIER(RB_FLAG_SPLIT_BARRIER_END),
    DEF_BARRIER(RB_FLAG_SPLIT_BARRIER_BEGIN),
    DEF_BARRIER(RB_FLAG_ACQUIRE_PIPELINE_OWNERSHIP),
    DEF_BARRIER(RB_FLAG_RELEASE_PIPELINE_OWNERSHIP),
    DEF_BARRIER(RB_RO_RAYTRACE_ACCELERATION_BUILD_SOURCE),
    DEF_BARRIER(RB_RO_BLIT_SOURCE),
    DEF_BARRIER(RB_RO_COPY_SOURCE),
    DEF_BARRIER(RB_RO_VARIABLE_RATE_SHADING_TEXTURE),
    DEF_BARRIER(RB_RO_INDIRECT_BUFFER),
    DEF_BARRIER(RB_RO_INDEX_BUFFER),
    DEF_BARRIER(RB_RO_VERTEX_BUFFER),
    DEF_BARRIER(RB_RO_CONSTANT_BUFFER),
    DEF_BARRIER(RB_RO_SRV),
    DEF_BARRIER(RB_RO_OPACITY_MICRO_MAP_BUILD_INPUT_BUFFER),
    DEF_BARRIER(RB_RO_OPACITY_MICRO_MAP_BUILD_DESCRIPTION_BUFFER),
    DEF_BARRIER(RB_RW_BLIT_DEST),
    DEF_BARRIER(RB_RW_COPY_DEST),
    DEF_BARRIER(RB_RW_UAV),
    DEF_BARRIER2(RB_RW_RENDER_TARGET, RB_RW_DEPTH_STENCIL_TARGET),
#undef DEF_BARRIER2
#undef DEF_BARRIER
  };
  auto activeMask = static_cast<eastl::underlying_type_t<ResourceBarrier>>(barrier);
  for (auto &e : table)
  {
    if (e.value == (e.value & activeMask))
    {
      activeMask ^= e.value;
      target(e.name);
    }
  }
  return static_cast<ResourceBarrier>(activeMask);
}
