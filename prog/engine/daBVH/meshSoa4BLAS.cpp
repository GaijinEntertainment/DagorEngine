// Copyright (C) Gaijin Games KFT.  All rights reserved.

// constraint: own TU, so a caller of build_mesh_blas alone links no converter

#include <daBVH/dag_meshBLAS.h>
#include <daBVH/dag_swBLAS_soa4Convert.h>
#include <memory/dag_framemem.h>
#include <debug/dag_log.h>

bool build_mesh_soa4_blas(MeshSoa4BLAS &blas, dag::ConstSpan<Point3> verts, dag::ConstSpan<uint32_t> indices)
{
  blas = MeshSoa4BLAS{};

  // the stackless form lives only to be converted, and the conversion nests its own allocations
  MeshBLAS stackless;
  dag::set_allocator(stackless.data, framemem_ptr());
  if (!build_mesh_blas(stackless, verts, indices))
    return false;

  const soa4::ConvertResult cr = soa4::buildFromStackless(stackless.data.data(), 0, stackless.blasSize, stackless.vertsOfs,
    (int)stackless.data.size() - stackless.vertsOfs, blas.tree, /*with_flags*/ false);
  if (!cr.valid())
  {
    logerr("build_mesh_soa4_blas: the SoA4 conversion refused a %d byte tree", stackless.blasSize);
    blas.tree.clear(); // constraint: buildFromStackless leaves its out buffer unspecified on failure
    return false;
  }

  blas.root = cr.root;
  blas.treeBytes = cr.treeBytes;
  blas.vertsOfs = cr.vertsOfs;
  blas.scale = stackless.scale;
  blas.invScale = stackless.invScale;
  blas.ofs = stackless.ofs;
  blas.box = stackless.box;
  return true;
}
