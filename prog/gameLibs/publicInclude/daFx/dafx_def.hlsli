#ifndef DAFX_DEF_HLSL
#define DAFX_DEF_HLSL 1

#include <daFx/dafx_dispatch_desc.hlsli>

#ifdef __cplusplus
#define DAFX_USE_VEC_MATH
// #define DAFX_DEBUG_VALUES // ONLY FOR LOCAL DEBUG!!! DO NOT ENABLE IN MASTER!!!
#endif

#ifdef __cplusplus
#define DAFX_SYSTEM_DATA_UAV_SLOT 0
#define DAFX_CULLING_DATA_UAV_SLOT 1
#else
#define DAFX_SYSTEM_DATA_UAV_SLOT u0
#define DAFX_CULLING_DATA_UAV_SLOT u1
#endif

#define DAFX_FLIPBOOK_MAX_KEYFRAME_DIM 16

#define DAFX_INVALID_BOUNDARY_OFFSET 0xFFFF // 16 bits are enough (not packed currently)


#define DAFX_DEFAULT_WARP 32
#ifndef __cplusplus
#pragma wave32
#endif

// #define DAFX_DEBUG_CULLING_FETCH 1

#define DAFX_ELEM_STRIDE 4

#ifdef __cplusplus
#define DAFX_CULLING_STRIDE (8*4)
#else
#define DAFX_CULLING_STRIDE 8
#endif

#define DAFX_CULLING_SCALE 0.01f
#define DAFX_CULLING_SCALE_INV ( 1.f / DAFX_CULLING_SCALE )

#define DAFX_CULLING_DISCARD_MIN 0x7fffffff
#define DAFX_CULLING_DISCARD_MAX 0x80000000

#define dafx_pi 3.14159265359f
#define dafx_pi_rcp ( 1.f / dafx_pi )
#define dafx_2pi ( dafx_pi * 2.f )
#define dafx_2pi_rcp ( 1.f / dafx_2pi )

//
// basic types
//
#ifdef __cplusplus
  using dafx_byte = unsigned char;
  using bool_ref = bool&;

  using uint_ref = uint&;
  using uint1_ref = uint&;
  using uint2_ref = uint2&;
  using uint3_ref = uint3&;
  using uint4_ref = uint4&;

  using uint1_cref = const uint&;
  using uint2_cref = const uint2&;
  using uint3_cref = const uint3&;
  using uint4_cref = const uint4&;

  using float_ref = float&;
  using float1_ref = float&;
  using float2_ref = float2&;
  using float3_ref = float3&;
  using float4_ref = float4&;

  using float1_cref = const float&;
  using float2_cref = const float2&;
  using float3_cref = const float3&;
  using float4_cref = const float4&;

  using float3x3 = Matrix3;
  using float3x3_ref = Matrix3&;
  using float3x3_cref = const Matrix3&;
  using float4x4 = TMatrix4;
  using float4x4_ref = TMatrix4&;
  using float4x4_cref = const TMatrix4&;
#else
  #define dafx_byte uint
  #define bool_ref in out bool

  #define uint_ref in out uint
  #define uint1_ref in out uint
  #define uint2_ref in out uint2
  #define uint3_ref in out uint3
  #define uint4_ref in out uint4

  #define uint1_cref uint
  #define uint2_cref uint2
  #define uint3_cref uint3
  #define uint4_cref uint4

  #define float_ref in out float
  #define float1_ref in out float
  #define float2_ref in out float2
  #define float3_ref in out float3
  #define float4_ref in out float4

  #define float1_cref float
  #define float2_cref float2
  #define float3_cref float3
  #define float4_cref float4

  #define float3x3_ref in out float3x3
  #define float3x3_cref float3x3
  #define float4x4_ref in out float4x4
  #define float4x4_cref float4x4
#endif

//
// platform specific stuff
//
#ifdef __cplusplus
  #define ComputeCallDesc_cref const ComputeCallDesc&
  #define DispatchDesc_cref const DispatchDesc&
  #define GlobalData_cref const GlobalData&
  #define BufferData_ref uint32_t*
  using BufferData_cref = const uint32_t*;
  #define DAFX_INLINE __forceinline
  #define DAFX_REF(a) a&
  #define DAFX_OREF(a) a&
  #define DAFX_CREF(a) const a&
