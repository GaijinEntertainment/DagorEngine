//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <dag/dag_vector.h>
#include <generic/dag_span.h>
#include <generic/dag_tab.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <vecmath/dag_vecMath.h>
#include <daBVH/dag_swBLASRootRef.h>

// One triangle soup as one double-quad BLAS in its own quantized frame, laid out [tree][pad][verts]
// with the verts packed 21-bit, which is what the collision cook builds per node.
// The frame maps a world point as b = w * scale + ofs, and back as w = (b - ofs) * invScale. The
// SoA4 walks take box space, so a caller converts its query and its answer; each walker's own
// header names the space it takes.
struct MeshBLAS
{
  // dag::set_allocator before the build backs a transient one with framemem_ptr(); a build keeps
  // whatever allocator it is handed.
  Tab<uint8_t> data;
  vec4f scale = V_C_ONE;
  vec4f invScale = V_C_ONE; // build_mesh_blas keeps it agreeing with scale
  vec4f ofs = v_zero();
  int blasSize = 0; // tree bytes, from data's start
  int vertsOfs = 0; // the vert21 stream, 8-aligned past the tree: data.size() == vertsOfs + 8 * verts
  BBox3 box;        // world space, over the verts the faces referenced
  bool empty() const { return blasSize == 0; }
};

// Only whole triples of indices are built, but every entry must index a vertex, a trailing partial
// triple included. Answers false and leaves blas empty for an index past the verts, a soup with no
// buildable triangle, a non-finite vertex a face references, or a vert span past the 24-bit leaf
// base range.
bool build_mesh_blas(MeshBLAS &blas, dag::ConstSpan<Point3> verts, dag::ConstSpan<uint32_t> indices);

// The same soup in the SoA4 layout, which the 4-wide ray, box and XZ walks read. The conversion
// copies the vert stream in, so the tree is self-contained. It is built without the edge-flags
// tails, so leafFlagsOfs, leafEdgeFlags and soa4::computeEdgeFlags do not apply to it; a caller
// that wants flags needs a tree built with them.
struct MeshSoa4BLAS
{
  dag::Vector<uint8_t> tree;
  soa4::RootRef root;
  int treeBytes = 0; // the SoA4 tree region, which vertsOfs rounds up to 8
  int vertsOfs = 0;  // the vert21 stream inside tree: tree.size() == vertsOfs + 8 * verts
  vec4f scale = V_C_ONE;
  vec4f invScale = V_C_ONE;
  vec4f ofs = v_zero();
  BBox3 box; // world space, as MeshBLAS holds it
  // The walkers take root without testing it, so a caller walks only what this answers false for.
  bool empty() const { return !root.valid(); }
};

// Refuses what build_mesh_blas refuses, and a tree the SoA4 encoding cannot hold. A refusal leaves
// blas empty, tree included.
bool build_mesh_soa4_blas(MeshSoa4BLAS &blas, dag::ConstSpan<Point3> verts, dag::ConstSpan<uint32_t> indices);
