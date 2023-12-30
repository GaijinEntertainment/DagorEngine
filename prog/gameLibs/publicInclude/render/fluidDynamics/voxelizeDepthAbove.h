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
  Voxelizer(float world_box_width, float meters_per_voxel_xz = 1.0f, int num_slices = 8, int num_cascades = 3);

  void prepareWorldBox(const Point3 &world_pos, const Point2 &world_y_min_max);
  void voxelizeDepthAbove();
  TEXTUREID getVoxelTexId() const;
  TEXTUREID getBoundaryTexId(int cascade) const;
  Point3 getVoxelTC(const Point3 &world_pos);

  Point3 getCenter() const;
  BBox3 getWorldBox() const;
  Point4 getWorldToVoxel() const;
  Point4 getWorldToVoxelHeight() const;

private:
  UniqueTexHolder voxelTex;
  int numCascades;
  Tab<UniqueTex> boundaryCascades;

  IPoint3 voxelTexSize;
  Point3 worldBoxSize;
  BBox3 worldBox;

  Point4 worldToVoxel;
  Point4 worldToVoxelHeight;

  float metersPerVoxelXZ;
  float metersPerVoxelY;

  eastl::unique_ptr<ComputeShaderElement> voxelizeCs;
  eastl::unique_ptr<ComputeShaderElement> generateBoundariesCs;
};
} // namespace cfd