//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <vecmath/dag_vecMath.h>
#include <dag/dag_vector.h>
#include <util/dag_baseDef.h>
#include <daBVH/swBLASLeafDefs.hlsli>
#include <daBVH/dag_swBLASRootRef.h> // soa4::RootRef

class IGenSave;
class IGenLoad;

// Compact, boxless (de)serialization of a double-quad collision BLAS (uint16 boxes, vert21 verts).
//
// A runtime BLAS is a self-contained [tree][pad][vert21] buffer in normalized box space, in the
// stackless walker layout (CollisionResource grids store its SoA4 conversion instead -- see
// dag_swBLAS_soa4Convert.h). Every node box is the tight AABB of its descendant verts
// and every sibling-skip word is implied by the tree shape, so neither is stored on disk: only the
// per-node child count, the per-leaf topology (offsets + base vertex index), the verts and the one
// whole-BLAS box are written. A single-quad leaf (no quad B) stores half the leaf body: its canonical
// W2/W3 are implied by the marker byte. deserializeQuadBLAS rebuilds every box, skip and implied word
// in a single recursive descent while streaming the tree straight into ONE final allocation (no scratch).
//
// The disk stream is tightly packed, ordered header, verts, tree (verts first so deserialize can
// rebuild leaf boxes during its single tree pass); the source's tree/vert padding is not written.
// Because the leaf apex is stored as a layout-independent vertex INDEX (not a byte offset), the reader
// is free to place the vert21 region wherever it wants; deserialize re-aligns it to align8(treeBytes)
// -- the runtime canonical -- and reports it in its result struct. Any offset yields an identical
// trace, so the source vertsOfs need not be stored.
//
// Round-trip is lossless for ray/dist queries: the rebuilt boxes bound the very vert21 verts the
// traversal reads, so the set of triangles tested -- and thus every hit -- is identical to the source.
// A degenerate no-hit leaf (writeDoubleQuadLeaf's overflow path) has no valid apex vertex, so it is
// carried verbatim and rebuilt with an empty box -- still "no geometry here", still hit-identical.
//
// deserializeQuadBLAS treats its input as untrusted: every header field and tree offset is bounds
// checked, the rebuild recursion is depth-capped (BVH_IO_MAX_TREE_DEPTH), and any violation throws
// IGenLoad::LoadException instead of over-reading, over-allocating, or overflowing the stack.

