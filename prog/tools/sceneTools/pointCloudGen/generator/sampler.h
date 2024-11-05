// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "spatialGrid.h"
#include <EASTL/fixed_vector.h>
#include <shaders/dag_shaderCommon.h>

namespace plod
{

struct Polygon
{
  eastl::array<RawVertex, 3> v;
};

struct MaterialChannel
{
  float4 val;
  uint8_t mod = ChannelModifier::CMOD_NONE;
};

inline const MaterialChannel INVALID_MATERIAL_CHANNEl = {{FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX}};

using Material = eastl::fixed_vector<MaterialChannel, 8>;

struct ProcessedVertex
{
  Point3 position{};
  Point3 norm{};
  Material material{};
};

void samplePointsOnPolygon(SpatialGrid &samples, const Polygon &poly, size_t count);
float getPoissonRadius(float density);
float getPolygonSurfaceArea(const Polygon &poly);
void perturbNormals(dag::ConstSpan<RawVertex> raw_cloud, dag::Span<ProcessedVertex> processed_cloud);
void addTangents(dag::ConstSpan<RawVertex> raw_cloud, dag::Span<ProcessedVertex> processed_cloud);

} // namespace plod
