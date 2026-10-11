// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "corSkinning.h"
#include <math/dag_mesh.h>
#include <util/dag_parallelForInline.h>
#include <util/dag_threadPool.h>
#include <perfMon/dag_cpuFreq.h>
#include <debug/dag_debug.h>
#include <hash/wyhash.h>
#include <EASTL/vector.h>
#include <EASTL/sort.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <math.h>

namespace
{
constexpr float SIMILARITY_SIGMA = 0.1f;
constexpr float SUBDIV_WEIGHT_EPSILON = 0.1f;
constexpr int MAX_SUBDIV_DEPTH = 3;
constexpr int MAX_FACE_BONES = 3 * MAX_SKINBONES;
constexpr float GROUP_WEIGHT_STEPS = 32.f;
constexpr uint32_t ITEMS_PER_JOB = 16;

// The similarity of a sub triangle to a vertex depends on the weights alone, so the sub triangles with the
// same quantized weight vector share one group, which sums their area and area-weighted centroid.
struct WeightGroupKey
{
  int16_t bone[MAX_FACE_BONES] = {};
  uint8_t q[MAX_FACE_BONES] = {};
  int16_t count = 0;
  bool operator==(const WeightGroupKey &) const = default;
};

static_assert(eastl::has_unique_object_representations_v<WeightGroupKey>, "WeightGroupKeyHash hashes raw bytes");
struct WeightGroupKeyHash
{
  size_t operator()(const WeightGroupKey &k) const { return wyhash(&k, sizeof(k), 0); }
};

struct WeightGroup : WeightGroupKey
{
  float weight[MAX_FACE_BONES] = {};
  DPoint3 sumAreaCenter = DPoint3(0, 0, 0);
  double sumArea = 0, sumAreaWeight[MAX_FACE_BONES] = {};
};

struct DenseCorner
{
  Point3 p = Point3(0, 0, 0);
  float w[MAX_FACE_BONES] = {};
};

int bone_index(const int16_t *bone, int count, int16_t b)
{
  for (int i = 0; i < count; i++)
    if (bone[i] == b)
      return i;
  return -1;
}

struct GroupedMesh
{
  ska::flat_hash_map<WeightGroupKey, uint32_t, WeightGroupKeyHash> groupIndex;
  eastl::vector<WeightGroup> groups;
  ska::flat_hash_map<uint32_t, eastl::vector<uint32_t>> pairGroups;
};

uint32_t bone_pair_key(int16_t a, int16_t b)
{
  return a < b ? (uint32_t(uint16_t(a)) << 16) | uint16_t(b) : (uint32_t(uint16_t(b)) << 16) | uint16_t(a);
}

float weight_distance(const float *a, const float *b, int n)
{
  float d = 0;
  for (int i = 0; i < n; i++)
    d += fabsf(a[i] - b[i]);
  return d;
}

DenseCorner midpoint(const DenseCorner &a, const DenseCorner &b, int n)
{
  DenseCorner m;
  m.p = (a.p + b.p) * 0.5f;
  for (int i = 0; i < n; i++)
    m.w[i] = (a.w[i] + b.w[i]) * 0.5f;
  return m;
}

void subdivide_face(const DenseCorner &c0, const DenseCorner &c1, const DenseCorner &c2, const WeightGroupKey &bones, int depth,
  float max_edge_weight_distance, GroupedMesh &out)
{
  const int n = bones.count;
  if (depth < MAX_SUBDIV_DEPTH && max_edge_weight_distance > SUBDIV_WEIGHT_EPSILON)
  {
    const DenseCorner m01 = midpoint(c0, c1, n), m12 = midpoint(c1, c2, n), m20 = midpoint(c2, c0, n);
    // A split halves every edge's weight distance.
    const float half = max_edge_weight_distance * 0.5f;
    subdivide_face(c0, m01, m20, bones, depth + 1, half, out);
    subdivide_face(m01, c1, m12, bones, depth + 1, half, out);
    subdivide_face(m20, m12, c2, bones, depth + 1, half, out);
    subdivide_face(m01, m12, m20, bones, depth + 1, half, out);
    return;
  }

  const float area = length((c1.p - c0.p) % (c2.p - c0.p)) * 0.5f;
  if (area <= 0)
    return;
  float w[MAX_FACE_BONES];
  WeightGroupKey key = bones;
  for (int i = 0; i < n; i++)
  {
    w[i] = (c0.w[i] + c1.w[i] + c2.w[i]) / 3.f;
    key.q[i] = uint8_t(w[i] * GROUP_WEIGHT_STEPS + 0.5f);
  }
  auto ins = out.groupIndex.emplace(key, uint32_t(out.groups.size()));
  WeightGroup &g = ins.second ? out.groups.emplace_back(key) : out.groups[ins.first->second];
  g.sumArea += area;
  g.sumAreaCenter += DPoint3((c0.p + c1.p + c2.p) / 3.f) * area;
  for (int i = 0; i < n; i++)
    g.sumAreaWeight[i] += double(area) * w[i];
}

void build_grouped_mesh(dag::ConstSpan<Point3> vert, dag::ConstSpan<Face> face, dag::ConstSpan<CorSkinWeights> skin, GroupedMesh &out)
{
  for (const Face &f : face)
  {
    WeightGroupKey fb;
    for (int k = 0; k < 3; k++)
    {
      const CorSkinWeights &sw = skin[f.v[k]];
      for (int j = 0; j < sw.count; j++)
        if (bone_index(fb.bone, fb.count, sw.bone[j]) < 0)
          fb.bone[fb.count++] = sw.bone[j];
    }
    if (fb.count < 2)
      continue;
    // Sorted once here, so every sub triangle's group key is canonical.
    eastl::sort(fb.bone, fb.bone + fb.count);

    DenseCorner c[3];
    for (int k = 0; k < 3; k++)
    {
      c[k].p = vert[f.v[k]];
      const CorSkinWeights &sw = skin[f.v[k]];
      for (int j = 0; j < sw.count; j++)
        c[k].w[bone_index(fb.bone, fb.count, sw.bone[j])] += sw.weight[j];
    }
    const int n = fb.count;
    const float maxEdgeDist =
      max(weight_distance(c[0].w, c[1].w, n), max(weight_distance(c[1].w, c[2].w, n), weight_distance(c[2].w, c[0].w, n)));
    subdivide_face(c[0], c[1], c[2], fb, 0, maxEdgeDist, out);
  }

  for (uint32_t gi = 0; gi < out.groups.size(); gi++)
  {
    WeightGroup &g = out.groups[gi];
    for (int i = 0; i < g.count; i++)
      g.weight[i] = float(g.sumAreaWeight[i] / g.sumArea);
    for (int a = 0; a < g.count; a++)
      for (int b = a + 1; b < g.count; b++)
        out.pairGroups[bone_pair_key(g.bone[a], g.bone[b])].push_back(gi);
  }
}

struct Scratch
{
  eastl::vector<uint32_t> candidates, groupStamp;
  uint32_t stamp = 0;
};

// Zero when no sub triangle shares two bones with the vertex, so it stays on linear blend skinning.
Point3 center_offset(const CorSkinWeights &vw, const Point3 &pos, const GroupedMesh &m, Scratch &sc)
{
  sc.candidates.clear();
  sc.stamp++;
  for (int a = 0; a < vw.count; a++)
    for (int b = a + 1; b < vw.count; b++)
      if (auto it = m.pairGroups.find(bone_pair_key(vw.bone[a], vw.bone[b])); it != m.pairGroups.end())
        for (uint32_t g : it->second)
          if (eastl::exchange(sc.groupStamp[g], sc.stamp) != sc.stamp)
            sc.candidates.push_back(g);

  const float invSigmaSq = 1.f / (SIMILARITY_SIGMA * SIMILARITY_SIGMA);
  DPoint3 num(0, 0, 0);
  double den = 0;
  for (uint32_t gi : sc.candidates)
  {
    const WeightGroup &g = m.groups[gi];
    int idx[MAX_SKINBONES];
    for (int a = 0; a < vw.count; a++)
      idx[a] = bone_index(g.bone, g.count, vw.bone[a]);
    float s = 0;
    for (int a = 0; a < vw.count; a++)
      for (int b = a + 1; b < vw.count; b++)
        if (idx[a] >= 0 && idx[b] >= 0)
        {
          const float pj = vw.weight[a], pk = vw.weight[b], vj = g.weight[idx[a]], vk = g.weight[idx[b]];
          const float x = pj * vk - pk * vj;
          s += pj * vk * pk * vj * expf(-x * x * invSigmaSq);
        }
    num += g.sumAreaCenter * s;
    den += s * g.sumArea;
  }
  if (den <= 1e-30)
    return Point3(0, 0, 0);
  return Point3(float(num.x / den), float(num.y / den), float(num.z / den)) - pos;
}
} // namespace

