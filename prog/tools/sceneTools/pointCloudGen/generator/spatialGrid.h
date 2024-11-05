// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_tab.h>
#include <EASTL/array.h>
#include <EASTL/span.h>
#include <EASTL/string.h>
#include <math/dag_e3dColor.h>
#include <math/dag_hlsl_floatx.h>
#include <math/integer/dag_IPoint3.h>

namespace plod
{

struct RawVertex
{
  Point3 position{};
  Matrix3 tbn{};
  E3DCOLOR color{};
  eastl::array<Point2, 3> tc{};
};

enum class SampleId : uint32_t
{
  Invalid = ~(uint32_t)0u
};

using SpatialCell = dag::Vector<SampleId>;

class SpatialGrid
{
public:
  SpatialGrid(BBox3 bounding_box, float cell_size);
  SpatialGrid(const SpatialGrid &) = delete;
  SpatialGrid(SpatialGrid &&) = default;
  SpatialGrid &operator=(const SpatialGrid &) = delete;
  SpatialGrid &operator=(SpatialGrid &&) = default;
  virtual ~SpatialGrid() = default;

  SampleId addVertex(RawVertex vertex);
  RawVertex getVertex(SampleId sample_id) const;
  SampleId popSample();
  void restoreSamplePool();
  void removeSamplesInPoissonRadius(SampleId sample_id, float radius);

private:
  float distance(Point3 pos1, Point3 pos2) const;
  size_t getIdxForCoord(IPoint3 coord) const;
  SpatialCell &getCell(Point3 pos);
  std::size_t lastValidSample = 0;
  Tab<SampleId> samples;
  Tab<RawVertex> pool;
  BBox3 boundingBox = {};
  float cellSize = 0;
  IPoint3 spaceDim;
  Tab<SpatialCell> space;
};

} // namespace plod
