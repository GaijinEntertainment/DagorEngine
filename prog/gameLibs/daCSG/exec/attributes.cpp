// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "exec/pipeline.h"
#include "mesh/solidUtil.h"
#include "mesh/unionFind.h"
#include "prims/prims.h"
#include <math/dag_check_nan.h>
#include <EASTL/algorithm.h>
#include <EASTL/heap.h>
#include <math.h>
#include <string.h>

namespace dacsg
{
void tangent_frame(const DPoint3 &n, DPoint3 &u, DPoint3 &v)
{
  const DPoint3 side = DPoint3(0, 1, 0) % n;
  u = lengthSq(side) > 1e-12 ? unit_or_zero(side) : unit_or_zero(DPoint3(1, 0, 0) - n * n.x);
  v = n % u;
}

// The chart of an output vertex under one record, shared by its corners; a radial below zero: not sampled yet
struct ChartSample
{
  double radial = -1, u = 0, v = 0;
};

// frame: the unit axis and its tangent_frame; false: a uv past the float range
static bool project_uv(const AttrRecord &r, const DPoint3 frame[3], const Mesh &m, uint32_t f, Channel &uv,
  dag::Vector<ChartSample> &cache)
{
  const AttrDesc &d = r.desc;
  DPoint3 root[3], p[3];
  for (int k = 0; k < 3; ++k)
  {
    root[k] = DPoint3(m.pos[m.tri[3 * f + k]]);
    p[k] = transform_point(r.fromRoot, root[k]);
  }
  double u[3], v[3];
  if (d.kind == AttrKind::UvPlanar || d.kind == AttrKind::UvBox)
  {
    DPoint3 tu = frame[1], tv = frame[2];
    if (d.kind == AttrKind::UvBox)
    {
      const DPoint3 rootNormal = ((root[1] - root[0]) % (root[2] - root[0])) * double(r.flip);
      const DPoint3 n(r.toRoot.col[0] * rootNormal, r.toRoot.col[1] * rootNormal, r.toRoot.col[2] * rootNormal);
      const int dominant = dominant_axis(n);
      DPoint3 axis(0, 0, 0);
      axis[dominant] = n[dominant] < 0 ? -1 : 1;
      tangent_frame(axis, tu, tv);
    }
    for (int k = 0; k < 3; ++k)
    {
      u[k] = (tu * p[k]) / d.scale;
      v[k] = (tv * p[k]) / d.scale;
    }
  }
  else
  {
    const DPoint3 &axis = frame[0], &tx = frame[1], tz = tx % axis;
    bool pole[3];
    double radial[3];
    for (int k = 0; k < 3; ++k)
    {
      ChartSample &s = cache[m.tri[3 * f + k]];
      if (s.radial < 0)
      {
        const double x = tx * p[k], y = axis * p[k], z = tz * p[k];
        s.radial = sqrt(x * x + z * z);
        s.u = atan2(z, x) / (2 * PI_D) + 0.5;
        s.v = d.kind == AttrKind::UvCyl ? y / d.scale : atan2(y, s.radial) / PI_D + 0.5;
      }
      radial[k] = s.radial;
      u[k] = s.u;
      v[k] = s.v;
    }
    // a turned cap centre rounds off the axis: a pole is a corner far closer to it than the face's farthest
    for (int k = 0; k < 3; ++k)
      pole[k] = radial[k] <= 1e-6 * max(max(radial[0], radial[1]), radial[2]);
    double best[3] = {0.5, 0.5, 0.5}, bestSpan = DBL_MAX, mean = 0.5;
    for (int ref = 0; ref < 3; ++ref)
    {
      if (pole[ref])
        continue;
      double w[3], lo = u[ref], hi = u[ref], sum = 0;
      int count = 0;
      for (int k = 0; k < 3; ++k)
      {
        const double turns = u[k] - u[ref];
        w[k] = u[ref] + turns - floor(turns + 0.5);
        if (pole[k])
          continue;
        lo = min(lo, w[k]);
        hi = max(hi, w[k]);
        sum += w[k];
        ++count;
      }
      if (hi - lo < bestSpan)
      {
        bestSpan = hi - lo;
        memcpy(best, w, sizeof(w));
        mean = sum / count;
      }
    }
    for (int k = 0; k < 3; ++k)
      u[k] = pole[k] ? mean : best[k];
  }
  for (int k = 0; k < 3; ++k)
    if (!narrow_float(u[k], uv.data[(3 * f + k) * uv.dim]) || !narrow_float(v[k], uv.data[(3 * f + k) * uv.dim + 1]))
      return false;
  return true;
}

// false: a uv past the float range
static bool apply_record(const AttrRecord &r, Mesh &out, dag::ConstSpan<uint32_t> leaf_start, dag::ConstSpan<uint32_t> leaf_faces,
  dag::Vector<uint8_t> &own_normal)
{
  const AttrDesc &d = r.desc;
  Channel *color = out.findChannel(CH_COLOR);
  Channel *uv = out.findChannel(CH_UV);
  DPoint3 frame[3] = {unit_or_zero(DPoint3(d.axis)), {}, {}};
  tangent_frame(frame[0], frame[1], frame[2]);
  dag::Vector<ChartSample> cache(d.kind == AttrKind::UvCyl || d.kind == AttrKind::UvSphere ? out.pos.size() : 0);
  for (uint32_t l = r.firstLeaf; l < r.endLeaf; ++l)
    for (uint32_t i = leaf_start[l]; i < leaf_start[l + 1]; ++i)
    {
      const uint32_t f = leaf_faces[i];
      FaceInfo &fi = out.face[f];
      if (d.kind == AttrKind::Color)
      {
        if (d.onlyUnset && (fi.attrSet & ATTR_SET_COLOR))
          continue;
        for (int k = 0; k < 3; ++k)
          memcpy(&color->data[(3 * f + k) * color->dim], d.color, sizeof(d.color));
        fi.attrSet |= ATTR_SET_COLOR;
      }
      else if (d.kind == AttrKind::Mat)
      {
        if (d.onlyUnset && (fi.attrSet & ATTR_SET_MAT))
          continue;
        fi.mat = d.mat;
        fi.attrSet |= ATTR_SET_MAT;
      }
      else if (d.kind == AttrKind::Flat || d.kind == AttrKind::Smooth)
      {
        fi.smgr = d.kind == AttrKind::Smooth ? SMOOTH : 0;
        own_normal[f] = 0;
      }
      else if (d.onlyUnset && (fi.attrSet & ATTR_SET_UV))
        continue;
      else if (d.kind == AttrKind::UvAffine)
        for (int k = 0; k < 3; ++k)
        {
          float *t = &uv->data[(3 * f + k) * uv->dim];
          const double u = t[0], v = t[1];
          for (int row = 0; row < 2; ++row)
            if (!narrow_float(d.uvAffine[row][0] * u + d.uvAffine[row][1] * v + d.uvAffine[row][2], t[row]))
              return false;
        }
      else
      {
        if (!project_uv(r, frame, out, f, *uv, cache))
          return false;
        fi.attrSet |= ATTR_SET_UV;
      }
    }
  return true;
}

struct ChannelSpec
{
  uint32_t id;
  uint8_t dim;
};

static void add_channel(dag::Vector<ChannelSpec> &set, uint32_t id, uint8_t dim)
{
  for (ChannelSpec &s : set)
    if (s.id == id)
    {
      s.dim = s.dim > dim ? s.dim : dim;
      return;
    }
  set.push_back(ChannelSpec{id, dim});
}

// within float rounding: a daSurf normal differs by 1e-16 between the faces of a vertex
static bool same_values(const Channel &c, const float *a, const float *b)
{
  for (uint32_t d = 0; d < c.dim; ++d)
    if (!(fabsf(a[d] - b[d]) <= 1e-6f * eastl::max(1.f, fabsf(a[d]))))
      return false;
  return true;
}

void compute_corner_classes(const SourceMesh &src)
{
  const Solid &s = src.solid;
  UnionFind classes;
  classes.init(s.halfedge.size());
  for (uint32_t h = 0; h < s.halfedge.size(); ++h)
  {
    const uint32_t t = s.halfedge[h].pairedHalfedge;
    if (t >= s.halfedge.size() || t < h)
      continue;
    // corner h is its halfedge's start: h and next_he(t) are one vertex
    for (const auto &c : {eastl::make_pair(h, next_he(t)), eastl::make_pair(next_he(h), t)})
      if (eastl::all_of(src.mesh.channels.begin(), src.mesh.channels.end(),
            [&](const Channel &ch) { return same_values(ch, &ch.data[c.first * ch.dim], &ch.data[c.second * ch.dim]); }))
        classes.join(c.first, c.second);
  }
  classes.compress(src.cornerClass);
}

static uint32_t corner_class(const Solid &leaf, const LeafSource &ls, uint32_t f, uint32_t v)
{
  uint32_t k = 0;
  while (leaf.faceVert(f, k) != v)
    ++k;
  return ls.src->cornerClass[3 * f + (ls.flipped ? (3 - k) % 3 : k)];
}

bool same_chart(const Lowered &lw, LeafRef a, LeafRef b)
{
  // a client kernel's ref outside the leaves keeps the border; the resolve reports it
  const auto missing = [&](LeafRef r) { return r.leaf >= lw.tree.leaves.size() || r.leafFace >= lw.tree.leaves[r.leaf].faceCount(); };
  if (missing(a) || missing(b))
    return false;
  a = lw.tree.leaves[a.leaf].leafRef[a.leafFace];
  b = lw.tree.leaves[b.leaf].leafRef[b.leafFace];
  if (a.leaf != b.leaf || a.leafFace == b.leafFace)
    return a.leaf == b.leaf;
  const Solid &leaf = lw.tree.leaves[a.leaf];
  bool shared = false;
  for (uint32_t i = 0; i < 3; ++i)
    for (uint32_t j = 0; j < 3; ++j)
      if (const uint32_t v = leaf.faceVert(a.leafFace, i); v == leaf.faceVert(b.leafFace, j))
      {
        if (corner_class(leaf, lw.sources[a.leaf], a.leafFace, v) != corner_class(leaf, lw.sources[a.leaf], b.leafFace, v))
          return false;
        shared = true;
      }
  return shared;
}

// a cleanup slide passes lf's neighbours; returns the chart's face nearest p, w its weights there
static uint32_t holding_face(const Solid &leaf, const LeafSource &ls, uint32_t lf, const DPoint3 &p, double tol, double w[3])
{
  const auto distance = [&](uint32_t f, double nw[3]) {
    const DPoint3 &a = leaf.pos[leaf.faceVert(f, 0)], &b = leaf.pos[leaf.faceVert(f, 1)], &c = leaf.pos[leaf.faceVert(f, 2)];
    closest_barycentric(p, a, b, c, nw);
    return length(a * nw[0] + b * nw[1] + c * nw[2] - p);
  };
  double nw[3], bestW[3];
  double best = distance(lf, nw);
  const double reach = best + 4 * tol;
  uint32_t bestFace = lf;
  dag::Vector<eastl::pair<double, uint32_t>> open = {eastl::make_pair(-reach, lf)};
  dag::Vector<uint32_t> seen = {lf};
  while (!open.empty() && best > tol && seen.size() < 4096)
  {
    eastl::pop_heap(open.begin(), open.end());
    const uint32_t f = open.back().second;
    open.pop_back();
    for (uint32_t k = 0; k < 3 && best > tol; ++k)
    {
      const Halfedge &h = leaf.halfedge[3 * f + k];
      const uint32_t g = h.pairedHalfedge / 3;
      const auto at = eastl::lower_bound(seen.begin(), seen.end(), g);
      if (h.pairedHalfedge >= leaf.halfedge.size() || (at != seen.end() && *at == g) ||
          corner_class(leaf, ls, f, h.startVert) != corner_class(leaf, ls, g, h.startVert) ||
          corner_class(leaf, ls, f, h.endVert) != corner_class(leaf, ls, g, h.endVert))
        continue;
      seen.insert(at, g);
      const double d = distance(g, nw);
      if (d < best)
      {
        best = d;
        bestFace = g;
        memcpy(bestW, nw, sizeof(nw));
      }
      if (d <= reach)
      {
        open.push_back(eastl::make_pair(-d, g));
        eastl::push_heap(open.begin(), open.end());
      }
    }
  }
  if (bestFace != lf)
    memcpy(w, bestW, sizeof(bestW));
  return bestFace;
}

bool resolve_attributes(const Lowered &lw, Solid &result, double tolerance, Mesh &out, dag::Vector<uint32_t> &face_group,
  dag::Vector<uint8_t> &own_normal, eastl::string &err)
{
  solid_to_mesh(result, out);
  const uint32_t faceCount = out.faceCount();
  const uint32_t leafCount = lw.tree.leaves.size();
  if (result.leafRef.size() != faceCount)
  {
    err.sprintf("kernel output has %u leaf references for %u faces", uint32_t(result.leafRef.size()), faceCount);
    return false;
  }
  // a clipped chain's leaf face names the chain leaf face it lies in
  dag::Vector<LeafRef> &refs = result.leafRef;
  for (uint32_t f = 0; f < faceCount; ++f)
  {
    LeafRef &ref = refs[f];
    if (ref.leaf >= leafCount || ref.leafFace >= lw.tree.leaves[ref.leaf].faceCount())
    {
      err.sprintf("kernel output face %u references a missing leaf face", f);
      return false;
    }
    ref = lw.tree.leaves[ref.leaf].leafRef[ref.leafFace];
  }

  dag::Vector<ChannelSpec> channels;
  for (const LeafSource &ls : lw.sources)
    for (const Channel &c : ls.src->mesh.channels)
    {
      add_channel(channels, c.id, c.dim);
      const char *name = ls.src->mesh.userChannelName(c.id);
      if (name && !out.userChannelName(c.id))
        out.setUserChannelName(c.id, name);
    }
  for (const ChannelSpec &s : channels)
    out.addChannel(s.id, s.dim);
  out.addDefaultChannels();

  const uint32_t channelCount = out.channels.size();
  dag::Vector<const Channel *> leafChannel(leafCount * channelCount, nullptr);
  for (uint32_t l = 0; l < leafCount; ++l)
    for (uint32_t c = 0; c < channelCount; ++c)
      leafChannel[l * channelCount + c] = lw.sources[l].src->mesh.findChannel(out.channels[c].id);
  Channel *normal = out.findChannel(CH_NORMAL);
  // per leaf the inverse transpose of its tm's linear part, up to a positive factor: the cofactor columns times the
  // determinant's sign, applied to the sum as before, so a zero keeps its sign bit
  dag::Vector<DPoint3> cofactors(leafCount * 3);
  dag::Vector<double> detSign(leafCount);
  for (uint32_t l = 0; l < leafCount; ++l)
  {
    const DPoint3 &a = lw.sources[l].tm.col[0], &b = lw.sources[l].tm.col[1], &c = lw.sources[l].tm.col[2];
    detSign[l] = a * (b % c) < 0 ? -1 : 1;
    cofactors[3 * l] = b % c;
    cofactors[3 * l + 1] = c % a;
    cofactors[3 * l + 2] = a % b;
  }

  face_group.resize(faceCount);
  own_normal.assign(faceCount, 0);
  for (uint32_t f = 0; f < faceCount; ++f)
  {
    const LeafRef &ref = refs[f];
    const Solid &leaf = lw.tree.leaves[ref.leaf];
    const LeafSource &ls = lw.sources[ref.leaf];
    out.face[f] = leaf.face[ref.leafFace];
    face_group[f] = ls.groupBase + (ls.src->faceGroup.empty() ? 0 : ls.src->faceGroup[ref.leafFace]);
    const DPoint3 &a = leaf.pos[leaf.faceVert(ref.leafFace, 0)];
    const DPoint3 &b = leaf.pos[leaf.faceVert(ref.leafFace, 1)];
    const DPoint3 &c = leaf.pos[leaf.faceVert(ref.leafFace, 2)];
    // cleanup edits move corners off their source
    uint32_t srcCorner[3][3];
    double w[3][3];
    for (uint32_t k = 0; k < 3; ++k)
    {
      const DPoint3 p(out.pos[out.tri[3 * f + k]]);
      closest_barycentric(p, a, b, c, w[k]);
      const bool held = lengthSq(a * w[k][0] + b * w[k][1] + c * w[k][2] - p) <= tolerance * tolerance;
      const uint32_t lf = held ? ref.leafFace : holding_face(leaf, ls, ref.leafFace, p, tolerance, w[k]);
      for (uint32_t i = 0; i < 3; ++i)
        srcCorner[k][i] = 3 * lf + (ls.flipped ? (3 - i) % 3 : i);
    }
    for (uint32_t ch = 0; ch < channelCount; ++ch)
    {
      const Channel *src = leafChannel[ref.leaf * channelCount + ch];
      if (!src)
        continue;
      Channel &dst = out.channels[ch];
      for (uint32_t d = 0; d < dst.dim && d < src->dim; ++d)
      {
        double s[3][3];
        for (uint32_t k = 0; k < 3; ++k)
          for (uint32_t i = 0; i < 3; ++i)
            s[k][i] = k && srcCorner[k][i] == srcCorner[0][i] ? s[0][i] : src->data[srcCorner[k][i] * src->dim + d];
        // the leading 0.0 keeps zero sums +0
        for (uint32_t k = 0; k < 3; ++k)
          dst.data[(3 * f + k) * dst.dim + d] = float(0.0 + w[k][0] * s[k][0] + w[k][1] * s[k][1] + w[k][2] * s[k][2]);
      }
    }
    if (normal && !ls.src->ownNormal.empty() && ls.src->ownNormal[ref.leafFace])
    {
      own_normal[f] = 1;
      // a subtracted operand's face turns over
      const DPoint3 o0(out.pos[out.tri[3 * f]]), o1(out.pos[out.tri[3 * f + 1]]), o2(out.pos[out.tri[3 * f + 2]]);
      const double side = ((o1 - o0) % (o2 - o0)) * ((b - a) % (c - a)) < 0 ? -1 : 1;
      for (uint32_t k = 0; k < 3; ++k)
      {
        float *n = &normal->data[(3 * f + k) * 3];
        const DPoint3 *m = &cofactors[3 * ref.leaf];
        const Point3 mapped(unit_or_zero((m[0] * n[0] + m[1] * n[1] + m[2] * n[2]) * detSign[ref.leaf]) * side);
        memcpy(n, &mapped, sizeof(mapped));
      }
    }
  }

  dag::Vector<uint32_t> leafStart, leafFaces;
  bucket_items(faceCount, leafCount, [&](uint32_t f) { return refs[f].leaf; }, leafStart, leafFaces);
  for (const AttrRecord &r : lw.attrs)
    if (!apply_record(r, out, leafStart, leafFaces, own_normal))
      return (err = "a uv projection or a uv affine gives a uv beyond the float range", false);
  return true;
}
} // namespace dacsg
