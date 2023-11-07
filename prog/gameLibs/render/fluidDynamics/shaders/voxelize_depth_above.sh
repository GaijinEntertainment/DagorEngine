include "shader_global.sh"
include "depth_above.sh"

texture cfd_voxel_tex;
int4 cfd_voxel_tex_size;
float4 cfd_voxel_size;
float4 world_box_corner;

shader voxelize_depth_above
{
  ENABLE_ASSERT(cs)
  INIT_DEPTH_ABOVE(cs, blurred_depth)
  USE_DEPTH_ABOVE_TC(cs)

  (cs) {
    depth_above@smp2d = depth_around;
    voxel_tex@uav = cfd_voxel_tex hlsl {
      RWTexture3D<uint> voxel_tex@uav;
    };
    tex_size@i3 = (cfd_voxel_tex_size.x, cfd_voxel_tex_size.y, cfd_voxel_tex_size.z);
    voxel_size@f3 = cfd_voxel_size;
    world_box_corner@f3 = world_box_corner;
  }

  hlsl(cs) {
    float3 getVoxelCornerInWorld(uint3 id)
    {
      return world_box_corner + id.xzy * voxel_size;
    }
  }

  hlsl(cs) {
    [numthreads(8, 8, 1)]
    void cs_main(uint3 tid : SV_DispatchThreadID)
    {
      if (tid.x >= tex_size.x || tid.y >= tex_size.y || tid.z >= tex_size.z)
        return;

      float3 voxelCorner = getVoxelCornerInWorld(tid);
      float3 voxelCenter = voxelCorner + voxel_size * 0.5f;
      float2 depthAboveTc = saturate(world_to_depth_ao.xy * voxelCenter.xz + world_to_depth_ao.zw) - depth_ao_heights.zw;

      float height = decode_depth_above(tex2Dlod(depth_above, float4(depthAboveTc,0,0) ).x);


      if (height > voxelCenter.y)
        voxel_tex[tid] = 1u;
      else
        voxel_tex[tid] = 0u;
    }
  }
  compile("cs_5_0", "cs_main")
}