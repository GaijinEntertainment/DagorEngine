//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// Conversions between the stackless BLAS layout (dag_swBLAS_ray.h, the GPU upload format) and the
// SoA4 CPU layout (dag_swBLAS_soa4.h). Both directions rebuild only the tree region and re-point
// the leaf apex bases; the vert21 vert region is copied verbatim. A SoA4 -> stackless -> SoA4 (or
// the reverse) round trip is byte-identical for SAH-built trees. Implemented in soa4Convert.cpp.

#include <daBVH/dag_swBLAS_soa4.h>
#include <dag/dag_vector.h>

namespace soa4
{

// Geometry of a converted buffer: [tree][pad to 8][vert21 verts], vertsOfs == align8(treeBytes).
struct ConvertResult
{
  RootRef root;      // invalid on failure (out is then unspecified; keep using the source buffer)
  int treeBytes = 0; // SoA4 tree region size
  int vertsOfs = 0;  // byte offset of the copied vert region
  bool valid() const { return root.valid(); }
};

// Convert a stackless BLAS (root-children span [blas_start, blas_start + blas_size) in `src`, vert21
// verts at src_verts_ofs) into a freshly filled SoA4 buffer. Fails -- in release too -- on a tree
// too large for the ref encoding (32 MB), a span fanout above 4 (create_bvh_node_sah caps it) or a
// malformed source walk; the caller must keep its stackless data on failure.
ConvertResult buildFromStackless(const uint8_t *src, int blas_start, int blas_size, int src_verts_ofs, int vert_bytes,
  dag::Vector<uint8_t> &out);

// Rebuild a stackless buffer from a SoA4 one (for GPU upload / stackless-only consumers).
struct StacklessResult
{
  int treeBytes = -1; // stackless tree region size (== the original stackless blasSize), < 0 on failure
  int vertsOfs = 0;
  bool valid() const { return treeBytes >= 0; }
};
StacklessResult buildStackless(const uint8_t *src, RootRef root, int src_verts_ofs, int vert_bytes, dag::Vector<uint8_t> &out);

} // namespace soa4
