#ifndef __DAGI_COMMON_TYPES__
#define __DAGI_COMMON_TYPES__

#include "dagi_envi_cube_consts.hlsli"
#include "daGI/dagi_volmap_cb.hlsli"
#include "poisson_samples.hlsli"

struct VisibleAmbientVoxelPoint
{
  uint voxelId;
};

#define MAX_TRACE_DIST 35.f

#define CULL_ALL_CASCADE 0
#define MAX_SELECTED_INTERSECTED_VOXELS (512)
#define MAX_SELECTED_NON_INTERSECTED_VOXELS (128)
#define MAX_SELECTED_INITIAL_VOXELS (4096)
#define MAX_BINS 2//MAX_BINS*2 will be filled with bin count. (before/after random split)
#define FULL_VOXELS_INDEX_BEFORE_TEMPORAL (MAX_BINS*2)
#define INITIALIZE_VOXELS_INDEX (MAX_BINS*2+1)
#define FULL_VOXELS_INDEX_AFTER_TEMPORAL (MAX_BINS*2+2)
#define HISTOGRAM_SIZE (FULL_VOXELS_INDEX_AFTER_TEMPORAL+1)

//SAMPLE_NUM can be part of quality settings
#define NUM_RAYS_PER_VOXEL 64
#define NUM_RAY_GROUPS (SAMPLE_NUM/NUM_RAYS_PER_VOXEL)

#define SELECT_WARP_SIZE 64
#define OCCLUSION_WARP_SIZE 64
#define POINT_OCCLUSION_WARP_SIZE 64
#define TEMPORAL_AMBIENT_VOXELS_WARP 16
#define INITIAL_VOXELS_RAYS 256//with 256 it is much better quality. we can also try 192 rays
#define COMBINE_WARP_SIZE NUM_RAY_GROUPS
#define INITIAL_WALLS_DIST_WARP 32

#define THREAD_PER_RAY NUM_RAYS_PER_VOXEL//all rays are in different threads, each group - is voxel
#define LIGHT_WARP_SIZE THREAD_PER_RAY

#define ENCODE_TEMP_COLOR_111110 0
#define ENCODE_TEMP_COLOR_16 1
#define ENCODE_VOXEL_COLORS ENCODE_TEMP_COLOR_16
struct VisibleAmbientVoxel
{
  uint voxelId;
  uint col0;
  uint col1;
  uint col2;
  uint col3;
  uint col4;
  uint col5;
  uint padding;
};

struct TraceResultAmbientVoxel
{
  uint3 col0;
  uint pad0;
  uint3 col1;
  uint pad1;
  uint3 col2;
  uint pad2;
};

struct AmbientVoxelsPlanes//best planes that affect voxel
{
  float4 planes[3];//this is not plane, but float4(plane.xyz, -1/dist-to-plane). for optimizations
};

#ifndef __cplusplus
#define reset_voxel(v) {v.padding  = 0;}

#define VOLMAP_RES_SHFT 8
#define VOLMAP_RES_SHFT_Z 7
#define MAX_VOLMAP_RES (1U<<VOLMAP_RES_SHFT)
#define VOLMAP_RES_MASK (MAX_VOLMAP_RES-1)
#define VOLMAP_RES_MASK_Z ((1U<<VOLMAP_RES_SHFT_Z)-1)
#define VOLMAP_WEIGHT_SHIFT (uint(VOLMAP_RES_SHFT*3))
#define VOLMAP_BIN_BITS (32-VOLMAP_WEIGHT_SHIFT)
#define VOLMAP_BIN_MAX_VAL ((1U<<VOLMAP_BIN_BITS)-1)
#define VOLMAP_INDEX_MASK ((1U<<VOLMAP_WEIGHT_SHIFT)-1)

