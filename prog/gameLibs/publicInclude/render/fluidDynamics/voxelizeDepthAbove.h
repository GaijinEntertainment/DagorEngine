#pragma once

#include <shaders/dag_computeShaders.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_resPtr.h>
#include <math/integer/dag_IPoint3.h>
#include <math/dag_bounds3.h>

namespace cfd
{
class Voxelizer
{
public:
  Voxelizer(Point3 world_box_size, float meters_per_voxel_xz = 1.0f, float meters_per_voxel_y = 4.0f);

  void voxelizeDepthAbove(const Point3 &world_pos, float world_y_min);
  TEXTUREID getVoxelTexId() const;
  Point3 getVoxelTC(const Point3 &world_pos);

  Point3 getCenter() const;
  BBox3 getWorldBox() const;
  Point4 getWorldToVoxel() const;
  Point4 getWorldToVoxelHeight() const;

private:
  UniqueTexHolder voxelTex;
  IPoint3 voxelTexSize;
  Point3 worldBoxSize;
  BBox3 worldBox;

  Point4 worldToVoxel;
  Point4 worldToVoxelHeight;

  float metersPerVoxelXZ;
  float metersPerVoxelY;

  eastl::unique_ptr<ComputeShaderElement> voxelizeCs;
};
} // namespace cfd