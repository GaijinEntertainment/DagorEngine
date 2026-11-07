// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/meshletLayout.hlsli>
#include <debug/dag_assert.h>
#include <math/dag_hlsl_floatx.h>
#include <util/dag_stdint.h>
#include <EASTL/algorithm.h>

static_assert(MESHLET_MAX_VERTICES <= (1 << MESHLET_ENTRY_LOCAL_INDEX_BITS));
static_assert(MESHLET_MAX_VERTICES <= (1 << MESHLET_DESC_VERT_COUNT_BITS));
static_assert(MESHLET_MAX_VERTICES <= MESHLET_THREAD_GROUP_SIZE, "one entry for each thread holds 1 vertex");
static_assert(MESHLET_MAX_TRIANGLES <= (1 << MESHLET_DESC_TRI_COUNT_BITS));
static_assert(MESHLET_MAX_TRIANGLES <= 2 * MESHLET_THREAD_GROUP_SIZE, "one entry for each thread holds up to 2 triangles");
static_assert(MESHLET_DESC_TRI_COUNT_SHIFT + MESHLET_DESC_TRI_COUNT_BITS == 32);
static_assert(MESHLET_ENTRY_VERTEX_INDEX_BITS + 2 * MESHLET_ENTRY_TRI_BITS == 64);

namespace shader_meshlets
{

inline constexpr uint32_t VERTICES_IN_TRIANGLE = 3;

using PackedEntry = uint2;
static_assert(sizeof(PackedEntry) == 2 * sizeof(uint32_t));

inline uint32_t meshlet_entry_count(uint32_t vertex_count, uint32_t triangle_count)
{
  const uint32_t tri0Count = eastl::min<uint32_t>(triangle_count, MESHLET_THREAD_GROUP_SIZE);
  return eastl::max(vertex_count, tri0Count);
}

inline PackedEntry pack_desc(uint32_t data_offset, uint32_t vertex_count, uint32_t triangle_count)
{
  G_ASSERT(data_offset <= MESHLET_DESC_DATA_OFS_MASK);
  G_ASSERT(vertex_count >= 1 && vertex_count <= MESHLET_MAX_VERTICES);
  G_ASSERT(triangle_count >= 1 && triangle_count <= MESHLET_MAX_TRIANGLES);

  return {data_offset | ((triangle_count - 1) << MESHLET_DESC_TRI_COUNT_SHIFT), vertex_count - 1};
}

/// triangle is VERTICES_IN_TRIANGLE local vertex indices of the meshlet
inline uint32_t encode_triangle(const uint8_t *triangle)
{
  G_ASSERT(triangle != nullptr);

  uint32_t result = 0;
  for (uint32_t i = 0; i < VERTICES_IN_TRIANGLE; ++i)
  {
    G_ASSERT(triangle[i] < MESHLET_MAX_VERTICES);
    result |= triangle[i] << (MESHLET_ENTRY_LOCAL_INDEX_BITS * i);
  }
  return result;
}

inline PackedEntry pack_entry(uint32_t vertex_index, uint32_t encoded_tri0, uint32_t encoded_tri1)
{
  G_ASSERT(vertex_index <= MESHLET_ENTRY_VERTEX_INDEX_MASK);
  G_ASSERT(encoded_tri0 < (1u << MESHLET_ENTRY_TRI_BITS));
  G_ASSERT(encoded_tri1 < (1u << MESHLET_ENTRY_TRI_BITS));

  return {vertex_index | (encoded_tri0 << MESHLET_ENTRY_VERTEX_INDEX_BITS),
    (encoded_tri0 >> MESHLET_ENTRY_TRI0_X_BITS) | (encoded_tri1 << MESHLET_ENTRY_TRI1_Y_SHIFT)};
}
} // namespace shader_meshlets
