#include <render/fluidDynamics/voxelizeDepthAbove.h>

namespace cfd
{
Voxelizer::Voxelizer(IPoint3 vol_tex_size) : volTexSize(vol_tex_size), worldBoxSize(vol_tex_size.x, vol_tex_size.z, vol_tex_size.y)
{
  voxelizeCs.reset(new_compute_shader("voxelize_depth_above"));
  voxelTex = dag::create_voltex(volTexSize.x, volTexSize.y, volTexSize.z, TEXFMT_R8UI | TEXCF_UNORDERED, 1, "cfd_voxel_tex");

  ShaderGlobal::set_int4(get_shader_variable_id("cfd_voxel_tex_size"), IPoint4(volTexSize.x, volTexSize.y, volTexSize.z, 0));
  ShaderGlobal::set_color4(get_shader_variable_id("cfd_voxel_size"), metersPerVoxel, metersPerVoxel * 4, metersPerVoxel, 0);
}

void Voxelizer::voxelizeDepthAbove(const Point3 &world_pos, const Point2 &world_y_min_max)
{
  const Point2 worldCenterXZ = Point2::xz(world_pos);
  Point2 worldMinXZ = worldCenterXZ - Point2::xz(worldBoxSize) / 2.f;

  Point3 worldMin = Point3::xVy(worldMinXZ, world_y_min_max.x);

  ShaderGlobal::set_color4(get_shader_variable_id("world_box_corner"), Point4::xyz0(worldMin));

  voxelizeCs->dispatchThreads(volTexSize.x, volTexSize.y, volTexSize.z);
}
} // namespace cfd
