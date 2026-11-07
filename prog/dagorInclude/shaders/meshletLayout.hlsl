#ifndef MESHLET_LAYOUT_HLSL
#define MESHLET_LAYOUT_HLSL

#include <meshletLayout.hlsli>

struct MeshletDesc
{
  uint dataOffset;
  uint vertexCount;
  uint triangleCount;
};

MeshletDesc meshlet_unpack_desc(uint2 data)
{
  MeshletDesc desc;
  desc.dataOffset = data.x & MESHLET_DESC_DATA_OFS_MASK;
  desc.triangleCount = (data.x >> MESHLET_DESC_TRI_COUNT_SHIFT) + 1;
  desc.vertexCount = (data.y & MESHLET_DESC_VERT_COUNT_MASK) + 1;
  return desc;
}

struct MeshletEntry
{
  uint vertexIndex;
  uint3 triangle0;
  uint3 triangle1;
};

uint3 meshlet_unpack_triangle(uint t)
{
  return uint3(t, t >> MESHLET_ENTRY_LOCAL_INDEX_BITS, t >> (2 * MESHLET_ENTRY_LOCAL_INDEX_BITS)) &
         MESHLET_ENTRY_LOCAL_INDEX_MASK;
}

MeshletEntry meshlet_unpack_entry(uint2 data)
{
  MeshletEntry entry;
  entry.vertexIndex = data.x & MESHLET_ENTRY_VERTEX_INDEX_MASK;
  entry.triangle0 = meshlet_unpack_triangle((data.x >> MESHLET_ENTRY_VERTEX_INDEX_BITS) | (data.y << MESHLET_ENTRY_TRI0_X_BITS));
  entry.triangle1 = meshlet_unpack_triangle(data.y >> MESHLET_ENTRY_TRI1_Y_SHIFT);
  return entry;
}

#endif