#else
  #define ComputeCallDesc_cref ComputeCallDesc
  #define DispatchDesc_cref DispatchDesc
  #define GlobalData_cref GlobalData
  #define BufferData_ref uint
  #define BufferData_cref uint
  #define DAFX_INLINE
  #define DAFX_REF(a) in out a
  #define DAFX_OREF(a) out a
  #define DAFX_CREF(a) a
  #define G_UNREFERENCED(a)
  #define G_STATIC_ASSERT(a)
#endif

//
// instance head
//
struct DataHead
{
  uint parentCpuRenOffset;
  uint parentCpuSimOffset;
  uint dataCpuRenOffset;
  uint dataCpuSimOffset;
  uint serviceCpuOffset;

  uint parentGpuRenOffset;
  uint parentGpuSimOffset;
  uint dataGpuRenOffset;
  uint dataGpuSimOffset;
  uint serviceGpuOffset;

  uint parentRenStride;
  uint parentSimStride;

  uint dataRenStride;
  uint dataSimStride;

  float lifeLimit;
  float lifeLimitRcp;

  uint elemLimit;
};
#define DAFX_DATA_HEAD_SIZE (17*4)

#ifdef __cplusplus
G_STATIC_ASSERT( sizeof( DataHead ) == DAFX_DATA_HEAD_SIZE );
#endif

//
// parsed-gpu params for client shader cb (dafx_emission_shader_cb/dafx_simulation_shader_cb)
//
struct ComputeCallDesc
{
  uint gid;
  uint idx;
  uint lod;
  uint start;
  uint count;
  uint aliveStart;
  uint aliveCount;

  uint parentRenOffset;
  uint parentSimOffset;

  uint dataRenOffsetStart;
  uint dataSimOffsetStart;

  uint dataRenOffsetCurrent;
  uint dataSimOffsetCurrent;

  uint dataRenOffsetRelative;
  uint dataSimOffsetRelative;

  uint serviceDataOffset;
  uint cullingId;

  float lifeLimit;
  float lifeLimitRcp;
  uint rndSeed;
};

//
// generic bbox
//
struct BBox
{
  float3 bmin;
  float3 bmax;
};

#ifdef DAFX_DEBUG_VALUES
#define DAFX_TEST_BYTE(a) G_ASSERT(a<=255);
#define DAFX_TEST_NORM_FLOAT(a) G_ASSERT(a>=0&&a<=1);{int cl = fpclassify(a); G_ASSERT(cl==FP_NORMAL||cl==FP_ZERO);};
#define DAFX_TEST_NORM_FLOAT3(a) DAFX_TEST_NORM_FLOAT(a.x);DAFX_TEST_NORM_FLOAT(a.y);DAFX_TEST_NORM_FLOAT(a.z);
#define DAFX_TEST_NORM_FLOAT4(a) DAFX_TEST_NORM_FLOAT(a.x);DAFX_TEST_NORM_FLOAT(a.y);DAFX_TEST_NORM_FLOAT(a.z);DAFX_TEST_NORM_FLOAT(a.w);
#define DAFX_TEST_UNNORM_FLOAT(a) {int cl = fpclassify(a); G_ASSERT(cl==FP_NORMAL||cl==FP_ZERO);};
#define DAFX_TEST_UNNORM_FLOAT3(a) DAFX_TEST_UNNORM_FLOAT(a.x);DAFX_TEST_UNNORM_FLOAT(a.y);DAFX_TEST_UNNORM_FLOAT(a.z);
#else
#define DAFX_TEST_BYTE(a)
#define DAFX_TEST_NORM_FLOAT(a)
#define DAFX_TEST_NORM_FLOAT3(a)
#define DAFX_TEST_NORM_FLOAT4(a)
#define DAFX_TEST_UNNORM_FLOAT3(a)
#endif


#endif