// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <memory/dag_framemem.h>
#include <dag/dag_vectorMap.h>
#include <math/dag_mathUtils.h>
#include <generic/dag_relocatableFixedVector.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include <daFracture/core/cutFaceFill.h>
#include "cutMeshCommon.h"


namespace frx
{

DAGOR_NOINLINE inline static void cut_mesh_plane_impl(const DestrContext &ctx, const DestrMesh &mesh, DestrMesh &up_mesh,
  DestrMesh &down_mesh, plane3f cut_plane, CutFaceData &cut_face_data)
{
  constexpr float EPS = 1e-6f;

  // reserve face arrays
  dag::Vector<DestrMesh::Face, framemem_allocator> candidateFacesVec;
  const uint32_t upMeshFacesStart = up_mesh.faces.size();
  const uint32_t downMeshFacesStart = down_mesh.faces.size();
  const uint32_t upMeshVertsStart = up_mesh.verts.size();
  const uint32_t downMeshVertsStart = down_mesh.verts.size();

  // precompute distances of vertices to plane
  const DestrMesh::Vertex *__restrict verts = mesh.verts.data();
  dag::Vector<float, framemem_allocator> vertDistVec;
  dag::Vector<uint8_t, framemem_allocator> vertClassifyVec;
  vertDistVec.resize_noinit(mesh.verts.size());
  vertClassifyVec.resize_noinit(mesh.verts.size());
  float *__restrict vertDist = vertDistVec.data();
  uint8_t *__restrict vertClassify = vertClassifyVec.data();

  // repack vertices, that can be classified to be in one part of the mesh in advance
  dag::Vector<uint32_t, framemem_allocator> old2newUp, old2newDown;
  old2newUp.resize_noinit(mesh.verts.size());
  old2newDown.resize_noinit(mesh.verts.size());
  uint32_t *__restrict old2newUpPtr = old2newUp.data();
  uint32_t *__restrict old2newDownPtr = old2newDown.data();

  {
    up_mesh.verts.resize_noinit(upMeshVertsStart + mesh.verts.size());
    down_mesh.verts.resize_noinit(downMeshVertsStart + mesh.verts.size());
    DestrMesh::Vertex *__restrict upVertsPtr = up_mesh.verts.data() + upMeshVertsStart;
    DestrMesh::Vertex *__restrict downVertsPtr = down_mesh.verts.data() + downMeshVertsStart;
    auto *upVertsStart = up_mesh.verts.data(); // absolute base, so old2new matches reindexVert's v.size()
    auto *downVertsStart = down_mesh.verts.data();
    for (int i = 0, ie = mesh.verts.size(); i != ie; i++)
    {
      const float d = vertDist[i] = v_extract_x(v_plane_dist_x(cut_plane, v_ld(&verts[i].pos.x)));
      *upVertsPtr = *downVertsPtr = verts[i];
      const uint8_t down = uint8_t(d < -EPS);
      const uint8_t up = uint8_t(d > EPS);
      old2newUpPtr[i] = up ? upVertsPtr - upVertsStart : uint32_t(-1);
      old2newDownPtr[i] = down ? downVertsPtr - downVertsStart : uint32_t(-1);
      upVertsPtr += up;
      downVertsPtr += down;
      vertClassify[i] = up + (down << 1); // 0 for center, 1 for up, 2 for down
    }
    up_mesh.verts.resize(upVertsPtr - up_mesh.verts.data());
    down_mesh.verts.resize(downVertsPtr - down_mesh.verts.data());
  }

  // coarse cull pass: pure faces are remapped inline here; straddling faces are deferred to the fine pass
  {
    up_mesh.faces.resize_noinit(upMeshFacesStart + mesh.faces.size());
    down_mesh.faces.resize_noinit(downMeshFacesStart + mesh.faces.size());
    candidateFacesVec.resize_noinit(mesh.faces.size());
    DestrMesh::Face *__restrict upFacesPtr = up_mesh.faces.data() + upMeshFacesStart;
    DestrMesh::Face *__restrict downFacesPtr = down_mesh.faces.data() + downMeshFacesStart;
    DestrMesh::Face *__restrict candidateFacesPtr = candidateFacesVec.data();
    for (const auto &face : mesh.faces)
    {
      uint8_t c0 = vertClassify[face.idx.data()[0]];
      uint8_t c1 = vertClassify[face.idx.data()[1]];
      uint8_t c2 = vertClassify[face.idx.data()[2]];
      uint8_t c012 = c0 & c1 & c2;
      if ((c0 & c1 & c2) == 0) // either on different sides, or some are in the [0, max_dist] range
        *candidateFacesPtr++ = face;
      else
      {
        const bool isDown = c012 & 2;
        DestrMesh::Face *__restrict &facesPtr = isDown ? downFacesPtr : upFacesPtr;
        DestrMesh::Face &f = *facesPtr++;
        uint32_t *remap = isDown ? old2newDownPtr : old2newUpPtr;
        f = face;
        f.idx.data()[0] = remap[f.idx.data()[0]];
        f.idx.data()[1] = remap[f.idx.data()[1]];
        f.idx.data()[2] = remap[f.idx.data()[2]];
      }
    }
    up_mesh.faces.resize(upFacesPtr - up_mesh.faces.data());
    down_mesh.faces.resize(downFacesPtr - down_mesh.faces.data());
    candidateFacesVec.resize(candidateFacesPtr - candidateFacesVec.data());
  }

  // fine pass + split faces

  const uint32_t upMeshCutFacesStart = up_mesh.faces.size();
  const uint32_t downMeshCutFacesStart = down_mesh.faces.size();

  constexpr uint32_t CUT_VERT_FLAG = 0x80000000u;
  ska::flat_hash_map<uint64_t, uint32_t, eastl::hash<uint64_t>, eastl::equal_to<uint64_t>, framemem_allocator> cutEdges;
  dag::Vector<DestrMesh::Vertex, framemem_allocator> cutVerts;
  const auto reindexVert = [&](bool up, uint32_t idx) FORCE_INLINE_LAMBDA {
    uint32_t &__restrict remap = (up ? old2newUpPtr : old2newDownPtr)[idx];
    // vertex is most likely already put in the appropriate mesh during vertex classify pass
    if (DAGOR_UNLIKELY(remap == uint32_t(-1)))
    {
      auto &v = (up ? up_mesh : down_mesh).verts;
      remap = v.size();
      v.push_back(verts[idx]);
    }
    return remap;
  };
  const auto makeCutVert = [&](uint32_t i1, uint32_t i2, vec4f tttt) FORCE_INLINE_LAMBDA -> uint32_t {
    const uint64_t edgeKey = i1 < i2 ? (uint64_t(i1) << 32u) | i2 : (uint64_t(i2) << 32u) | i1;
    uint32_t &slot = cutEdges[edgeKey];
    if (!slot)
    {
      slot = CUT_VERT_FLAG | cutVerts.size();
      lerp_vertex_data_v(cutVerts.push_back_noinit(), verts[i1], verts[i2], tttt);
    }
    return slot;
  };

  cut_face_data.edges.reserve(cut_face_data.edges.size() + candidateFacesVec.size());
  cutEdges.reserve(candidateFacesVec.size() * 2);
  cutVerts.reserve(candidateFacesVec.size());
  for (const auto &face : candidateFacesVec)
  {
    uint32_t idx0 = face.idx.data()[0];
    uint32_t idx1 = face.idx.data()[1];
    uint32_t idx2 = face.idx.data()[2];
    vec4f d = v_make_vec3f(vertDist[idx0], vertDist[idx1], vertDist[idx2]);

    vec4f dNonZeroMask = v_cmp_gt(v_abs(d), v_splats(EPS));
    d = v_and(d, dNonZeroMask); // snap to zero: d = abs(d) < EPS ? 0 : d
    vec4f dRot = v_perm_yzxw(d);
    // bit0 = i01, bit1 = i12, bit2 = i20
    const unsigned intersectMask = v_signmask(v_and(v_xor(d, dRot), v_and(dNonZeroMask, v_perm_yzxw(dNonZeroMask)))) & 7;
    const unsigned intersectCnt = dag::popcount(intersectMask);

    // no intersection or very rare case
    if (DAGOR_UNLIKELY(intersectCnt != 2))
    {
      // TODO: handle 1 edge intersection (e.i. d0=0, d1<0, d2>0)
      if (DAGOR_UNLIKELY((v_signmask(dNonZeroMask) & 7) == 0)) // on the plane, very rare case, push to both
      {
        DestrMesh::Face &fu = up_mesh.faces.push_back_noinit();
        fu.idx[0] = reindexVert(true, idx0);
        fu.idx[1] = reindexVert(true, idx1);
        fu.idx[2] = reindexVert(true, idx2);
        fu.mat = face.mat;
        DestrMesh::Face &fd = down_mesh.faces.push_back_noinit();
        fd.idx[0] = reindexVert(false, idx0);
        fd.idx[1] = reindexVert(false, idx1);
        fd.idx[2] = reindexVert(false, idx2);
        fd.mat = face.mat;
      }
      else
      {
        const bool toUp = (v_signmask(d) & 7) == 0; // all-positive -> up_mesh, else down_mesh
        DestrMesh::Face &f = (toUp ? up_mesh : down_mesh).faces.push_back_noinit();
        f.idx[0] = reindexVert(toUp, idx0);
        f.idx[1] = reindexVert(toUp, idx1);
        f.idx[2] = reindexVert(toUp, idx2);
        f.mat = face.mat;
      }
      continue;
    }

    // rotate so v0-v1 and v1-v2 are 2 intersections
    vec4i idx = v_make_vec3i(idx0, idx1, idx2);
    if (intersectMask == 0b110)
    {
      d = v_perm_yzxw(d);
      dRot = v_perm_yzxw(dRot);
      idx = v_cast_vec4i(v_perm_yzxw(v_cast_vec4f(idx)));
    }
    if (intersectMask == 0b101)
    {
      d = v_perm_zxyw(d);
      dRot = v_perm_zxyw(dRot);
      idx = v_cast_vec4i(v_perm_zxyw(v_cast_vec4f(idx)));
    }
    vec4f t = v_div(d, v_sub(d, dRot)); // .x = t01, .y = t12, .z = t20

    idx0 = v_extract_xi(idx);
    idx1 = v_extract_yi(idx);
    idx2 = v_extract_zi(idx);
    const uint32_t cutIdx0 = makeCutVert(idx0, idx1, v_splat_x(t));
    const uint32_t cutIdx1 = makeCutVert(idx1, idx2, v_splat_y(t));

    // store cut faces: lone vertex v1 -> 1 triangle on its side; quad (v0,v2) -> 2 triangles on the other
    const bool up = !(v_signmask(d) & 1);
    idx0 = reindexVert(up, idx0);
    idx1 = reindexVert(!up, idx1);
    idx2 = reindexVert(up, idx2);
    auto &__restrict quadFaces = up ? up_mesh.faces : down_mesh.faces;
    auto &__restrict loneFaces = up ? down_mesh.faces : up_mesh.faces;
    DestrMesh::Face &fl = loneFaces.push_back_noinit();
    quadFaces.resize_noinit(quadFaces.size() + 2);
    DestrMesh::Face *fq = quadFaces.data() + quadFaces.size() - 2;
    fl.idx[0] = cutIdx0;
    fl.idx[1] = idx1;
    fl.idx[2] = cutIdx1;
    fl.mat = face.mat;
    fq[0].idx[0] = cutIdx0;
    fq[0].idx[1] = cutIdx1;
    fq[0].idx[2] = idx0;
    fq[0].mat = face.mat;
    fq[1].idx[0] = idx0;
    fq[1].idx[1] = cutIdx1;
    fq[1].idx[2] = idx2;
    fq[1].mat = face.mat;

    // write cut indices
    if (ctx.materials[face.mat].isSolid)
    {
      auto &cutEdge = cut_face_data.edges.push_back_noinit();
      cutEdge.a = (!up ? cutIdx0 : cutIdx1) & ~CUT_VERT_FLAG;
      cutEdge.b = (!up ? cutIdx1 : cutIdx0) & ~CUT_VERT_FLAG;
    }
  }

  // flush cut vertices, project them on the plane
  {
    cut_face_data.basis = PlaneBasis(cut_plane);
    mat33f basis;
    basis.col0 = v_ldu(&cut_face_data.basis.U.x);
    basis.col1 = v_ldu(&cut_face_data.basis.V.x);
    basis.col2 = v_ldu(&cut_face_data.basis.plane.n.x);
    mat33f invBasis;
    v_mat33_orthonormal_inverse(invBasis, basis);
    const DestrMesh::Vertex *__restrict cutVertsPtr = cutVerts.data();
    cut_face_data.verts.reserve(cut_face_data.verts.size() + cutVerts.size());
    for (int i = 0, ie = cutVerts.size(); i != ie; i++)
    {
      vec4f v = v_ld(&cutVertsPtr[i].pos.x);
      v = v_mat33_mul_vec3(invBasis, v);
      v_st(&cut_face_data.verts.push_back_noinit(), v_perm_xyab(v, v_zero()));
    }
  }

  const auto finalizeCutFaces = [&](DestrMesh &dst_mesh, uint32_t cut_faces_start) FORCE_INLINE_LAMBDA {
    DestrMesh::Face *__restrict faces = dst_mesh.faces.data();
    const uint32_t facesEnd = dst_mesh.faces.size();
    const uint32_t cutOfs = dst_mesh.verts.size();
    for (uint32_t i = cut_faces_start; i != facesEnd; i++)
      for (uint32_t &idx : faces[i].idx)
        if (idx & CUT_VERT_FLAG)
          idx = (idx & ~CUT_VERT_FLAG) + cutOfs;
    dst_mesh.verts.resize_noinit(cutOfs + cutVerts.size());
    memcpy(dst_mesh.verts.data() + cutOfs, cutVerts.data(), sizeof(DestrMesh::Vertex) * cutVerts.size());
  };
  finalizeCutFaces(up_mesh, upMeshCutFacesStart);
  finalizeCutFaces(down_mesh, downMeshCutFacesStart);
}

} // namespace frx