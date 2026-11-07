// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include <string.h>

namespace dacsg
{
uint64_t compute_output_normals(const Mesh &m, dag::ConstSpan<uint32_t> face_group, dag::ConstSpan<uint8_t> keep,
  dag::Vector<float> &normal)
{
  uint64_t visits = 0;
  const uint32_t faceCount = m.faceCount(), cornerCount = faceCount * 3;
  dag::Vector<uint32_t> start, corner;
  bucket_items(cornerCount, m.pos.size(), [&](uint32_t c) { return m.tri[c]; }, start, corner);
  dag::Vector<DPoint3> faceNormal(faceCount), weighted(cornerCount, DPoint3(0, 0, 0));
  for (uint32_t f = 0; f < faceCount; ++f)
  {
    DPoint3 p[3];
    for (int k = 0; k < 3; ++k)
      p[k] = DPoint3(m.pos[m.tri[3 * f + k]]);
    faceNormal[f] = unit_or_zero((p[1] - p[0]) % (p[2] - p[0]));
    // a flat face's corners take its normal below: no weights
    for (int k = 0; m.face[f].smgr && k < 3; ++k)
    {
      const DPoint3 e1 = p[(k + 1) % 3] - p[k], e2 = p[(k + 2) % 3] - p[k];
      weighted[3 * f + k] = faceNormal[f] * atan2(length(e1 % e2), e1 * e2);
    }
  }
  // the engine rule, no transitive group: a corner averages its own face and the faces at its vertex from its leaf
  // instance whose bits meet its face's bits. Every corner of one instance and one mask reads the same fan, so the fan
  // is summed once per distinct mask of the vertex: a vertex costs its corners times its masks, not its corners squared
  const auto kept = [&](uint32_t f) { return f < keep.size() && keep[f]; };
  // the corners at one vertex from one leaf instance whose smgr bits meet one mask
  struct FanMask
  {
    uint32_t group = 0, bits = 0;
    uint32_t firstKept = NO_INDEX;  // the first kept corner the mask joins
    DPoint3 sum = DPoint3(0, 0, 0); // their weighted normals, in corner order
  };
  dag::Vector<FanMask> masks;
  const auto find_mask = [&](uint32_t f) -> FanMask * {
    for (FanMask &k : masks)
      if (k.group == face_group[f] && k.bits == m.face[f].smgr)
        return &k;
    return nullptr;
  };
  normal.resize(cornerCount * 3);
  for (uint32_t v = 0; v + 1 < start.size(); ++v)
  {
    masks.clear();
    for (uint32_t i = start[v]; i < start[v + 1]; ++i)
      if (const uint32_t f = corner[i] / 3; m.face[f].smgr && !find_mask(f))
        masks.push_back(FanMask{face_group[f], m.face[f].smgr});
    for (uint32_t i = start[v]; i < start[v + 1]; ++i)
      for (FanMask &k : masks)
      {
        const uint32_t d = corner[i], f = d / 3;
        ++visits;
        if (face_group[f] != k.group || !(m.face[f].smgr & k.bits))
          continue;
        k.sum += weighted[d];
        if (k.firstKept == NO_INDEX && kept(f))
          k.firstKept = d;
      }
    for (uint32_t i = start[v]; i < start[v + 1]; ++i)
    {
      const uint32_t c = corner[i], f = c / 3;
      const FanMask *k = find_mask(f);
      float *n = &normal[3 * c];
      ++visits;
      if (kept(f))
      {
        // a kept corner takes the value of the first kept corner the rule joins it to when they nearly agree, so an
        // export welds them
        const float *r = k && k->firstKept < c ? &normal[3 * k->firstKept] : nullptr;
        if (r && sqr(n[0] - r[0]) + sqr(n[1] - r[1]) + sqr(n[2] - r[2]) < 1e-8f)
          memcpy(n, r, 3 * sizeof(float));
        continue;
      }
      DPoint3 u = unit_or_zero(k ? k->sum : weighted[c]);
      // the zero drops a negative zero, which would split a flat quad's triangles at the export
      if (lengthSq(u) == 0)
        u = DPoint3(0, 0, 0) + faceNormal[f];
      const Point3 out(u);
      memcpy(n, &out, sizeof(out));
    }
  }
  return visits;
}
} // namespace dacsg
