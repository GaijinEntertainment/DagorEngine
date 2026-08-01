// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <memory/dag_framemem.h>
#include <vecmath/dag_vecMath.h>
#include <dag/dag_vectorMap.h>
#include <math/random/dag_random.h>
#include <math/dag_mathUtils.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_relocatableFixedVector.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include <daFracture/core/destrMesh.h>
#include <daFracture/core/meshCommon.h>
#include <daFracture/core/meshSlicing.h>
#include <daFracture/core/cutFaceFill.h>

#include "cutMeshCommon.h"
#include "cutMeshPlane.h"
#include "cutFaceFill.h"


namespace frx
{

void mesh_slice(DestrContext &ctx, const DestrMesh &mesh, DestrMesh *up_mesh, DestrMesh *down_mesh, Plane3 plane, uint16_t cut_mat_id,
  float push_dist)
{
  if (!up_mesh && !down_mesh)
    return;

  plane3f localPlane = transform_plane_to_local(v_ldu(&plane.n.x), mesh.tm);
  ctx.dbgDraw.tm = mesh.tm;

  DestrMesh meshToDiscard;
  DestrMesh &upMeshRef = up_mesh ? *up_mesh : meshToDiscard;
  DestrMesh &downMeshRef = down_mesh ? *down_mesh : meshToDiscard;
  upMeshRef.tm = downMeshRef.tm = mesh.tm;
  CutFaceData cutFaceData; // intentionally not framemem, fill_cut_faces need a lot of it

  // crude estimate
  upMeshRef.verts.reserve(mesh.verts.size());
  downMeshRef.verts.reserve(mesh.verts.size());
  upMeshRef.faces.reserve(mesh.faces.size());
  downMeshRef.faces.reserve(mesh.faces.size());
  cutFaceData.edges.reserve(16 + mesh.faces.size() / 4);
  cutFaceData.verts.reserve(cutFaceData.edges.size() / 2);

  {
    FRAMEMEM_REGION; // note: safe because cutFaceData and meshes are not framemem
    cut_mesh_plane_impl(ctx, mesh, upMeshRef, downMeshRef, localPlane, cutFaceData);
  }
  fill_cut_faces(ctx, cutFaceData, up_mesh, down_mesh, cut_mat_id);

  if (down_mesh)
    down_mesh->tm.setcol(3, down_mesh->tm.getcol(3) - plane.n * push_dist);
  if (up_mesh)
    up_mesh->tm.setcol(3, up_mesh->tm.getcol(3) + plane.n * push_dist);
}


void ShatterMaterialProfile::loadFromBlk(const DataBlock &blk)
{
  minPieces = blk.getInt("minPieces", minPieces);
  maxPieces = blk.getInt("maxPieces", maxPieces);
  impactPointCuts = blk.getInt("impactPointCuts", impactPointCuts);
  relativeSizeWindow = blk.getPoint2("relativeSizeWindow", relativeSizeWindow);
  minCutRatio = blk.getReal("minCutRatio", minCutRatio);

  sizeModes.clear();
  cutPlaneModes.clear();
  const int sizeModeNid = blk.getNameId("sizeMode");
  const int cutPlaneModeNid = blk.getNameId("cutPlaneMode");
  for (int i = 0, n = blk.blockCount(); i < n; i++)
  {
    const DataBlock &b = *blk.getBlock(i);
    if (b.getBlockNameId() == sizeModeNid)
    {
      SizeMode &m = sizeModes.push_back();
      m.weight = b.getReal("weight", m.weight);
      m.powerRange = b.getPoint2("powerRange", m.powerRange);
      m.sizeRange = b.getPoint2("sizeRange", m.sizeRange);
    }
    else if (b.getBlockNameId() == cutPlaneModeNid)
    {
      CutPlaneMode &m = cutPlaneModes.push_back();
      m.weight = b.getReal("weight", m.weight);
      // authored in degrees (deviation from the long axis), stored as cosine
      const Point2 angleRangeDeg = b.getPoint2("angleRangeDeg", Point2::ZERO);
      m.cosineRange = Point2(cosf(angleRangeDeg.x * DEG_TO_RAD), cosf(angleRangeDeg.y * DEG_TO_RAD));
    }
  }

  // shorthand: with no explicit cutPlaneMode blocks, init a single jitter mode of [0, cutPlaneJitterDeg] degrees
  if (const float jitterDeg = blk.getReal("cutPlaneJitterDeg", -1.f); cutPlaneModes.empty() && jitterDeg >= 0.f)
  {
    CutPlaneMode &m = cutPlaneModes.push_back();
    m.cosineRange = Point2(1.f, cosf(jitterDeg * DEG_TO_RAD));
  }
}


void shatter_into_pieces(DestrContext &ctx, DestrSystem &sys, int start_piece_idx, uint16_t interior_mat,
  const ShatterImpactProfile &impact, const ShatterMaterialProfile &material, int &seed)
{
  TIME_PROFILE(shatter_into_pieces)
  FRAMEMEM_REGION;
  const int maxPieces = material.maxPieces;
  const int minPieces = material.minPieces;

  const auto getPower = [&](const Point3 &p) {
    const float t = cvt(length(p - impact.pos), impact.radiusRange.x, impact.radiusRange.y, 0.f, 1.f);
    return cvt(powf(t, impact.falloffPow), 0.f, 1.f, impact.powerRange.x, impact.powerRange.y);
  };

  const auto pickTargetPieceSize = [&](const Point3 &p, float upper_limit) -> float {
    const float power = getPower(p);
    const auto eligible = [&](const ShatterMaterialProfile::SizeMode &m) {
      return power >= m.powerRange.x && power <= m.powerRange.y && m.sizeRange.x <= upper_limit;
    };
    float total = 0.f;
    for (const auto &m : material.sizeModes)
      if (eligible(m))
        total += m.weight * m.sizeRange.x; // small pieces are compensated to have smaller real weight
    if (total <= 0.f)
      return -1.f;
    float r = _frnd(seed) * total;
    for (const auto &m : material.sizeModes)
      if (eligible(m))
      {
        r -= m.weight * m.sizeRange.x;
        if (r <= 0.f)
          return lerp(m.sizeRange.x, min(m.sizeRange.y, upper_limit), _frnd(seed));
      }
    return -1.f;
  };

  const auto pickCutNormal = [&](const Point3 &ref) -> Point3 {
    if (material.cutPlaneModes.empty())
      return ref;
    float total = 0.f;
    for (const auto &m : material.cutPlaneModes)
      total += m.weight;
    float r = _frnd(seed) * max(total, 1e-6f);
    const ShatterMaterialProfile::CutPlaneMode *chosen = &material.cutPlaneModes[0];
    for (const auto &m : material.cutPlaneModes)
    {
      r -= m.weight;
      if (r <= 0.f)
      {
        chosen = &m;
        break;
      }
    }
    const float cosT = clamp(lerp(chosen->cosineRange.x, chosen->cosineRange.y, _frnd(seed)), -1.f, 1.f);
    const float sinT = sqrtf(max(0.f, 1.f - cosT * cosT));
    const Point3 t = normalize(cross(ref, fabsf(ref.x) < 0.9f ? Point3(1, 0, 0) : Point3(0, 1, 0)));
    const Point3 b = cross(ref, t);
    const float phi = _srnd(seed) * PI;
    const Point3 azim = cosf(phi) * t + sinf(phi) * b;
    return cosT * ref + sinT * azim;
  };

  struct PieceInfo
  {
    Point3 worldNormal = Point3(0, 1, 0);
    Point3 p1 = Point3::ZERO, p2 = Point3::ZERO;
    float pieceSize = 0.f;
    float pickWeight = 0.f;
  };
  const auto findPieceLongAxis = [&](const DestrMesh &piece) {
    G_ASSERT(!piece.verts.empty());
    PieceInfo r;

    Point3 c = Point3::ZERO;
    for (const auto &v : piece.verts)
      c += v.pos;
    c *= safeinv(float(piece.verts.size()));

    Point3 p1 = c;
    float bestSq = 0.f;
    for (const auto &v : piece.verts)
    {
      const float s = lengthSq(v.pos - c);
      if (s > bestSq)
      {
        bestSq = s;
        p1 = v.pos;
      }
    }
    Point3 p2 = p1;
    bestSq = 0.f;
    for (const auto &v : piece.verts)
    {
      const float s = lengthSq(v.pos - p1);
      if (s > bestSq)
      {
        bestSq = s;
        p2 = v.pos;
      }
    }
    if (bestSq < 1e-8f)
      return r;

    const float diameter = sqrtf(bestSq);
    const Point3 localAxis = (p2 - p1) * (1.f / diameter);
    r.worldNormal = normalize(piece.tm % localAxis);
    r.pieceSize = diameter;
    r.pickWeight = diameter;
    r.p1 = piece.tm * p1;
    r.p2 = piece.tm * p2;
    return r;
  };

  dag::VectorMap<int, PieceInfo, eastl::less<int>, framemem_allocator> piecesToCut;
  mesh_normalize_transform(ctx, sys.pieces[start_piece_idx]);
  piecesToCut.reserve(maxPieces);
  piecesToCut.emplace(start_piece_idx, findPieceLongAxis(sys.pieces[start_piece_idx]));

  const auto pushNewPiece = [&](DestrMesh &&mesh) {
    if (!mesh.faces.empty())
    {
      const int pieceId = sys.pieces.back().first + 1;
      mesh_normalize_transform(ctx, mesh);
      piecesToCut.emplace(pieceId, findPieceLongAxis(mesh));
      sys.pieces[pieceId] = eastl::move(mesh);
      return true;
    }
    return false;
  };

  sys.pieces.reserve(sys.pieces.size() + maxPieces);
  int cutsDone = 0;
  for (int iterGuard = 0; !piecesToCut.empty() && int(sys.pieces.size()) < maxPieces && iterGuard < 4096; iterGuard++)
  {
    float total = 0.f;
    for (const auto &[idx, info] : piecesToCut)
      total += info.pickWeight;
    float pick = _frnd(seed) * total;
    int selected = int(piecesToCut.size()) - 1, scan = 0;
    for (const auto &[idx, info] : piecesToCut)
    {
      if ((pick -= info.pickWeight) <= 0.f)
      {
        selected = scan;
        break;
      }
      ++scan;
    }
    const auto [pieceIdx, axisInfo] = piecesToCut.data()[selected];

    const float sizeUpperConstraint = axisInfo.pieceSize * material.relativeSizeWindow.y;
    float theta1 = pickTargetPieceSize(lerp(axisInfo.p1, axisInfo.p2, 0.25f), sizeUpperConstraint);
    if (theta1 < 0.f)
      theta1 = axisInfo.pieceSize;
    float theta2 = pickTargetPieceSize(lerp(axisInfo.p1, axisInfo.p2, 0.75f), sizeUpperConstraint);
    if (theta2 < 0.f)
      theta2 = axisInfo.pieceSize;
    if (int(sys.pieces.size()) >= minPieces && axisInfo.pieceSize * material.relativeSizeWindow.x <= min(theta1, theta2))
    {
      piecesToCut.erase(pieceIdx);
      continue;
    }

    const float cutT = lerp(material.minCutRatio, 1.f - material.minCutRatio, theta1 / (theta1 + theta2));
    Point3 cutCenter = lerp(axisInfo.p1, axisInfo.p2, cutT);
    if (cutsDone < material.impactPointCuts)
    {
      const Point3 axis = axisInfo.p2 - axisInfo.p1;
      const float axisLen2 = lengthSq(axis);
      const float tRaw = axisLen2 > 1e-8f ? (impact.pos - axisInfo.p1) * axis / axisLen2 : 0.5f;
      cutCenter = (tRaw >= material.minCutRatio && tRaw <= 1.f - material.minCutRatio)
                    ? impact.pos
                    : lerp(axisInfo.p1, axisInfo.p2, clamp(tRaw, material.minCutRatio, 1.f - material.minCutRatio));
    }
    const Point3 cutNormal = pickCutNormal(axisInfo.worldNormal);
    const Plane3 cutPlane(cutNormal, cutCenter);

    DestrMesh mesh = eastl::move(sys.pieces[pieceIdx]);
    G_ASSERT(!mesh.faces.empty());
    DestrMesh upMesh, downMesh;
    mesh_slice(ctx, mesh, &upMesh, &downMesh, cutPlane, interior_mat, 0.0f);
    pushNewPiece(eastl::move(upMesh));
    pushNewPiece(eastl::move(downMesh));
    piecesToCut.erase(pieceIdx);
    sys.pieces.erase(pieceIdx);
    cutsDone++;
  }

  DA_PROFILE_TAG(shatter_into_pieces, "cuts:%d", cutsDone)
}

} // namespace frx
