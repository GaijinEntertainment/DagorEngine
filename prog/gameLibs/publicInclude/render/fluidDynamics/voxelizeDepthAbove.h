#pragma once

#include <shaders/dag_computeShaders.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint3.h>

namespace cfd
{
class Voxelizer
{
public:
  Voxelizer(IPoint3 vol_tex_size);

  void voxelizeDepthAbove(const Point3 &world_pos, const Point2 &worldYMinMax);

private:
  UniqueTexHolder voxelTex;
  IPoint3 volTexSize;
  Point3 worldBoxSize;
  static constexpr float metersPerVoxel = 1.0f;

  eastl::unique_ptr<ComputeShaderElement> voxelizeCs;
};
} // namespace cfd