namespace build_bvh
{

inline constexpr int BVH_IO_MAGIC = _MAKE4C('dBLi'); // daBVH BLAS Io
inline constexpr uint16_t BVH_IO_VERSION = 1;        // bump on any on-disk layout change

// The 24-bit quad base reaches at most QUAD_BASE_BYTE_MAX (~64 MB), so no valid BLAS is bigger.
// Capping the accepted total at that reach keeps a corrupt header from overflowing the size math or
// requesting a wild alloc, and guarantees every leaf base the rebuild derives fits its 24-bit W1 field.
inline constexpr int64_t BVH_IO_MAX_BLAS_BYTES = QUAD_BASE_BYTE_MAX;

// A real SAH-built BLAS nests at most BVH_MAX_BLAS_DEPTH (32) deep, but both sides walk the tree by
// recursion, so a crafted stream or buffer could chain single children far deeper. This generous cap
// (well above any real tree, far below a native-stack overflow) makes such a chain bail, not overflow.
inline constexpr int BVH_IO_MAX_TREE_DEPTH = 256;

// Self-describing, independent header. All fields are naturally aligned (no packing), LE.
struct BlasIoHeader
{
  int32_t magic;      // BVH_IO_MAGIC
  uint16_t version;   // BVH_IO_VERSION
  uint8_t vertStride; // runtime vert stride (8 = vert21); guards the verbatim vert copy
  uint8_t leafSize;   // runtime leaf size (BVH_BLAS_LEAF_SIZE)
  uint32_t treeBytes; // runtime tree region size; verts follow tightly on disk, re-aligned on load
  uint32_t vertCount; // vert21 vertices
  float bmin[3];      // whole-BLAS box in local space (recovers scale/ofs == placement frame)
  float bmax[3];
};
static_assert(sizeof(BlasIoHeader) == 40, "BlasIoHeader must stay 40 LE bytes");

// Write a runtime BLAS buffer to `cwr`. Returns false -- writing NOTHING, so a disk caller never
// commits a truncated stream -- when the input is not a plausible BLAS (null/empty, an internal
// node fanout the one-byte child count cannot encode, a tree nested past BVH_IO_MAX_TREE_DEPTH,
// or a no-hit leaf as the whole tree, which both deserializers refuse).
//   blas       : stackless [tree][pad][vert21] buffer. CollisionResource::Grid::blasData stores the
//                SoA4 conversion and cannot be passed here: the serializer walks stackless skip words.
//   tree_bytes : real stackless tree size.
//   verts_ofs  : byte offset of the source vert21 region; any offset >= tree_bytes -- tight or
//                aligned -- works, deserialize re-aligns on load regardless.
//   vert_count : number of vert21 vertices.
//   local_box  : the whole-BLAS local box, needed to recover the placement frame.
bool serializeQuadBLAS(IGenSave &cwr, const uint8_t *blas, int tree_bytes, int verts_ofs, int vert_count, bbox3f local_box,
  int leaf_size = BVH_BLAS_LEAF_SIZE, int vert_stride = 8);

// Everything a caller needs to mount the buffer deserializeQuadBLAS filled as a runtime BLAS.
struct BlasDeserializeResult
{
  int treeBytes; // stackless tree region size
  int vertsOfs;  // byte offset of the vert21 region: align8(treeBytes), the runtime canonical
  int vertCount; // vert21 vertices in that region
  bbox3f box;    // whole-BLAS local box; the caller derives scale/ofs from it as Grid::buildBLAS does
};

// Read a BLAS from `crd` into one freshly-allocated runtime [tree][pad][vert21] buffer. `out` is
// resized to the final size; the vert21 region begins at align8(treeBytes) -- the runtime canonical,
// regardless of the source vertsOfs. Throws IGenLoad::LoadException on a bad magic/version/field.
BlasDeserializeResult deserializeQuadBLAS(IGenLoad &crd, dag::Vector<uint8_t> &out);

// Everything a caller needs to mount the buffer deserializeQuadBLASToSoA4 filled as a SoA4 CPU BLAS
// (the layout CollisionResource stores today). `root` is invalid on a tree the SoA4 encoding cannot
// represent (see below); `out` is then unspecified.
struct Soa4DeserializeResult
{
  soa4::RootRef root;      // tagged root child word for the SoA4 traversals; invalid() on a failed build
  int treeBytes = 0;       // SoA4 tree region size
  int vertsOfs = 0;        // byte offset of the vert21 region: align8(treeBytes)
  int vertCount = 0;       // vert21 vertices in that region
  bbox3f box = {};         // whole-BLAS local box (from the header; carried separately, not in the buffer)
  int serializedBytes = 0; // on-wire bytes this BLAS occupied: header + verts + stackless tree
};

// Read a BLAS from `crd` straight into one freshly-allocated SoA4 CPU buffer, skipping the transient
// stackless build the two-step deserializeQuadBLAS + soa4::buildFromStackless would materialize (both
// the detour work and its peak memory). The result is byte-identical to that two-step path. Throws
// IGenLoad::LoadException on the same corrupt-stream violations deserializeQuadBLAS rejects; returns an
// invalid root (no throw) when the tree is structurally valid but not SoA4-representable (a node fanout
// above 4, or a tree past the 32 MB LeafRef offset reach), matching soa4::buildFromStackless.
// One depth asymmetry with deserializeQuadBLAS: the SoA4 walkers traverse on a fixed stack, so a tree
// whose SoA4-emitted depth would exceed soa4::MAX_TREE_DEPTH (a stricter cap than the BVH_IO_MAX_TREE_DEPTH
// parse depth deserializeQuadBLAS accepts) is thrown out here at the load trust boundary. 1-child chains
// are promoted before that depth is measured, so a chain that collapses within the bound loads; one that
// stays past it after promotion returns an invalid root (no throw), like the other non-representable trees.
Soa4DeserializeResult deserializeQuadBLASToSoA4(IGenLoad &crd, dag::Vector<uint8_t> &out);

} // namespace build_bvh
