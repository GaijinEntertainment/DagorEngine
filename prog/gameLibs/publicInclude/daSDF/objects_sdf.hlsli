#ifndef OBJECTS_SDF_HLSLI
#define OBJECTS_SDF_HLSLI 1

// One voxel border around object for gradient
#define SDF_VOLUME_BORDER 1
#define SDF_INTERNAL_BLOCK_SIZE 7
//SDF_INTERNAL_BLOCK_SIZE + 0.5 texels around
#define SDF_BLOCK_SIZE 8
#define SDF_BLOCK_VOLUME (SDF_BLOCK_SIZE*SDF_BLOCK_SIZE*SDF_BLOCK_SIZE)
//quantization
#define SDF_MAX_VOXELS_DIST 4
//indirection
#define SDF_NUM_MIPS 3
#define SDF_INVALID_LUT_INDEX 0xFFFFFFFF
#define SDF_MAX_INDIRECTION_DIM_BITS 10
#define SDF_MAX_INDIRECTION_DIM (1<<SDF_MAX_INDIRECTION_DIM_BITS)
#define SDF_MAX_INDIRECTION_DIM_MASK (SDF_MAX_INDIRECTION_DIM - 1)

// lut table is encoded as 11_11_10 in 32 bits
#define SDF_INDIRECTION_BLOCK_SHIFT_XY 11
#define SDF_INDIRECTION_BLOCK_MASK_XY ((1<<SDF_INDIRECTION_BLOCK_SHIFT_XY) - 1)

// we use object packed to float4/ByteBuffer which is wasting 2 words, but allows to get number of loaded mips without reading whole 12 words
#define SDF_PACKED_MIP_SIZE 3 // in float4

struct PackedObjectMip // this is c++ only structure, as alignment rules are different in hlsl (float3 + uint!=float4)
{
  float3 extent;
  uint maxDist__object_id; // when executing command it is 16 bit half maxEncodedDist (negative if mostly two sided) | (objectId<<16).
                           // when reading in buffer, it is just float maxEncodedDist (decoded), although we could store some 16 bit there.
  float3 toIndirMul; uint indirectionDim;//2 bits are free, 10|10|10 are dimensions
  float3 toIndirAdd; uint indirOffset;
};

#define SDF_SUPPORT_OBJECT_TRACING 0

#if SDF_SUPPORT_OBJECT_TRACING
#define SDF_PACKED_INSTANCE_SIZE 8 // in float4
#else
#define SDF_PACKED_INSTANCE_SIZE 5 // in float4
#endif

struct IndirectionCommand {uint destPosition, sourcePosition, count;};

// we read as float4 to avoid reading all 8 float4 when we don't need last 4
struct ObjectSDFInstancePacked
{
  float4x3 worldToLocal;
  float3 extent;
  uint objectId;
  float4 volumeToWorldScale;
#if SDF_SUPPORT_OBJECT_TRACING
  float4x3 localToWorld;
#endif
};

struct ObjectSDFInstance
{
  float4x4 worldToLocal;
  float3 extent;
  uint objectId;
  float4 volumeToWorldScale;
#if SDF_SUPPORT_OBJECT_TRACING
  float4x4 localToWorld;
#endif
};

#endif