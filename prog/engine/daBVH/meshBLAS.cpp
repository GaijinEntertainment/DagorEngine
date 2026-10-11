// Copyright (C) Gaijin Games KFT.  All rights reserved.

// constraint: own TU, so a target that never builds a BLAS links none of it

#include <daBVH/dag_meshBLAS.h>
#include <daBVH/dag_bvhBuild.h>
#include <daBVH/dag_quadBLASBuilder.h>
#include <generic/dag_tab.h>
#include <debug/dag_log.h>
#include <string.h>

bool build_mesh_blas(MeshBLAS &blas, dag::ConstSpan<Point3> src_verts, dag::ConstSpan<uint32_t> src_indices)
{
  int vert_count = (int)src_verts.size();
  const int index_count = (int)src_indices.size();

  // the reset would hand data the default allocator back, losing a caller's framemem choice
  IMemAlloc *dataAlloc = dag::get_allocator(blas.data);
  blas = MeshBLAS{};
  dag::set_allocator(blas.data, dataAlloc);

  if (vert_count <= 0 || index_count < 3)
    return false;
  for (int i = 0; i < index_count; ++i)
    if (src_indices[i] >= (uint32_t)vert_count)
    {
      logerr("build_mesh_blas: index %u at %d is past the %d verts", src_indices[i], i, vert_count);
      return false;
    }

  dag::Vector<vec4f> srcVerts4(vert_count);
  for (int i = 0; i < vert_count; ++i)
    srcVerts4[i] = i + 1 < vert_count ? v_ldu(&src_verts[i].x) : v_ldu_p3_safe(&src_verts[i].x);

  dag::Vector<uint32_t> idx(src_indices.begin(), src_indices.end());
  dag::Vector<vec4f> verts;
  build_bvh::leafOrderVertexFetch(idx.data(), (unsigned)index_count, srcVerts4.data(), (unsigned)vert_count, verts);
  vert_count = (int)verts.size();

  for (int i = 0; i < vert_count; ++i)
    if (!v_test_xyz_finite(verts[i]))
    {
      logerr("build_mesh_blas: referenced vertex %d of %d is not finite", i, vert_count);
      return false;
    }

  const bbox3f worldBox = build_bvh::calcBox(verts.data(), vert_count);

  Tab<build_bvh::QuadPrim> prims;
  int quadCount = 0, singleCount = 0;
  build_bvh::buildQuadPrims(prims, quadCount, singleCount, idx.data(), index_count / 3, verts.data());
  if (prims.empty())
    return false;

  dag::Vector<build_bvh::DoubleQuadPrim> dqs;
  build_bvh::buildDoubleQuadPrims(dqs, prims.data(), (int)prims.size(), verts.data());
  const int dqCount = (int)dqs.size();

  dag::Vector<bbox3f> dqBoxes(dqCount);
  build_bvh::addDoubleQuadPrimitivesAABBList(dqBoxes.data(), dqs.data(), dqCount, verts.data());
  Tab<bbox3f> nodes;
  int maxDepth = 0;
  const int rootNode = build_bvh::create_bvh_node_sah(nodes, dqBoxes.data(), (uint32_t)dqCount, 4, maxDepth);

  const vec3f safeSize = v_max(v_sub(worldBox.bmax, worldBox.bmin), v_splats(0.0001f));
  const vec4f xyzOnly = v_cast_vec4f(V_CI_MASK1110);
  const vec4f scale = v_and(v_div(v_splats(65535.f), safeSize), xyzOnly);
  const vec4f ofs = v_and(v_neg(v_mul(worldBox.bmin, scale)), xyzOnly);

  const int vertsOfs = (build_bvh::calcBLASTreeBytes((int)nodes.size(), dqCount) + 7) & ~7;
  if ((int64_t)vertsOfs + (int64_t)(vert_count - 1) * 8 > (int64_t)QUAD_BASE_BYTE_MAX)
  {
    logerr("build_mesh_blas: vert span of %d verts exceeds the 24-bit leaf base range", vert_count);
    return false;
  }

  blas.scale = scale;
  blas.invScale = v_div(V_C_ONE, v_sel(V_C_ONE, scale, xyzOnly));
  blas.ofs = ofs;
  blas.vertsOfs = vertsOfs;
  v_stu_bbox3(blas.box, worldBox);
  blas.data.resize_noinit(vertsOfs + vert_count * 8);

  int dataOffset = 0;
  build_bvh::writeDoubleQuadBVH2(blas.data.data(), nodes.data(), dqs.data(), blas.scale, blas.ofs, vertsOfs, rootNode, rootNode,
    dataOffset, 8);
  blas.blasSize = dataOffset;
  if (vertsOfs > dataOffset) // resize_noinit leaves the align-to-8 pad holding allocator bytes
    memset(blas.data.data() + dataOffset, 0, (size_t)(vertsOfs - dataOffset));
  for (int i = 0; i < vert_count; ++i)
    build_bvh::packVert21(blas.data.data() + vertsOfs + i * 8, v_madd(verts[i], blas.scale, blas.ofs));
  return true;
}
