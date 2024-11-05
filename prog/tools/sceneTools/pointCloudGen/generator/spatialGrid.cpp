// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "spatialGrid.h"
#include <math/dag_approachWindowed.h>
#include <math/integer/dag_IPoint3.h>

namespace plod
{

SpatialGrid::SpatialGrid(BBox3 bounding_box, float cell_size) : cellSize(cell_size), boundingBox(bounding_box)
{
  const auto width = bounding_box.width();
  const auto getDim = [&](float size) { return eastl::max(int(size / cell_size), 1); };
  spaceDim = {getDim(width.x), getDim(width.y), getDim(width.z)};
  space.resize(spaceDim.x * spaceDim.y * spaceDim.z);
  G_ASSERT(spaceDim.x > 0);
  G_ASSERT(spaceDim.y > 0);
  G_ASSERT(spaceDim.z > 0);
}

size_t SpatialGrid::getIdxForCoord(IPoint3 coord) const
{
  return (spaceDim.y * spaceDim.z) * coord.x + spaceDim.z * coord.y + coord.z;
}

SpatialCell &SpatialGrid::getCell(Point3 pos)
{
  auto cell = (pos - boundingBox.boxMin()) / cellSize;
  const IPoint3 cellCoord = {
    clamp((int)cell.x, 0, (spaceDim.x - 1)), clamp((int)cell.y, 0, spaceDim.y - 1), clamp((int)cell.z, 0, spaceDim.z - 1)};
  return space[getIdxForCoord(cellCoord)];
}

SampleId SpatialGrid::addVertex(RawVertex vertex)
{
  pool.push_back(vertex);
  const auto sampleId = static_cast<SampleId>(pool.size() - 1);
  auto &cell = getCell(vertex.position);
  cell.push_back(sampleId);
  samples.push_back(sampleId);
  lastValidSample = pool.size();
  return sampleId;
}

SampleId SpatialGrid::popSample()
{
  G_ASSERT_RETURN(lastValidSample <= samples.size(), SampleId::Invalid);
  while (lastValidSample > 0)
    if (samples[--lastValidSample] != SampleId::Invalid)
      return samples[lastValidSample];
  return SampleId::Invalid;
}

RawVertex SpatialGrid::getVertex(SampleId sample_id) const
{
  size_t poolIdx = eastl::to_underlying(sample_id);
  G_ASSERT(poolIdx < pool.size());
  return pool[poolIdx];
}

float SpatialGrid::distance(Point3 pos1, Point3 pos2) const { return distance_between(pos1, pos2); }

void SpatialGrid::removeSamplesInPoissonRadius(SampleId sample_id, float radius)
{
  G_ASSERT(eastl::to_underlying(sample_id) < samples.size());
  const auto vertex = getVertex(sample_id);
  const int cellRadius = radius / cellSize + 1;
  const int cellX = eastl::min(cellRadius, spaceDim.x);
  const int cellY = eastl::min(cellRadius, spaceDim.y);
  const int cellZ = eastl::min(cellRadius, spaceDim.z);
  for (int dx = -cellX; dx <= cellX; ++dx)
    for (int dy = -cellY; dy <= cellY; ++dy)
      for (int dz = -cellZ; dz <= cellZ; ++dz)
      {
        const auto cellPos = vertex.position + Point3(dx, dy, dz) * cellSize;
        const auto &cell = getCell(cellPos);
        for (auto neighbourId : cell)
        {
          const auto neighbourIntId = eastl::to_underlying(neighbourId);
          G_ASSERT(neighbourIntId < samples.size());
          const auto neighbour = getVertex(neighbourId);
          if (samples[neighbourIntId] != SampleId::Invalid && distance(vertex.position, neighbour.position) < radius &&
              dot(neighbour.tbn.getcol(2), vertex.tbn.getcol(2)) > 0)
            samples[neighbourIntId] = SampleId::Invalid;
        }
      }
}

void SpatialGrid::restoreSamplePool()
{
  for (size_t i = 0; i < pool.size(); ++i)
  {
    const auto sampleId = static_cast<SampleId>(i);
    samples[i] = sampleId;
  }
  lastValidSample = samples.size();
}

} // namespace plod
