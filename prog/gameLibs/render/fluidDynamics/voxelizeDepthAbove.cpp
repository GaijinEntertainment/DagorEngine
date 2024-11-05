// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/fluidDynamics/voxelizeDepthAbove.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_bounds3.h>
#include <math/dag_bounds2.h>

namespace cfd
{
#define VARS_LIST                   \
  VAR(cfd_voxel_tex_size)           \
  VAR(cfd_voxel_size)               \
  VAR(cfd_world_box_min)            \
  VAR(cfd_world_to_voxel)           \
  VAR(cfd_world_to_voxel_height)    \
  VAR(cfd_prev_boundaries)          \
  VAR(cfd_next_boundaries)          \
  VAR(cfd_next_boundaries_tex_size) \
  VAR(cfd_calc_offset)

#define VAR(a) static int a##VarId = -1;
VARS_LIST
#undef VAR

static float default_box_height = 60.f;
static float default_meters_per_voxel_y = 1.0f;

Voxelizer::Voxelizer(float world_box_width, float meters_per_voxel_xz, int num_slices, int num_cascades) :
  voxelTexSize(IPoint3(world_box_width / meters_per_voxel_xz, world_box_width / meters_per_voxel_xz, num_slices)),
  worldBoxSize(Point3(world_box_width, default_box_height, world_box_width)),
  metersPerVoxelXZ(meters_per_voxel_xz),
  metersPerVoxelY(default_meters_per_voxel_y),
  numCascades(num_cascades)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  VARS_LIST
#undef VAR

  voxelizeCs.reset(new_compute_shader("voxelize_depth_above"));
  generateBoundariesCs.reset(new_compute_shader("cfd_generate_boundaries"));
  voxelTex = dag::create_voltex(voxelTexSize.x, voxelTexSize.y, voxelTexSize.z, TEXFMT_L8 | TEXCF_UNORDERED, 1, "cfd_voxel_tex");
  voxelTex->texfilter(TEXFILTER_POINT);
  voxelTex->texaddr(TEXADDR_CLAMP);

  boundaryCascades.resize(numCascades);
  for (int i = 0; i < numCascades; ++i)
  {
    int width = voxelTexSize.x >> i;
    int height = voxelTexSize.y >> i;
    boundaryCascades[i] =
      dag::create_voltex(width, height, voxelTexSize.z, TEXFMT_L8 | TEXCF_UNORDERED, 1, String(0, "cfd_boundary_tex_%d", i));
    boundaryCascades[i]->texfilter(TEXFILTER_POINT);
    boundaryCascades[i]->texaddr(TEXADDR_CLAMP);
  }

  ShaderGlobal::set_int4(cfd_voxel_tex_sizeVarId, IPoint4(voxelTexSize.x, voxelTexSize.y, voxelTexSize.z, 0));

  Point4 dummyWorldToVoxel = Point4(1.0f / 1000.f, 1.0f / 1000.f, -100000.f, -100000.f);
  ShaderGlobal::set_color4(cfd_world_to_voxelVarId, dummyWorldToVoxel);
}

void Voxelizer::prepareWorldBox(const Point3 &world_pos, const Point2 &world_y_min_max)
{
  metersPerVoxelY = (world_y_min_max.y - world_y_min_max.x) / voxelTexSize.z;
  worldBoxSize.y = world_y_min_max.y - world_y_min_max.x;
  ShaderGlobal::set_color4(cfd_voxel_sizeVarId, metersPerVoxelXZ, metersPerVoxelY, metersPerVoxelXZ, 0);

  const Point2 worldCenterXZ = Point2::xz(world_pos);
  Point2 worldMinXZ = worldCenterXZ - Point2::xz(worldBoxSize) / 2.f;
  Point3 worldMin = Point3::xVy(worldMinXZ, world_y_min_max.x);
  worldBox = BBox3(worldMin, worldMin + worldBoxSize);

  worldToVoxel = Point4(1.0f / worldBoxSize.x, 1.0f / worldBoxSize.z, worldMinXZ.x, worldMinXZ.y);
  worldToVoxelHeight = Point4(1.0f / worldBoxSize.y, world_y_min_max.x, 0, 0);

  ShaderGlobal::set_color4(cfd_world_box_minVarId, Point4::xyz0(worldMin));
}

void Voxelizer::voxelizeDepthAbove(const BBox2 &area)
{
  ShaderGlobal::set_int4(cfd_calc_offsetVarId, IPoint4(area.getMin().x * voxelTexSize.x, area.getMin().y * voxelTexSize.y, 0, 0));
  Point2 areaSize = area.getMax() - area.getMin();
  IPoint2 numDispatches = IPoint2(ceil(voxelTexSize.x * areaSize.x), ceil(voxelTexSize.y * areaSize.y));

  voxelizeCs->dispatchThreads(numDispatches.x, numDispatches.y, voxelTexSize.z);
  d3d::resource_barrier({voxelTex.getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});

  for (int i = 0; i < numCascades; ++i)
  {
    int width = voxelTexSize.x >> i;
    int height = voxelTexSize.y >> i;
    ShaderGlobal::set_int4(cfd_next_boundaries_tex_sizeVarId, IPoint4(width, height, voxelTexSize.z, 0));
    if (i == 0)
      ShaderGlobal::set_texture(cfd_prev_boundariesVarId, voxelTex.getTexId());
    else
      ShaderGlobal::set_texture(cfd_prev_boundariesVarId, boundaryCascades[i - 1].getTexId());
    ShaderGlobal::set_texture(cfd_next_boundariesVarId, boundaryCascades[i].getTexId());

    ShaderGlobal::set_int4(cfd_calc_offsetVarId, IPoint4(area.getMin().x * width, area.getMin().y * height, 0, 0));
    numDispatches = IPoint2(ceil(width * areaSize.x), ceil(height * areaSize.y));

    generateBoundariesCs->dispatchThreads(numDispatches.x, numDispatches.y, voxelTexSize.z);
    d3d::resource_barrier({boundaryCascades[i].getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});
  }
}

void Voxelizer::setTransformVars()
{
  ShaderGlobal::set_color4(cfd_world_to_voxelVarId, worldToVoxel);
  ShaderGlobal::set_color4(cfd_world_to_voxel_heightVarId, worldToVoxelHeight);
}

TEXTUREID Voxelizer::getVoxelTexId() const { return voxelTex.getTexId(); }
TEXTUREID Voxelizer::getBoundaryTexId(int cascade) const { return boundaryCascades[cascade].getTexId(); }
Point4 Voxelizer::getWorldToVoxel() const { return worldToVoxel; }
Point4 Voxelizer::getWorldToVoxelHeight() const { return worldToVoxelHeight; };

Point3 Voxelizer::getCenter() const { return worldBox.center(); }
BBox3 Voxelizer::getWorldBox() const { return worldBox; }

Point3 Voxelizer::getVoxelTC(const Point3 &world_pos)
{
  Point3 tc;
  tc.x = clamp((world_pos.x - worldToVoxel.z) * worldToVoxel.x, 0.f, 1.f);
  tc.y = clamp((world_pos.z - worldToVoxel.w) * worldToVoxel.y, 0.f, 1.f);
  tc.z = clamp((world_pos.y - worldToVoxelHeight.y) * worldToVoxelHeight.x, 0.f, 1.f);
  return tc;
}
} // namespace cfd