uint encode_voxel_coord(uint3 coord)
{
  return coord.x|(coord.y<<VOLMAP_RES_SHFT)|(coord.z<<(VOLMAP_RES_SHFT*2));
}
uint encode_voxel_coord_bin(uint3 coord, uint bin)
{
  return coord.x|(coord.y<<VOLMAP_RES_SHFT)|(coord.z<<(VOLMAP_RES_SHFT*2))| (bin<<VOLMAP_WEIGHT_SHIFT);
}
uint3 decode_voxel_coord_safe(uint voxel, out uint bin)
{
  bin = voxel>>VOLMAP_WEIGHT_SHIFT;
  return uint3(voxel&VOLMAP_RES_MASK, (voxel>>VOLMAP_RES_SHFT)&VOLMAP_RES_MASK, (voxel>>(VOLMAP_RES_SHFT*2))&VOLMAP_RES_MASK_Z);
}
uint3 decode_voxel_coord_unsafe(uint voxel)
{
  return uint3(voxel&VOLMAP_RES_MASK, (voxel>>VOLMAP_RES_SHFT)&VOLMAP_RES_MASK, (voxel>>(VOLMAP_RES_SHFT*2)));
}

#include <pixelPacking/PixelPacking_R11G11B10.hlsl>

uint3 encode_two_colors16(float3 c0, float3 c1) {return uint3(f32tof16(c0)|(f32tof16(c1)<<16));}
void decode_two_colors16(uint3 c, out float3 c0, out float3 c1)
{
  c0 = f16tof32(c);c1 = f16tof32(c>>16);
}
uint encode_color(float3 col) {return Pack_R11G11B10_FIXED_LOG(col);}//Pack_R11G11B10_FLOAT//Pack_R11G11B10_FLOAT_LOG//Pack_R11G11B10_FIXED_LOG
float3 decode_color(uint col) {return Unpack_R11G11B10_FIXED_LOG(col);}//Unpack_R11G11B10_FLOAT//Unpack_R11G11B10_FLOAT_LOG//

void encode_voxel_colors(inout VisibleAmbientVoxel voxel, float3 c0, float3 c1, float3 c2, float3 c3, float3 c4, float3 c5)
{
    voxel.col0 = encode_color(c0);
    voxel.col1 = encode_color(c1);
    voxel.col2 = encode_color(c2);
    voxel.col3 = encode_color(c3);
    voxel.col4 = encode_color(c4);
    voxel.col5 = encode_color(c5);
}

void decode_voxel_colors(VisibleAmbientVoxel voxel, out float3 c0, out float3 c1, out float3 c2, out float3 c3, out float3 c4, out float3 c5)
{
    c0 = decode_color(voxel.col0);
    c1 = decode_color(voxel.col1);
    c2 = decode_color(voxel.col2);
    c3 = decode_color(voxel.col3);
    c4 = decode_color(voxel.col4);
    c5 = decode_color(voxel.col5);
}

void encode_trace_result_colors(inout TraceResultAmbientVoxel voxel, float3 c0, float3 c1, float3 c2, float3 c3, float3 c4, float3 c5)
{
  voxel.col0 = encode_two_colors16(c0, c1);
  voxel.col1 = encode_two_colors16(c2, c3);
  voxel.col2 = encode_two_colors16(c4, c5);
}

void decode_trace_result_colors(TraceResultAmbientVoxel voxel, out float3 c0, out float3 c1, out float3 c2, out float3 c3, out float3 c4, out float3 c5)
{
  decode_two_colors16(voxel.col0, c0, c1);
  decode_two_colors16(voxel.col1, c2, c3);
  decode_two_colors16(voxel.col2, c4, c5);
}

uint encode_voxel_world_posOfs(float3 ofs)
{
  uint3 uofs = clamp((ofs*0.5+0.25)*1023+0.5, 0, 1023);//-0.5,1.5 range encode - with trilinear filtering
  //uint3 uofs = clamp(ofs*1023, 0, 1023);
  return uofs.x|(uofs.y<<10)|(uofs.z<<20);
}
float3 decode_voxel_world_posOfs(uint ofs)
{
  return float3(ofs&1023, (ofs>>10)&1023, ofs>>20)*(2/1023.0)-0.5;//-0.5,1.5 range encode - with trilinear filtering
  //return float3(ofs&1023, (ofs>>10)&1023, ofs>>20)/1023.0;
}
#endif
#endif
