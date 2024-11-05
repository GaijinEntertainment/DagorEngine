#ifndef DAFX_SHADERS_HLSL
#define DAFX_SHADERS_HLSL

#ifdef __cplusplus
DAFX_INLINE
void update_culling_data( uint cull_id, BBox bbox, int4 *data )
#else
void update_culling_data( uint cull_id, BBox bbox )
#endif
{
#ifndef __cplusplus // cpp culling is always valid
 #if !DAFX_UPDATE_GPU_CULLING // for warmup and presim we skip culling completely
    return;
 #endif
#endif

#if DAFX_DEBUG_CULLING_FETCH
  #ifdef __cplusplus
    data[cull_id * 2].w = cull_id;
    data[cull_id * 2 + 1].w = cull_id;
  #else
    structuredBufferAt(dafx_culling_data,( cull_id * 2 + 0 ) * 4 + 3) = cull_id;
    structuredBufferAt(dafx_culling_data,( cull_id * 2 + 1 ) * 4 + 3) = cull_id;
  #endif
#endif

  if ( bbox.bmin.x >= bbox.bmax.x )
    return;

  bbox.bmin *= DAFX_CULLING_SCALE_INV;
  bbox.bmax *= DAFX_CULLING_SCALE_INV;

  int3 i0 = int3( bbox.bmin.x, bbox.bmin.y, bbox.bmin.z );
  int3 i1 = int3( bbox.bmax.x, bbox.bmax.y, bbox.bmax.z );

#ifdef __cplusplus
  int4 &line0 = data[cull_id * 2 + 0];
  int4 &line1 = data[cull_id * 2 + 1];

  line0.x = min( line0.x, i0.x );
  line0.y = min( line0.y, i0.y );
  line0.z = min( line0.z, i0.z );
  line0.w++;

  line1.x = max( line1.x, i1.x );
  line1.y = max( line1.y, i1.y );
  line1.z = max( line1.z, i1.z );
#else

  #if (_HARDWARE_VULKAN || _HARDWARE_METAL) && SHADER_COMPILER_DXC
    //DXC can't write to component of buffer when reflected to Spir-V
    #define CULLING_COMPONENT
  #else
    // metal supports atomics only for 1 components values
    #define CULLING_COMPONENT .x
  #endif

  InterlockedMin( structuredBufferAt(dafx_culling_data, ( cull_id * 2 + 0 ) * 4 + 0) CULLING_COMPONENT, i0.x );
  InterlockedMin( structuredBufferAt(dafx_culling_data, ( cull_id * 2 + 0 ) * 4 + 1) CULLING_COMPONENT, i0.y );
  InterlockedMin( structuredBufferAt(dafx_culling_data, ( cull_id * 2 + 0 ) * 4 + 2) CULLING_COMPONENT, i0.z );
  InterlockedAdd( structuredBufferAt(dafx_culling_data, ( cull_id * 2 + 0 ) * 4 + 3) CULLING_COMPONENT, 1 ); // alive parts

  InterlockedMax( structuredBufferAt(dafx_culling_data, ( cull_id * 2 + 1 ) * 4 + 0) CULLING_COMPONENT, i1.x );
  InterlockedMax( structuredBufferAt(dafx_culling_data, ( cull_id * 2 + 1 ) * 4 + 1) CULLING_COMPONENT, i1.y );
  InterlockedMax( structuredBufferAt(dafx_culling_data, ( cull_id * 2 + 1 ) * 4 + 2) CULLING_COMPONENT, i1.z );
#endif
}

#ifdef __cplusplus
DAFX_INLINE
void update_culling_data( uint cull_id, float4_cref v, int4 *data )
#else
void update_culling_data( uint cull_id, float4_cref v )
#endif
{
  BBox bbox;
  bbox.bmin = float3( v.x - v.w, v.y - v.w, v.z - v.w );
  bbox.bmax = float3( v.x + v.w, v.y + v.w, v.z + v.w );
#ifdef __cplusplus
  update_culling_data(cull_id, bbox, data);
#else
  update_culling_data(cull_id, bbox );
#endif
}

DAFX_INLINE
#ifdef __cplusplus
void dafx_parse_dispatch_desc(const DispatchDesc &ddesc, uint &rnd_seed, uint &culling_id, uint &alive_start, uint &alive_count, uint &lod)
#else
void dafx_parse_dispatch_desc(DispatchDesc ddesc, out uint rnd_seed, out uint culling_id, out uint alive_start, out uint alive_count, out uint lod)
#endif
{
  rnd_seed = ddesc.rndSeedAndCullingId & 0xff;
  culling_id = ddesc.rndSeedAndCullingId >> 8;
  alive_start = ddesc.aliveStartAndCount & 0xffff;
  alive_count = ddesc.aliveStartAndCount >> 16;
  lod = ddesc.headOffsetAndLodOfs >> 24;
}

