#ifndef TOROIDAL_VOXEL_GRID_INC
#define TOROIDAL_VOXEL_GRID_INC

#define TOR_VGRID_WRAP_OFFSET_BITS 24u
#define TOR_VGRID_WRAP_OFFSET (1u << TOR_VGRID_WRAP_OFFSET_BITS)

#ifndef __cplusplus
int3 tor_vgrid_volindex_to_world_voxel(uint3 voxel_idx, int3 origin, uint2 tex_dim_half)
{
  return int3(voxel_idx.xzy) - int3(tex_dim_half.xyx) + origin;
}

uint3 tor_vgrid_world_voxel_to_tex_tci(int3 world_voxel, uint2 tex_dim)
{
  // uint3 v = (world_voxel % tex_dim.xyx + voxel_shadows_tex_dim.xyx) % voxel_shadows_tex_dim.xyx; // python-style modulo to wrap negative remainders
  uint3 v = uint3(world_voxel + TOR_VGRID_WRAP_OFFSET) % tex_dim.xyx; // cheaper than ^, since it unsigned
  return v.xzy;
}

float3 tor_vgrid_world_pos_to_tex_tc_linear(float3 world_pos, float2 world_to_tc)
{
  float3 v = world_pos.xyz * world_to_tc.xyx;
  return v.xzy;
}

float3 tor_vgrid_tc_linear_to_atlas(float3 tc, uint tex_z_dim, uint cascade_id, uint cascade_count)
{
  // TODO: precalc and move to vars
  float z = tex_z_dim;
  float z_padded = z + 2;
  float atlas_dim = z_padded * cascade_count;

  tc.z = frac(tc.z) * (z / atlas_dim) + cascade_id * (z_padded / atlas_dim) + 1.f / atlas_dim;

  return tc;
}

float3 tor_vgrid_world_ofs_to_tc_ofs_atlas(float3 world_ofs, float2 world_to_tc, uint tex_z_dim, uint cascade_count) // for tc offsets (dither)
{
  float3 v = world_ofs.xyz * world_to_tc.xyx;
  v = v.xzy;

  // TODO: precalc and move to vars
  float z = tex_z_dim;
  float z_padded = z + 2;
  float atlas_dim = z_padded * cascade_count;
  v.z *= tex_z_dim / atlas_dim;
  return v;
}

#endif // __cplusplus

#endif