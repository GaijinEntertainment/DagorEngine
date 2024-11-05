// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sampler.h"
#include "packUtil.h"
#include <EASTL/fixed_set.h>
#include <math/dag_tangentSpace.h>
#include <math/random/dag_random.h>

namespace plod
{

template <typename T>
static T interpolate_value(T v1, T v2, T v3, float u, float v, float w)
{
  return v1 * u + v2 * v + v3 * w;
}

void samplePointsOnPolygon(SpatialGrid &samples, const Polygon &poly, size_t count)
{
  eastl::fixed_set<eastl::pair<float, float>, 4096> uniqueUVs{};
  float u, v;
  for (size_t i = 0; i < count; ++i)
  {
    do
    {
      u = dagor_random::rnd_float(0.0, 1.0);
      v = dagor_random::rnd_float(0.0, 1.0);
      if (u + v > 1)
      {
        u = 1 - u;
        v = 1 - v;
      }
    } while (uniqueUVs.find({u, v}) != uniqueUVs.end());
    uniqueUVs.insert({u, v});
    const auto w = 1 - u - v;
    const auto p = interpolate_value(poly.v[0].position, poly.v[1].position, poly.v[2].position, u, v, w);

    RawVertex sample{};
    sample.position = p;
    sample.tbn.col[2] =
      normalize(interpolate_value(poly.v[0].tbn.getcol(2), poly.v[1].tbn.getcol(2), poly.v[2].tbn.getcol(2), u, v, w));
    for (size_t i = 0; i < sample.tc.size(); ++i)
      sample.tc[i] = interpolate_value(poly.v[0].tc[i], poly.v[1].tc[i], poly.v[2].tc[i], u, v, w);

    calculate_dudv(poly.v[0].position, poly.v[1].position, poly.v[2].position, poly.v[0].tc[0], poly.v[1].tc[0], poly.v[2].tc[0],
      sample.tbn.col[0], sample.tbn.col[1]);

    sample.color = packToE3DCOLOR(
      interpolate_value(unpackToPoint3(poly.v[0].color), unpackToPoint3(poly.v[1].color), unpackToPoint3(poly.v[2].color), u, v, w));
    samples.addVertex(sample);
  }
}

float getPolygonSurfaceArea(const Polygon &poly)
{
  const auto p1 = poly.v[0].position;
  const auto p2 = poly.v[1].position;
  const auto p3 = poly.v[2].position;

  const auto p1p2 = p2 - p1;
  const auto p3p1 = p3 - p1;

  return 0.5 * length(cross(p1p2, p3p1));
}

float getPoissonRadius(float density) { return std::sqrt(3.1415 / density); }

static constexpr int normal_map_channel_idx = 1;

static Point3 toPoint3(const Point4 v) { return {v.x, v.y, v.z}; }

void perturbNormals(dag::ConstSpan<RawVertex> raw_cloud, dag::Span<ProcessedVertex> processed_cloud)
{
  G_ASSERT_RETURN(raw_cloud.size() == processed_cloud.size(), );
  for (size_t i = 0; i < raw_cloud.size(); ++i)
  {
    const auto normalMap = toPoint3(processed_cloud[i].material[normal_map_channel_idx].val);
    const auto material = processed_cloud[i].material[normal_map_channel_idx].val.w;
    processed_cloud[i].material[normal_map_channel_idx].val = {raw_cloud[i].tbn * normalMap * 0.5f + 0.5f, material};
    processed_cloud[i].material[normal_map_channel_idx].mod = ChannelModifier::CMOD_NONE;
  }
}

void addTangents(dag::ConstSpan<RawVertex> raw_cloud, dag::Span<ProcessedVertex> processed_cloud)
{
  G_ASSERT_RETURN(raw_cloud.size() == processed_cloud.size(), );
  for (size_t i = 0; i < raw_cloud.size(); ++i)
    processed_cloud[i].material.push_back({{raw_cloud[i].tbn.getcol(0), 0.0f}, ChannelModifier::CMOD_UNSIGNED_PACK});
}

} // namespace plod