#ifdef __cplusplus

  DAFX_INLINE
  void dafx_fill_cdesc( const DataHead &head, const DispatchDesc &ddesc, ComputeCallDesc &cdesc )
  {
    cdesc.parentRenOffset = head.parentCpuRenOffset / DAFX_ELEM_STRIDE;
    cdesc.parentSimOffset = head.parentCpuSimOffset / DAFX_ELEM_STRIDE;
    cdesc.dataRenOffsetStart = head.dataCpuRenOffset / DAFX_ELEM_STRIDE;
    cdesc.dataSimOffsetStart = head.dataCpuSimOffset / DAFX_ELEM_STRIDE;
    cdesc.serviceDataOffset = head.serviceCpuOffset / DAFX_ELEM_STRIDE;
    cdesc.lifeLimit = head.lifeLimit;
    cdesc.lifeLimitRcp = head.lifeLimitRcp;
    dafx_parse_dispatch_desc(ddesc, cdesc.rndSeed, cdesc.cullingId, cdesc.aliveStart, cdesc.aliveCount, cdesc.lod);
    cdesc.idx = 0;
  }

  DAFX_INLINE
  void dafx_emission_shader( dafx::CpuExecData &data )
  {
#if DAFX_PRELOAD_PARENT_DATA
  #define DAFX_SHADER_INC_CALL dafx_emission_shader_cb(cdesc, curBuf, parentRenData, parentSimData, float(cdesc.count - cdesc.idx - 1) / cdesc.count, *globalData)
#else
  #define DAFX_SHADER_INC_CALL dafx_emission_shader_cb(cdesc, curBuf, *globalData )
#endif
    #include "dafx_shaders_inc.hlsli"
    #undef DAFX_SHADER_INC_CALL
  }

  DAFX_INLINE
  void dafx_simulation_shader( dafx::CpuExecData &data )
  {
#if DAFX_PRELOAD_PARENT_DATA
  #define DAFX_SHADER_INC_CALL dafx_simulation_shader_cb(cdesc, curBuf, parentRenData, parentSimData, *globalData)
#else
  #define DAFX_SHADER_INC_CALL dafx_simulation_shader_cb(cdesc, curBuf, *globalData)
#endif
    #include "dafx_shaders_inc.hlsli"
    #undef DAFX_SHADER_INC_CALL
  }

