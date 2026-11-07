#ifndef MESHLET_LAYOUT_HLSLI
#define MESHLET_LAYOUT_HLSLI

// Meshlet buffer is StructuredBuffer<uint2>. Each uint2 is one of 2 types:
//  1) MeshletDesc - one meshlet: offset of its entries, vertex count, triangle count
//  2) MeshletEntry - data of one mesh shader thread: vertex index, triangle0, triangle1
//
// Each vdata has one block of meshletCount meshlets, in this order:
//  1) MeshletDesc[meshletCount]
//  2) MeshletEntry[max(vertexCount, min(triangleCount, MESHLET_THREAD_GROUP_SIZE))] for each meshlet
//
// A block does not depend on its place in the buffer: dataOffset is an entry index from the block start.

#define MESHLET_THREAD_GROUP_SIZE 64

#define MESHLET_MAX_VERTICES  64
#define MESHLET_MAX_TRIANGLES 124

#define MESHLET_HEADER_ENTRIES 1
#define MESHLET_HEADER_DESC    0

// MeshletDesc layout
//---------------------------------------------------------------------------
// desc.x: dataOffset, triangleCount - 1
#define MESHLET_DESC_DATA_OFS_BITS   25
#define MESHLET_DESC_DATA_OFS_MASK   ((1u << MESHLET_DESC_DATA_OFS_BITS) - 1)
#define MESHLET_DESC_TRI_COUNT_SHIFT MESHLET_DESC_DATA_OFS_BITS
#define MESHLET_DESC_TRI_COUNT_BITS  7

// desc.y: vertexCount - 1
#define MESHLET_DESC_VERT_COUNT_BITS 6
#define MESHLET_DESC_VERT_COUNT_MASK ((1u << MESHLET_DESC_VERT_COUNT_BITS) - 1)
//---------------------------------------------------------------------------

// MeshletEntry layout
//---------------------------------------------------------------------------
// Mesh shader thread k reads entry k: vertex k, tri0 = triangle k, tri1 = triangle k + MESHLET_THREAD_GROUP_SIZE.
// A field past vertexCount or triangleCount is 0, so check the counts, not the value.
// vertex_index is relative to RElem baseVertex, as an IB index is. A triangle holds 3 local indices into the
// meshlet vertices.
//
//   x: [tri0.x low bits | vertex_index]
//   y: [tri1.z | tri1.y | tri1.x | tri0.z | tri0.y | tri0.x high bits]
//
#define MESHLET_ENTRY_VERTEX_INDEX_BITS  28
#define MESHLET_ENTRY_VERTEX_INDEX_MASK  ((1u << MESHLET_ENTRY_VERTEX_INDEX_BITS) - 1)
#define MESHLET_ENTRY_LOCAL_INDEX_BITS   6
#define MESHLET_ENTRY_LOCAL_INDEX_MASK   ((1u << MESHLET_ENTRY_LOCAL_INDEX_BITS) - 1)
#define MESHLET_ENTRY_TRI_BITS           (3 * MESHLET_ENTRY_LOCAL_INDEX_BITS)
#define MESHLET_ENTRY_TRI0_X_BITS        (32 - MESHLET_ENTRY_VERTEX_INDEX_BITS)
#define MESHLET_ENTRY_TRI1_Y_SHIFT       (MESHLET_ENTRY_TRI_BITS - MESHLET_ENTRY_TRI0_X_BITS)
//---------------------------------------------------------------------------

#endif
