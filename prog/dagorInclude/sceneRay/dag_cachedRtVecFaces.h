//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sceneRay/dag_sceneRay.h>
#include <vecmath/dag_vecMath.h>
#include <osApiWrappers/dag_miscApi.h>
#include <memory/dag_framemem.h>

enum
{
  BITARRAY_BITS = 5,
  BITARRAY_MASK = (1 << BITARRAY_BITS) - 1
};

enum
{
  FACE_WORDS_CLEAR_AHEAD = 8,
  FACE_WORDS_START = 128
};

template <typename FI>
struct StaticSceneRayTracerT<FI>::GetFacesContext
{
  typedef uint32_t bitarray_type;
  vec4f *__gf_triangles;
  bitarray_type *__gf_face_checked;

  int __gf_triangles_left;
  int __gf_last_uncleared_idx;
  Tab<bitarray_type> __gf_face_checked_tab;

  bbox3f __gf_wbox_vec;
  StaticSceneRayTracerT<FI>::RTface *__gf_faces;
  vec3f *__gf_verts;
  __forceinline const bbox3f &get_wbox() const { return __gf_wbox_vec; }
  __forceinline const StaticSceneRayTracerT<FI>::RTface &getFace(int idx) { return __gf_faces[idx]; }
  __forceinline const vec3f getVert(int idx) { return __gf_verts[idx]; }
  GetFacesContext(bool skip_init = false) // -V730 // -V1077
  {
    if (!skip_init)
      init();
  }
  void init()
  {
    __gf_verts = NULL;
    __gf_faces = NULL;
    __gf_face_checked = NULL;
    __gf_last_uncleared_idx = 0;
    __gf_triangles = NULL;
    __gf_triangles_left = 0;
  }
  void init(IMemAlloc *mem)
  {
    init();
    dag::set_allocator(__gf_face_checked_tab, mem);
  }
  __forceinline bool markFace(int fidx)
  {
    int bitarrIdx = fidx >> BITARRAY_BITS;
    bitarray_type bitarrMask = 1 << (fidx & BITARRAY_MASK);
    if (bitarrIdx >= __gf_last_uncleared_idx)
    {
      memset(&__gf_face_checked[__gf_last_uncleared_idx], 0,
        (bitarrIdx + FACE_WORDS_CLEAR_AHEAD - __gf_last_uncleared_idx) * sizeof(bitarray_type));
      __gf_last_uncleared_idx = bitarrIdx + FACE_WORDS_CLEAR_AHEAD;
    }
    else if (__gf_face_checked[bitarrIdx] & bitarrMask)
      return false;
    __gf_face_checked[bitarrIdx] |= bitarrMask;
    return true;
  }
  __forceinline bool process_triangle(int fidx, vec4f vert0, vec4f vert1, vec4f vert2)
  {
    if (__gf_triangles_left <= 0)
      return false;
    __gf_triangles[0] = v_perm_xyzd(vert0, v_cast_vec4f(v_splatsi(fidx)));
    __gf_triangles[1] = v_sub(vert1, vert0);
    __gf_triangles[2] = v_sub(vert2, vert0);
    __gf_triangles += 3;
    __gf_triangles_left--;
    return true;
  }

  bool processFace(int fidx)
  {
    if (!markFace(fidx))
      return true;
    const auto &f = getFace(fidx);

    vec3f vert0 = getVert(f.v[0]);
    vec3f vert1 = getVert(f.v[1]);
    vec3f vert2 = getVert(f.v[2]);
    bbox3f facebox;
    facebox.bmin = facebox.bmax = vert0;
    v_bbox3_add_pt(facebox, vert1);
    v_bbox3_add_pt(facebox, vert2);

    if (!v_bbox3_test_box_intersect(get_wbox(), facebox))
      return true;
    if (!process_triangle(fidx, vert0, vert1, vert2))
      return false;
    return true;
  }
};