#else

  #if DAFX_EMISSION_SHADER_ENABLED || DAFX_SIMULATION_SHADER_ENABLED

  // workaround metal optimizer bug. it treats ++ as signed int addition and fails to fold
  #if _HARDWARE_METAL && !SHADER_COMPILER_DXC
  void dafx_fill_data_head( int ofs, out DataHead head )
  #else
  void dafx_fill_data_head( uint ofs, out DataHead head )
  #endif
  {
    head.parentCpuRenOffset = 0;
    head.parentCpuSimOffset = 0;
    head.dataCpuRenOffset = 0;
    head.dataCpuSimOffset = 0;
    head.serviceCpuOffset = 0;
    ofs += 5;

    head.parentGpuRenOffset = structuredBufferAt(dafx_system_data, ofs); ofs++;
    head.parentGpuSimOffset = structuredBufferAt(dafx_system_data, ofs); ofs++;
    head.dataGpuRenOffset = structuredBufferAt(dafx_system_data, ofs); ofs++;
    head.dataGpuSimOffset = structuredBufferAt(dafx_system_data, ofs); ofs++;
    head.serviceGpuOffset = structuredBufferAt(dafx_system_data, ofs); ofs++;

    head.parentRenStride = structuredBufferAt(dafx_system_data, ofs); ofs++;
    head.parentSimStride = structuredBufferAt(dafx_system_data, ofs); ofs++;

    head.dataRenStride = structuredBufferAt(dafx_system_data, ofs); ofs++;
    head.dataSimStride = structuredBufferAt(dafx_system_data, ofs); ofs++;

    head.lifeLimit = asfloat( structuredBufferAt(dafx_system_data, ofs) ); ofs++;
    head.lifeLimitRcp = asfloat( structuredBufferAt(dafx_system_data, ofs) ); ofs++;

    head.elemLimit = structuredBufferAt(dafx_system_data, ofs); ofs++;
  }

  void dafx_fill_cdesc( DataHead head, DispatchDesc ddesc, in out ComputeCallDesc cdesc )
  {
    cdesc.parentRenOffset = head.parentGpuRenOffset / DAFX_ELEM_STRIDE;
    cdesc.parentSimOffset = head.parentGpuSimOffset / DAFX_ELEM_STRIDE;
    cdesc.dataRenOffsetStart = head.dataGpuRenOffset / DAFX_ELEM_STRIDE;
    cdesc.dataSimOffsetStart = head.dataGpuSimOffset / DAFX_ELEM_STRIDE;
    cdesc.serviceDataOffset = head.serviceGpuOffset / DAFX_ELEM_STRIDE;
    cdesc.lifeLimit = head.lifeLimit;
    cdesc.lifeLimitRcp = head.lifeLimitRcp;
    dafx_parse_dispatch_desc(ddesc, cdesc.rndSeed, cdesc.cullingId, cdesc.aliveStart, cdesc.aliveCount, cdesc.lod);
    cdesc.start = 0;
    cdesc.count = 0;
    cdesc.idx = 0;
    cdesc.gid = 0;
    cdesc.dataRenOffsetCurrent = 0;
    cdesc.dataSimOffsetCurrent = 0;
    cdesc.dataRenOffsetRelative = 0;
    cdesc.dataSimOffsetRelative = 0;
  }
  #endif

  #if DAFX_EMISSION_SHADER_ENABLED
  [numthreads( DAFX_DEFAULT_WARP, 1, 1)]
  void dafx_emission_shader( uint3 t_id : SV_DispatchThreadID )
  {
#if DAFX_PRELOAD_PARENT_DATA
  #define DAFX_SHADER_INC_CALL dafx_emission_shader_cb(cdesc, 0, parentRenData, parentSimData, float( count - localId - 1 ) / count, globalData)
#else
  #define DAFX_SHADER_INC_CALL dafx_emission_shader_cb(cdesc, 0, globalData)
#endif
    #include "dafx_shaders_inc.hlsli"
    #undef DAFX_SHADER_INC_CALL
  }
  #endif

  #if DAFX_SIMULATION_SHADER_ENABLED
  [numthreads( DAFX_DEFAULT_WARP, 1, 1)]
  void dafx_simulation_shader( uint3 t_id : SV_DispatchThreadID )
  {
#if DAFX_PRELOAD_PARENT_DATA
  #define DAFX_SHADER_INC_CALL dafx_simulation_shader_cb(cdesc, 0, parentRenData, parentSimData, globalData)
#else
  #define DAFX_SHADER_INC_CALL dafx_simulation_shader_cb(cdesc, 0, globalData)
#endif
    #include "dafx_shaders_inc.hlsli"
    #undef DAFX_SHADER_INC_CALL
  }
  #endif

  #if DAFX_CULLING_DISCARD_SHADER_ENABLED
  [numthreads( DAFX_DEFAULT_WARP, 1, 1 )]
  void dafx_culling_discard( uint3 t_id : SV_DispatchThreadID )
  {
    uint count = get_immediate_dword_0();

    if ( t_id.x >= count )
      return;

    // 0x80000000 in uint = -INT_MAX in int
    structuredBufferAt(dafx_culling_data, ( t_id.x * 2 + 0 ) * 4 + 0) = asuint( DAFX_CULLING_DISCARD_MIN );
    structuredBufferAt(dafx_culling_data, ( t_id.x * 2 + 0 ) * 4 + 1) = asuint( DAFX_CULLING_DISCARD_MIN );
    structuredBufferAt(dafx_culling_data, ( t_id.x * 2 + 0 ) * 4 + 2) = asuint( DAFX_CULLING_DISCARD_MIN );
    structuredBufferAt(dafx_culling_data, ( t_id.x * 2 + 0 ) * 4 + 3) = 0;

    structuredBufferAt(dafx_culling_data, ( t_id.x * 2 + 1 ) * 4 + 0) = asuint( DAFX_CULLING_DISCARD_MAX );
    structuredBufferAt(dafx_culling_data, ( t_id.x * 2 + 1 ) * 4 + 1) = asuint( DAFX_CULLING_DISCARD_MAX );
    structuredBufferAt(dafx_culling_data, ( t_id.x * 2 + 1 ) * 4 + 2) = asuint( DAFX_CULLING_DISCARD_MAX );
  }
  #endif

#endif

#endif