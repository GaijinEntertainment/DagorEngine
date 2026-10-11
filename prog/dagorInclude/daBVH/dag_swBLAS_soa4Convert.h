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
// malformed source walk; the caller must keep its stackless data on failure. with_flags = false
// builds the tree without the node records' edge flags tails, for a chunk whose consumers never
// read flags; the degenerate whole-BLAS-is-one-leaf root block keeps LEAF_BYTES in both layouts
// (the size-keyed validators bound it by LEAF_BYTES).
ConvertResult buildFromStackless(const uint8_t *src, int blas_start, int blas_size, int src_verts_ofs, int vert_bytes,
  dag::Vector<uint8_t> &out, bool with_flags = true);

// Rebuild a stackless buffer from a SoA4 one (for GPU upload / stackless-only consumers).
// Emits subtrees in the SoA4 tree's lane order, so a leaf walk of the output visits leaves in the
// same DFS order iterateLeafRefs walks the source: the bvhIO edge-flags section (one word per leaf,
// wire order) relies on this agreement to land words back on their leaves.
struct StacklessResult
{
  // One outcome per conversion: Emitted fills `out`, AllCarved (the filter kept no leaf) and Failed (a structural refusal)
  // leave it empty, and only Failed is a fault.
  enum class Status : uint8_t
  {
    Failed,
    Emitted,
    AllCarved
  };
  Status status = Status::Failed;
  int treeBytes = 0; // EMITTED stackless tree bytes (== the original blasSize when no leaf is carved); 0 unless emitted
  int vertsOfs = 0;
  bool valid() const { return status == Status::Emitted; }
};
// Optional per-leaf filter: bit v of keep_mask keeps the leaves whose 6-bit user value is v
// (~0 = keep everything). One leaf carries one user value (the builder never mixes values in a
// leaf), so the filter is exact per value -- and a mask, unlike a callable, cannot answer the
// sizing and emit passes differently, so the two-pass agreement holds by construction. A dropped
// leaf is not emitted and an internal child whose whole subtree drops is pruned; kept boxes are
// NOT refit (conservative over the unfiltered subtree). When every leaf drops the status is AllCarved
// (no buffer, no fault), distinct from a structural Failed. Verts are still copied verbatim.
StacklessResult buildStackless(const uint8_t *src, RootRef root, int src_verts_ofs, int vert_bytes, dag::Vector<uint8_t> &out,
  uint64_t keep_mask = ~0ull);

} // namespace soa4