template <typename FI>
template <class CB>
inline bool StaticSceneRayTracerT<FI>::getVecFacesCachedNode(const StaticSceneRayTracerT<FI>::Node *node, CB &ctx)
{
  G_STATIC_ASSERT(sizeof(StaticSceneRayTracerT<FI>::Node) ==
                  sizeof(Point3) + sizeof(real) + 2 * sizeof(PatchablePtr<Node>) /* bsr2 should be after bsc */);
  vec4f boundingSph = v_ld(&node->bsc.x);
  if (!v_bbox3_test_sph_intersect(ctx.get_wbox(), boundingSph, v_splat_w(boundingSph)))
    return true;
  if (node->isNode())
  {
    // branch node
    if (!getVecFacesCachedNode(node->getLeft(), ctx))
      return false;
    return getVecFacesCachedNode(node->getRight(), ctx);
  }
  else
    return getVecFacesCachedLNode(*(const LNode *)node, ctx);
}

template <typename FI>
template <class CB>
inline bool StaticSceneRayTracerT<FI>::getVecFacesCachedLNode(const StaticSceneRayTracerT<FI>::LNode &node, CB &ctx)
{
  const FaceIndex *__restrict fIndices = node.getFaceIndexStart();
  const FaceIndex *__restrict fIndicesEnd = node.getFaceIndexEnd();

  for (; fIndices < fIndicesEnd; ++fIndices)
  {
    if (!ctx.processFace(*fIndices))
      return false;
  }
  return true;
}

template <typename FI>
template <class CB>
inline bool StaticSceneRayTracerT<FI>::getVecFacesCached(bbox3f_cref wbox, CB &ctx) const
{
  if (!v_bbox3_test_box_intersect(wbox, v_rtBBox))
    return true;
  vec3f v_inv_leaf_size = v_rcp(v_ldu(&getLeafSize().x));
  vec4i v_b0 = v_cvt_floori(v_mul(v_inv_leaf_size, wbox.bmin));
  vec4i v_b1 = v_cvt_floori(v_mul(v_inv_leaf_size, wbox.bmax));
  alignas(16) int b0[4];
  alignas(16) int b1[4];
  v_sti(b0, v_b0);
  v_sti(b1, v_b1);
  getLeafLimits().clip(b0[0], b0[1], b0[2], b1[0], b1[1], b1[2]);

  ctx.__gf_wbox_vec = wbox;
  int faceWords = (getFacesCount() + BITARRAY_MASK) >> BITARRAY_BITS;
  ctx.__gf_face_checked_tab.resize(faceWords + FACE_WORDS_CLEAR_AHEAD);
  ctx.__gf_face_checked = ctx.__gf_face_checked_tab.data();
  if (ctx.__gf_last_uncleared_idx > 0)
  {
    ctx.__gf_last_uncleared_idx = min(ctx.__gf_last_uncleared_idx, faceWords);
    memset(&ctx.__gf_face_checked[0], 0, ctx.__gf_last_uncleared_idx * sizeof(typename CB::bitarray_type));
  }

  //__gf_tab = &list;
  //__gf_bitarr = &fchk;

  ctx.__gf_faces = dump.facesPtr;
  ctx.__gf_verts = (vec3f *)((Point3_vec4 *)dump.vertsPtr);

  for (int z = b0[2]; z <= b1[2]; ++z)
    for (int y = b0[1]; y <= b1[1]; ++y)
      for (int x = b0[0]; x <= b1[0]; ++x)
      {
        auto getLeafRes = dump.grid->get_leaf(x, y, z);
        if (!getLeafRes || !*getLeafRes.leaf)
          continue;

        if (!getVecFacesCachedNode(*getLeafRes.leaf, ctx))
          return false;
      }
  return true;
}

template <typename FI>
inline bool StaticSceneRayTracerT<FI>::getVecFacesCached(bbox3f_cref wbox, vec4f *__restrict triangles, int &tri_left) const
{
  GetFacesContext localCtx(true);
  GetFacesContext *ctx;
  if (is_main_thread())
    ctx = getMainThreadCtx();
  else
  {
    localCtx.init(framemem_ptr());
    ctx = &localCtx;
  }
  ctx->__gf_triangles = triangles;
  ctx->__gf_triangles_left = tri_left;
  if (!getVecFacesCached(wbox, *ctx))
    return false;

  tri_left = ctx->__gf_triangles_left;
  return true;
}