void compute_centers_of_rotation(dag::ConstSpan<Point3> vert, dag::ConstSpan<Face> face, dag::ConstSpan<CorSkinWeights> skin,
  dag::Span<Point4> out_cor_offset)
{
  G_ASSERT(skin.size() == vert.size() && out_cor_offset.size() == vert.size());
  const int64_t refTime = ref_time_ticks();

  GroupedMesh mesh;
  build_grouped_mesh(vert, face, skin, mesh);

  eastl::vector<Scratch> scratch(threadpool::get_num_workers() + 1, Scratch{{}, eastl::vector<uint32_t>(mesh.groups.size(), 0)});
  threadpool::parallel_for_inline(0, vert.size(), ITEMS_PER_JOB, [&](uint32_t begin, uint32_t end, uint32_t thread_id) {
    for (uint32_t i = begin; i < end; i++)
      out_cor_offset[i] = Point4::xyz0(center_offset(skin[i], vert[i], mesh, scratch[thread_id]));
  });

  int withOffset = 0;
  float maxOffset = 0;
  double sumOffset = 0;
  for (const Point4 &o : out_cor_offset)
  {
    const float len = length(Point3::xyz(o));
    withOffset += len > 0;
    maxOffset = max(maxOffset, len);
    sumOffset += len;
  }
  debug("  cor skinning: %d of %d verts have a non-zero center offset, max=%.4f avg=%.4f, %d weight groups, %.2f s", withOffset,
    vert.size(), maxOffset, withOffset ? float(sumOffset / withOffset) : 0.f, mesh.groups.size(), get_time_usec(refTime) * 1e-6f);
}
