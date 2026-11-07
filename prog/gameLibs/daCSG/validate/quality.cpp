// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include <gameLibs/daCSG/validate_api.h>
#include <math/dag_check_nan.h>
#include <EASTL/sort.h>
#include <float.h>

namespace dacsg
{
QualityReport quality(const Solid &s, const CleanupParams &p)
{
  QualityReport r;
  r.triangles = s.faceCount();
  dag::Vector<uint32_t> pair;
  const EdgeCounts edges = pair_halfedges(s.halfedge, s.pos.size(), pair);
  r.nonManifoldEdges = edges.unpaired + edges.overPairedEdges;
  if (edges.badIndex || s.halfedge.size() != 3 * s.faceCount())
    return r;
  for (const DPoint3 &v : s.pos) // the duplicate sort below needs an order
    if (!check_finite(v.x) || !check_finite(v.y) || !check_finite(v.z))
      return r;

  double shortest = DBL_MAX;
  for (uint32_t f = 0; f < s.faceCount(); ++f)
  {
    const FaceShape fs = face_shape(s, f);
    r.minAngle.add(fs.minAngleDeg);
    r.slivers += is_sliver(fs, p);
    r.degenerateFaces += !(fs.height > 0);
    shortest = min(shortest, fs.edgeLen[fs.shortest]);
  }
  r.shortestEdge = s.faceCount() ? shortest : 0;

  dag::Vector<TriPair> skins;
  skin_pairs(s, skins);
  r.skinFaces = 2 * skins.size();

  dag::Vector<uint32_t> order(s.pos.size());
  for (uint32_t i = 0; i < order.size(); ++i)
    order[i] = i;
  const auto less = [&](uint32_t a, uint32_t b) {
    const DPoint3 &pa = s.pos[a], &pb = s.pos[b];
    return pa.x != pb.x ? pa.x < pb.x : pa.y != pb.y ? pa.y < pb.y : pa.z < pb.z;
  };
  eastl::sort(order.begin(), order.end(), less);
  for (uint32_t i = 1; i < order.size(); ++i)
    r.duplicateVerts += s.pos[order[i]] == s.pos[order[i - 1]];
  return r;
}
} // namespace dacsg
