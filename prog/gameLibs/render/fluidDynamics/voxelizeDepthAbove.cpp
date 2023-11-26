#include <render/fluidDynamics/voxelizeDepthAbove.h>
#include <math/dag_bounds3.h>

namespace cfd
{
#define VARS_LIST         \
  VAR(cfd_voxel_tex_size) \
  VAR(cfd_voxel_size)     \
  VAR(cfd_world_box_min)  \
  VAR(cfd_world_to_voxel) \
  VAR(cfd_world_to_voxel_height)

#define VAR(a) static int a##VarId = -1;
VARS_LIST
#undef VAR

Voxelizer::Voxelizer(Point3 world_box_size, float meters_per_voxel_xz, float meters_per_voxel_y) :
  voxelTexSize(
    IPoint3(world_box_size.x / meters_per_voxel_xz, world_box_size.z / meters_per_voxel_xz, world_box_size.y / meters_per_voxel_y)),
  worldBoxSize(world_box_size),
  metersPerVoxelXZ(meters_per_voxel_xz),
  metersPerVoxelY(meters_per_voxel_y)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a, true);
  VARS_LIST
#undef VAR

  voxelizeCs.reset(new_compute_shader("voxelize_depth_above"));
  voxelTex = dag::create_voltex(voxelTexSize.x, voxelTexSize.y, voxelTexSize.z, TEXFMT_L8 | TEXCF_UNORDERED, 1, "cfd_voxel_tex");
  voxelTex->texfilter(TEXFILTER_POINT);
  voxelTex->texaddr(TEXADDR_CLAMP);

  ShaderGlobal::set_int4(cfd_voxel_tex_sizeVarId, IPoint4(voxelTexSize.x, voxelTexSize.y, voxelTexSize.z, 0));
  ShaderGlobal::set_color4(cfd_voxel_sizeVarId, metersPerVoxelXZ, metersPerVoxelY, metersPerVoxelXZ, 0);
}

void Voxelizer::voxelizeDepthAbove(const Point3 &world_pos, float world_y_min)
{
  const Point2 worldCenterXZ = Point2::xz(world_pos);
  Point2 worldMinXZ = worldCenterXZ - Point2::xz(worldBoxSize) / 2.f;
  Point3 worldMin = Point3::xVy(worldMinXZ, world_y_min);
  worldBox = BBox3(worldMin, worldMin + worldBoxSize);

  worldToVoxel = Point4(1.0f / worldBoxSize.x, 1.0f / worldBoxSize.z, worldMinXZ.x, worldMinXZ.y);
  worldToVoxelHeight = Point4(1.0f / worldBoxSize.y, world_y_min, 0, 0);

  ShaderGlobal::set_color4(cfd_world_box_minVarId, Point4::xyz0(worldMin));
  ShaderGlobal::set_color4(cfd_world_to_voxelVarId, worldToVoxel);
  ShaderGlobal::set_color4(cfd_world_to_voxel_heightVarId, worldToVoxelHeight);

  voxelizeCs->dispatchThreads(voxelTexSize.x, voxelTexSize.y, voxelTexSize.z);
  d3d::resource_barrier({voxelTex.getBaseTex(), RB_STAGE_COMPUTE | RB_RO_SRV, 0, 0});
}

TEXTUREID Voxelizer::getVoxelTexId() const { return voxelTex.getTexId(); }
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
