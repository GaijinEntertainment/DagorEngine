// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mesh/solidUtil.h"
#include "mesh/unionFind.h"
#include <gameLibs/daCSG/io_api.h>
#include <debug/dag_log.h>
#include <math/dag_mesh.h>
#include <EASTL/algorithm.h>
#include <string.h>

namespace dacsg
{
static_assert(CH_UV10 - CH_UV2 + 2 == NUMMESHTCH, "a uv channel id per tvert channel of the dag Mesh");

static uint32_t dag_corner(uint32_t k) { return (3 - k) % 3; }
static const auto &corner_ids(const TFace &f) { return f.t; }
static const auto &corner_ids(const FaceNGr &f) { return f.v; }

// tvert[i] of the dag Mesh
static uint32_t uv_channel_id(int i) { return i == 0 ? CH_UV : CH_UV2 + i - 1; }
static int uv_channel_index(uint32_t id) { return id == CH_UV ? 0 : id >= CH_UV2 && id <= CH_UV10 ? int(id - CH_UV2) + 1 : -1; }

struct SmoothRegions
{
  dag::Vector<uint32_t> faceRegion;
  uint32_t regionCount = 0;
};

// edges with equal corner normals join regions, a flat face (smgr 0) none; computed: also by the rule they came from
static void smooth_regions(const Mesh &m, dag::ConstSpan<Halfedge> halfedges, dag::ConstSpan<uint32_t> normalId, bool computed,
  SmoothRegions &r)
{
  const uint32_t faceCount = m.faceCount();
  UnionFind regions;
  regions.init(faceCount);
  for (uint32_t h = 0; h < halfedges.size(); ++h)
  {
    const uint32_t p = halfedges[h].pairedHalfedge;
    if (p == NO_INDEX || p < h)
      continue;
    const uint32_t f = h / 3, g = p / 3;
    if (normalId[h] != normalId[3 * g + (p % 3 + 1) % 3] || normalId[3 * f + (h % 3 + 1) % 3] != normalId[p])
      continue;
    if (!m.face[f].smgr || !m.face[g].smgr || (computed && (m.face[f].prim != m.face[g].prim || !(m.face[f].smgr & m.face[g].smgr))))
      continue;
    regions.joinLower(f, g);
  }
  r.regionCount = regions.compress(r.faceRegion);
}

void to_dag_mesh(const Mesh &m, ::Mesh &out)
{
  const uint32_t faceCount = m.faceCount(), cornerCount = faceCount * 3;
  dag::Vector<Halfedge> halfedges;
  pair_mesh_edges(m, halfedges);
  dag::Vector<float> normals;
  const Channel *normalChannel = m.findChannel(CH_NORMAL);
  const bool given = normalChannel && normalChannel->dim >= 3;
  if (!given)
  {
    dag::Vector<uint32_t> group(faceCount);
    for (uint32_t f = 0; f < faceCount; ++f)
      group[f] = m.face[f].prim;
    compute_output_normals(m, group, {}, normals);
  }
  dag::Vector<uint32_t> normalId;
  dag::Vector<float> normalValues;
  index_tuples(given ? normalChannel->data.data() : normals.data(), cornerCount, 3, given ? normalChannel->dim : 3, normalId,
    normalValues);

  SmoothRegions regions;
  smooth_regions(m, halfedges, normalId, !given, regions);
  const uint32_t regionCount = regions.regionCount;

  dag::Vector<uint32_t> regionStart, regionFaces;
  bucket_items(faceCount, regionCount, [&](uint32_t f) { return regions.faceRegion[f]; }, regionStart, regionFaces);

  // vertBits: per vertex, the bits of coloured regions; no vertex splits: a round trip stays closed
  dag::Vector<uint32_t> regionBits(regionCount, 0), vertBits(m.pos.size(), 0);
  for (uint32_t r = 0; r < regionCount; ++r)
  {
    if (regionStart[r + 1] - regionStart[r] < 2)
      continue;
    uint32_t used = 0, bit = 1;
    for (uint32_t i = regionStart[r]; i < regionStart[r + 1]; ++i)
      for (uint32_t k = 0; k < 3; ++k)
        used |= vertBits[m.tri[3 * regionFaces[i] + k]];
    while (used & bit)
      bit <<= 1;
    // no free bit: the least shared one
    if (!bit)
    {
      uint32_t shared[32] = {};
      for (uint32_t i = regionStart[r]; i < regionStart[r + 1]; ++i)
        for (uint32_t k = 0; k < 3; ++k)
          for (uint32_t b = 0; b < 32; ++b)
            shared[b] += vertBits[m.tri[3 * regionFaces[i] + k]] >> b & 1;
      bit = 1u << (eastl::min_element(shared, shared + 32) - shared);
    }
    regionBits[r] = bit;
    for (uint32_t i = regionStart[r]; i < regionStart[r + 1]; ++i)
      for (uint32_t k = 0; k < 3; ++k)
        vertBits[m.tri[3 * regionFaces[i] + k]] |= bit;
  }

  out.ntab.clear();
  out.ngr.clear();
  out.facenorm.clear();
  out.faceflg.clear();
  out.extra.clear();
  out.cvert.clear();
  out.cface.clear();
  for (int i = 0; i < NUMMESHTCH; ++i)
  {
    out.tvert[i].clear();
    out.tface[i].clear();
  }
  out.vert.resize(m.pos.size());
  for (uint32_t v = 0; v < m.pos.size(); ++v)
    out.vert[v] = m.pos[v];
  out.face.resize(faceCount);
  for (uint32_t f = 0; f < faceCount; ++f)
    out.face[f].set(m.tri[3 * f + dag_corner(0)], m.tri[3 * f + dag_corner(1)], m.tri[3 * f + dag_corner(2)], m.face[f].mat,
      regionBits[regions.faceRegion[f]]);

  dag::Vector<uint32_t> id;
  dag::Vector<float> values;
  for (const Channel &ch : m.channels)
  {
    const int uvIndex = uv_channel_index(ch.id);
    const bool isColor = ch.id == CH_COLOR;
    if (ch.id == CH_NORMAL)
      continue;
    if ((uvIndex < 0 && !isColor) || (uvIndex >= 0 && ch.dim < 2) || (isColor && ch.dim < 4))
    {
      const char *name = m.userChannelName(ch.id);
      logwarn("daCSG: to_dag_mesh: channel %u '%s' (dim %d) is dropped", ch.id, name ? name : "", ch.dim);
      continue;
    }
    index_tuples(ch.data.data(), cornerCount, ch.dim, ch.dim, id, values);
    const uint32_t count = values.size() / ch.dim;
    TFaceTab &faces = uvIndex < 0 ? out.cface : out.tface[uvIndex];
    faces.resize(faceCount);
    for (uint32_t f = 0; f < faceCount; ++f)
      for (uint32_t k = 0; k < 3; ++k)
        faces[f].t[k] = id[3 * f + dag_corner(k)];
    if (uvIndex < 0)
    {
      out.cvert.resize(count);
      for (uint32_t i = 0; i < count; ++i)
        memcpy(&out.cvert[i], &values[i * ch.dim], sizeof(out.cvert[i]));
    }
    else
    {
      out.tvert[uvIndex].resize(count);
      for (uint32_t i = 0; i < count; ++i)
        out.tvert[uvIndex][i] = Point2(values[i * ch.dim], values[i * ch.dim + 1]);
    }
  }

  out.vertnorm.resize(normalValues.size() / 3);
  for (uint32_t i = 0; i < out.vertnorm.size(); ++i)
    out.vertnorm[i] = Point3(&normalValues[3 * i], Point3::CTOR_FROM_PTR);
  out.facengr.resize(faceCount);
  for (uint32_t f = 0; f < faceCount; ++f)
    for (uint32_t k = 0; k < 3; ++k)
      out.facengr[f][k] = normalId[3 * f + dag_corner(k)];
}

bool from_dag_mesh(const ::Mesh &in, Mesh &m, eastl::string &err)
{
  m = Mesh();
  const uint32_t faceCount = in.face.size();
  m.pos.assign(in.vert.begin(), in.vert.end());
  m.tri.resize(faceCount * 3);
  m.face.resize(faceCount);
  for (uint32_t f = 0; f < faceCount; ++f)
  {
    for (uint32_t k = 0; k < 3; ++k)
    {
      const uint32_t v = in.face[f].v[dag_corner(k)];
      if (v >= m.pos.size())
      {
        err.sprintf("face %u has vertex index %u of %u", f, v, uint32_t(m.pos.size()));
        m = Mesh();
        return false;
      }
      m.tri[3 * f + k] = v;
    }
    m.face[f] = FaceInfo{0, f, in.face[f].smgr, in.face[f].mat, 0};
  }

  // a table the mesh did not supply, empty like a mesh with no faces, gives no channel: the defaults come after
  const auto corner_data = [&](const auto &faces, uint32_t valueCount, uint32_t id, uint8_t dim, auto value) {
    if (faces.size() != faceCount && !faces.empty())
      logwarn("daCSG: from_dag_mesh: channel %u has %u faces of %u, dropped", id, uint32_t(faces.size()), faceCount);
    if (faces.size() != faceCount || faces.empty())
      return;
    for (const auto &face : faces)
      for (uint32_t t : corner_ids(face))
        if (t >= valueCount)
        {
          logwarn("daCSG: from_dag_mesh: channel %u has an index out of range, dropped", id);
          return;
        }
    Channel &ch = m.channels.push_back(Channel{id, dim});
    ch.data.resize_noinit(faceCount * 3 * dim);
    for (uint32_t f = 0; f < faceCount; ++f)
      for (uint32_t k = 0; k < 3; ++k)
        value(corner_ids(faces[f])[dag_corner(k)], &ch.data[(3 * f + k) * dim]);
  };
  for (int i = 0; i < NUMMESHTCH; ++i)
    corner_data(in.tface[i], in.tvert[i].size(), uv_channel_id(i), 2, [&](uint32_t t, float *dst) {
      dst[0] = in.tvert[i][t].x;
      dst[1] = in.tvert[i][t].y;
    });
  corner_data(in.cface, in.cvert.size(), CH_COLOR, 4, [&](uint32_t t, float *dst) { memcpy(dst, &in.cvert[t], sizeof(in.cvert[t])); });
  corner_data(in.facengr, in.vertnorm.size(), CH_NORMAL, 3,
    [&](uint32_t t, float *dst) { memcpy(dst, &in.vertnorm[t], sizeof(in.vertnorm[t])); });
  m.addDefaultChannels();
  return true;
}
} // namespace dacsg
