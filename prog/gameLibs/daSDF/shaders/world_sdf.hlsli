#ifndef WORLD_SDF_HLSLI
#define WORLD_SDF_HLSLI 1

#define HAS_OBJECT_SDF 0
#define MAX_WORLD_SDF_VOXELS_BAND 4 // max step and also quantization quality
#define MAX_WORLD_SDF_CLIPS 8
#define WORLD_SDF_ALLOW_NON_POW2 1
#define SDF_MAX_CULLED_INSTANCES 32767 // count of instances
#define SDF_CULLED_INSTANCES_GRID_SIZE 262144 // in dwords
#define SDF_INSTANCES_CPU_CULLING 1

#define SDF_UPDATE_WARP_X 8
#define SDF_UPDATE_WARP_Y 8
#define SDF_UPDATE_WARP_Z 4
#define SDF_UPDATE_WARP_RES uint3(SDF_UPDATE_WARP_X,SDF_UPDATE_WARP_Y,SDF_UPDATE_WARP_Z)
#define SDF_GRID_CELL_SIZE_IN_TILES 4 // culled instances will be placed in grid of N texels

#if HAS_OBJECT_SDF
#define WORLD_SDF_TEXELS_ALIGNMENT_XZ SDF_UPDATE_WARP_X  // all xz coordinates should be in multiples of alignment
#define WORLD_SDF_TEXELS_ALIGNMENT_ALT SDF_UPDATE_WARP_Z // all coordinates should be in multiples of alignment
#else
#define WORLD_SDF_TEXELS_ALIGNMENT_XZ  2 // it is 2, not 1, so voxels lit scene is updated (normally) once per voxel
#define WORLD_SDF_TEXELS_ALIGNMENT_ALT WORLD_SDF_TEXELS_ALIGNMENT_XZ
#endif

#define WORLD_SDF_RASTERIZE_VOXELS_DIST 1

#define CBUFF_STRUCTURE_world_sdf_params \
float4 world_sdf_lt[MAX_WORLD_SDF_CLIPS + 1]; \
float4 world_sdf_to_tc_add[MAX_WORLD_SDF_CLIPS + 1]; // One extra for invalid elem

#endif