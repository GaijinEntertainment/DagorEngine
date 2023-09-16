include "hardware_defines.sh"
include "particle_system_lib.sh"

hlsl
{
  #define PFX_THREAD_GROUP_SIZE  32
  #define PFX_GLOBAL_PARAM_COUNT 64

  #define PFX_SYSTEM_HEAD_SIZE 1
  #define PFX_EMITTER_HEAD_SIZE 1
  #define PFX_PARTICLEGROUP_HEAD_SIZE 1

  #define PFX_DISPATCH_SLOT         b0
  #define PFX_GLOBAL_PARAM_SLOT     b1

  #define PFX_SYSTEM_DATA_SLOT      t2
  #define PFX_EMITTER_DATA_SLOT     t3
  #define PFX_PARTICLES_DATA_SLOT   t4

  #define PFX_EMITTER_DATA_RW_SLOT   u3
  #define PFX_PARTICLES_DATA_RW_SLOT u4
}

macro PFX_DISPATCH_DATA()
  hlsl
  {
    // this struct must match pfx::DispatchData
    struct PfxDispatchData
    {
      uint block_offset;
      uint block_size;
      uint system_offset;
      uint emitter_offset;

      uint start;
      uint count;
      uint elem_size;
      uint elem_limit;

      float elem_life_Limit;
      uint rnd_seed;
      uint unused0;
      uint unused1;
    };
    cbuffer PfxDispatchDataCb : register( PFX_DISPATCH_SLOT )
    {
      PfxDispatchData pfx_dispatch_data;
    }
  }
endmacro

macro PFX_GLOBAL_PARAM_DATA()
  hlsl
  {
    cbuffer PfxGlobParamCb : register( PFX_GLOBAL_PARAM_SLOT )
    {
      float4 pfx_global_params[ PFX_GLOBAL_PARAM_COUNT ];
    }

    float pfx_get_global_param1f( uint offset )
    {
      return pfx_global_params[ offset / 4 ][ offset % 4 ];
    }
    float2 pfx_get_global_param2f( uint offset )
    {
      float2 v;
      v.x = pfx_get_global_param1f( offset + 0 );
      v.y = pfx_get_global_param1f( offset + 1 );
      return v;
    }
    float3 pfx_get_global_param3f( uint offset )
    {
      float3 v;
      v.x = pfx_get_global_param1f( offset + 0 );
      v.y = pfx_get_global_param1f( offset + 1 );
      v.z = pfx_get_global_param1f( offset + 2 );
      return v;
    }
    float4 pfx_get_global_param4f( uint offset )
    {
      float4 v;
      v.x = pfx_get_global_param1f( offset + 0 );
      v.y = pfx_get_global_param1f( offset + 1 );
      v.z = pfx_get_global_param1f( offset + 2 );
      v.w = pfx_get_global_param1f( offset + 3 );
      return v;
    }
    float4x4 pfx_get_global_param44mat( uint offset )
    {
      float4x4 v;
      v[0] = pfx_get_global_param4f( offset + 0 );
      v[1] = pfx_get_global_param4f( offset + 4 );
      v[2] = pfx_get_global_param4f( offset + 8 );
      v[3] = pfx_get_global_param4f( offset + 12 );
      return v;
    }
  }
endmacro

macro PFX_SYSTEM_DATA()
  hlsl
  {
    StructuredBuffer<float4> pfx_system_data : register( PFX_SYSTEM_DATA_SLOT );
    float pfx_get_system_param1f( uint offset )
    {
      return structuredBufferAt(pfx_system_data,  pfx_dispatch_data.system_offset + offset / 4 )[ offset % 4 ];
    }
    float2 pfx_get_system_param2f( uint offset )
    {
      float2 v;
      v.x = pfx_get_system_param1f( offset + 0 );
      v.y = pfx_get_system_param1f( offset + 1 );
      return v;
    }
    float3 pfx_get_system_param3f( uint offset )
    {
      float3 v;
      v.x = pfx_get_system_param1f( offset + 0 );
      v.y = pfx_get_system_param1f( offset + 1 );
      v.z = pfx_get_system_param1f( offset + 2 );
      return v;
    }
    float4 pfx_get_system_param4f( uint offset )
    {
      float4 v;
      v.x = pfx_get_system_param1f( offset + 0 );
      v.y = pfx_get_system_param1f( offset + 1 );
      v.z = pfx_get_system_param1f( offset + 2 );
      v.w = pfx_get_system_param1f( offset + 3 );
      return v;
    }
    float4x4 pfx_get_system_param44mat( uint offset )
    {
      float4x4 v;
      v[0] = pfx_get_system_param4f( offset + 0 );
      v[1] = pfx_get_system_param4f( offset + 4 );
      v[2] = pfx_get_system_param4f( offset + 8 );
      v[3] = pfx_get_system_param4f( offset + 12 );
      return v;
    }

    int pfx_get_system_param1i( uint offset )
    {
      return asint( pfx_get_system_param1f( offset ) );
    }

    uint pfx_get_system_param1ui( uint offset )
    {
      return asuint( pfx_get_system_param1f( offset ) );
    }
  }
endmacro

macro PFX_EMITTER_DATA()
  hlsl
  {
    #if PFX_EMITTER_DATA_RW
    RWStructuredBuffer<float4> pfx_emitter_data : register( PFX_EMITTER_DATA_RW_SLOT );
    #else
    StructuredBuffer<float4> pfx_emitter_data : register( PFX_EMITTER_DATA_SLOT );
    #endif

    float pfx_get_emitter_param1f( uint offset )
    {
      return structuredBufferAt(pfx_emitter_data,  pfx_dispatch_data.emitter_offset + offset / 4 )[ offset % 4 ];
    }
    float2 pfx_get_emitter_param2f( uint offset )
    {
      float2 v;
      v.x = pfx_get_emitter_param1f( offset + 0 );
      v.y = pfx_get_emitter_param1f( offset + 1 );
      return v;
    }
    float3 pfx_get_emitter_param3f( uint offset )
    {
      float3 v;
      v.x = pfx_get_emitter_param1f( offset + 0 );
      v.y = pfx_get_emitter_param1f( offset + 1 );
      v.z = pfx_get_emitter_param1f( offset + 2 );
      return v;
    }
    float4 pfx_get_emitter_param4f( uint offset )
    {
      float4 v;
      v.x = pfx_get_emitter_param1f( offset + 0 );
      v.y = pfx_get_emitter_param1f( offset + 1 );
      v.z = pfx_get_emitter_param1f( offset + 2 );
      v.w = pfx_get_emitter_param1f( offset + 3 );
      return v;
    }
  }
endmacro

macro PFX_PARTICLES_DATA()
  hlsl
  {
    #if PFX_PARTICLES_DATA_RW
    RWStructuredBuffer<float4> pfx_particles_data : register( PFX_PARTICLES_DATA_RW_SLOT );
    #else
    StructuredBuffer<float4> pfx_particles_data : register( PFX_PARTICLES_DATA_SLOT );
    #endif
  }
endmacro

macro PFX_PARTICLES_SIMULATION_DISPATCH_INFO()
  hlsl
  {
    struct PfxInfo
    {
      uint local_id;
      uint local_count;
      uint global_id;
      uint global_count;
      uint rnd_seed;
      float life_limit;
    };
  }
endmacro

macro PFX_EMITTER_SIMULATION_DISPATCH_INFO()
  hlsl
  {
    struct PfxInfo
    {
      uint param_id;
      uint param_count;
      uint rnd_seed;
    };
  }
endmacro

macro PFX_PARTICLES_RENDER_DISPATCH_INFO()
  hlsl
  {
    struct PfxInfo
    {
      uint vertex_id;
      uint local_id;
      uint global_id;
      uint rnd_seed;
      float life_limit;
    };
  }
endmacro

macro PFX_UTILS()
  hlsl
  {
    uint pfx_get_elem_id( uint inst_id )
    {
    #ifdef PFX_USE_REVERSE_DATA
      uint offset = pfx_dispatch_data.count - inst_id - 1;
    #else
      uint offset = inst_id;
    #endif
      return ( offset + pfx_dispatch_data.start ) % pfx_dispatch_data.elem_limit;
    }
  }
endmacro

macro PFX_LOAD_ELEM(pfx_elem_size, pfx_data)
  hlsl
  {
    void pfx_load_elem( uint elem_id, out float4 out_data[pfx_elem_size] )
    {
      uint offset = pfx_dispatch_data.block_offset + elem_id * pfx_elem_size;

      UNROLL
      for ( int i = 0 ; i < pfx_elem_size ; ++i )
        out_data[i] = pfx_data[offset + i];
    }
  }
endmacro

macro PFX_SAVE_ELEM(pfx_elem_size, pfx_data)
  hlsl
  {
    void pfx_save_elem( uint elem_id, float4 in_data[pfx_elem_size] )
    {
      uint offset = pfx_dispatch_data.block_offset + elem_id * pfx_elem_size;

      UNROLL
      for ( int i = 0 ; i < pfx_elem_size ; ++i )
        pfx_data[offset + i] = in_data[i];
    }
  }
endmacro

// helpers

macro PFX_USE_GLOBAL_PARAM_1F(ofs, name)
  hlsl {#define name (pfx_get_global_param1f(ofs))}
endmacro

macro PFX_USE_GLOBAL_PARAM_2F(ofs, name)
  hlsl {#define name (pfx_get_global_param2f(ofs))}
endmacro

macro PFX_USE_GLOBAL_PARAM_3F(ofs, name)
  hlsl {#define name (pfx_get_global_param3f(ofs))}
endmacro

macro PFX_USE_GLOBAL_PARAM_4F(ofs, name)
  hlsl {#define name (pfx_get_global_param4f(ofs))}
endmacro

macro PFX_USE_GLOBAL_PARAM_MAT44(ofs, name)
  hlsl {#define name (pfx_get_global_param44mat(ofs))}
endmacro

macro PFX_USE_SYSTEM_PARAM_1F(ofs, name)
  hlsl {#define name (pfx_get_system_param1f(ofs))}
endmacro

macro PFX_USE_SYSTEM_PARAM_2F(ofs, name)
  hlsl {#define name (pfx_get_system_param2f(ofs))}
endmacro

macro PFX_USE_SYSTEM_PARAM_3F(ofs, name)
  hlsl {#define name (pfx_get_system_param3f(ofs))}
endmacro

macro PFX_USE_SYSTEM_PARAM_4F(ofs, name)
  hlsl {#define name (pfx_get_system_param4f(ofs))}
endmacro

macro PFX_USE_SYSTEM_PARAM_MAT44(ofs, name)
  hlsl {#define name (pfx_get_system_param44mat(ofs))}
endmacro

macro PFX_USE_SYSTEM_PARAM_1I(ofs, name)
  hlsl {#define name (pfx_get_system_param1i(ofs))}
endmacro


macro PFX_USE_EMITTER_PARAM_1F(ofs, name)
  hlsl {#define name (pfx_get_emitter_param1f(ofs))}
endmacro

macro PFX_USE_CONST_PARAM(name, v)
  hlsl {#define name (v)}
endmacro

macro PFX_USE_SYSTEM_PARAM_LOAD()
  hlsl
  {
    float pfx_load_system_param1f( in out uint offset )
    {
      return pfx_get_system_param1f( offset++ );
    }

    float2 pfx_load_system_param2f( in out uint offset )
    {
      float2 v;
      v.x = pfx_load_system_param1f( offset );
      v.y = pfx_load_system_param1f( offset );
      return v;
    }

    float3 pfx_load_system_param3f( in out uint offset )
    {
      float3 v;
      v.x = pfx_load_system_param1f( offset );
      v.y = pfx_load_system_param1f( offset );
      v.z = pfx_load_system_param1f( offset );
      return v;
    }

    float4 pfx_load_system_param4f( in out uint offset )
    {
      float4 v;
      v.x = pfx_load_system_param1f( offset );
      v.y = pfx_load_system_param1f( offset );
      v.z = pfx_load_system_param1f( offset );
      v.w = pfx_load_system_param1f( offset );
      return v;
    }

    float4x4 pfx_load_system_param44mat( in out uint offset )
    {
      float4x4 v;
      v[0] = pfx_load_system_param4f( offset );
      v[1] = pfx_load_system_param4f( offset );
      v[2] = pfx_load_system_param4f( offset );
      v[3] = pfx_load_system_param4f( offset );
      return v;
    }

    int pfx_load_system_param1i( in out uint offset )
    {
      return asint( pfx_load_system_param1f( offset ) );
    }

    uint pfx_load_system_param1ui( in out uint offset )
    {
      return asuint( pfx_load_system_param1f( offset ) );
    }
  }
endmacro

// external

macro PFX_EMITTER_EMISSION_INIT()
  hlsl { #define PFX_EMITTER_DATA_RW 1 }
  PFX_DISPATCH_DATA()
  PFX_GLOBAL_PARAM_DATA()
  PFX_SYSTEM_DATA()
  PFX_EMITTER_DATA()
  PFX_EMITTER_SIMULATION_DISPATCH_INFO()
endmacro

macro PFX_EMITTER_EMISSION_USE()
  PFX_SAVE_ELEM( 1, pfx_emitter_data )
  PFX_LOAD_ELEM( 1, pfx_emitter_data )

  hlsl(cs)
  {
    [numthreads( 1, 1, 1 )]
    void pfx_emitter_emission_cs( uint3 t_id : SV_DispatchThreadID )
    {
      float4 elem[1];
      PfxInfo info;
      info.param_id = t_id.x;
      info.param_count = pfx_dispatch_data.count;
      info.rnd_seed = info.param_id + pfx_dispatch_data.rnd_seed;
      pfx_load_elem( info.param_id, elem );
      pfx_emitter_emission_cb( elem[0], info );
      pfx_save_elem( info.param_id, elem );
    }
  }
  compile( "cs_5_0", "pfx_emitter_emission_cs" );
endmacro

macro PFX_EMITTER_SIMULATION_INIT()
  hlsl { #define PFX_EMITTER_DATA_RW 1 }
  PFX_DISPATCH_DATA()
  PFX_GLOBAL_PARAM_DATA()
  PFX_SYSTEM_DATA()
  PFX_EMITTER_DATA()
  PFX_EMITTER_SIMULATION_DISPATCH_INFO()
endmacro

macro PFX_EMITTER_SIMULATION_USE()
  PFX_SAVE_ELEM( 1, pfx_emitter_data )
  PFX_LOAD_ELEM( 1, pfx_emitter_data )

  hlsl(cs)
  {
    [numthreads( 1, 1, 1 )]
    void pfx_emitter_simulation_cs( uint3 t_id : SV_DispatchThreadID )
    {
      float4 elem[1];
      PfxInfo info;
      info.param_id = t_id.x;
      info.param_count = pfx_dispatch_data.count;
      info.rnd_seed = info.param_id + pfx_dispatch_data.rnd_seed;
      pfx_load_elem( info.param_id, elem );
      pfx_emitter_simulation_cb( elem[0], info );
      pfx_save_elem( info.param_id, elem );
    }
  }
  compile( "cs_5_0", "pfx_emitter_simulation_cs" );
endmacro

macro PFX_PARTICLES_EMISSION_INIT()
  hlsl { #define PFX_PARTICLES_DATA_RW 1 }
  PFX_DISPATCH_DATA()
  PFX_GLOBAL_PARAM_DATA()
  PFX_SYSTEM_DATA()
  PFX_EMITTER_DATA()
  PFX_PARTICLES_DATA()
  PFX_PARTICLES_SIMULATION_DISPATCH_INFO()
  PFX_UTILS()
endmacro

macro PFX_PARTICLES_EMISSION_USE(pfx_elem_size)
  PFX_SAVE_ELEM( pfx_elem_size, pfx_particles_data )

  hlsl(cs)
  {
    [numthreads( PFX_THREAD_GROUP_SIZE, 1, 1 )]
    void pfx_particles_emission_cs( uint3 t_id : SV_DispatchThreadID )
    {
      if ( t_id.x < pfx_dispatch_data.count )
      {
        PfxInfo info;
        info.local_id = t_id.x;
        info.global_id = pfx_get_elem_id( t_id.x );
        info.local_count = pfx_dispatch_data.count;
        info.global_count = pfx_dispatch_data.elem_limit;
        info.rnd_seed = info.global_id + pfx_dispatch_data.rnd_seed;
        info.life_limit = pfx_dispatch_data.elem_life_Limit;

        float4 elem[pfx_elem_size];
        pfx_particles_emission_cb( elem, info );
        pfx_save_elem( info.global_id, elem );
      }
    }
  }
  compile( "cs_5_0", "pfx_particles_emission_cs" );
endmacro

macro PFX_PARTICLES_SIMULATION_INIT()
  hlsl { #define PFX_PARTICLES_DATA_RW 1 }
  PFX_DISPATCH_DATA()
  PFX_GLOBAL_PARAM_DATA()
  PFX_SYSTEM_DATA()
  PFX_EMITTER_DATA()
  PFX_PARTICLES_DATA()
  PFX_PARTICLES_SIMULATION_DISPATCH_INFO()
  PFX_UTILS()
endmacro

macro PFX_PARTICLES_SIMULATION_USE(pfx_elem_size)
  PFX_SAVE_ELEM( pfx_elem_size, pfx_particles_data )
  PFX_LOAD_ELEM( pfx_elem_size, pfx_particles_data )

  hlsl(cs)
  {
    [numthreads( PFX_THREAD_GROUP_SIZE, 1, 1 )]
    void pfx_particles_simulation_cs( uint3 t_id : SV_DispatchThreadID )
    {
      if ( t_id.x < pfx_dispatch_data.count )
      {
        PfxInfo info;
        info.local_id = t_id.x;
        info.global_id = pfx_get_elem_id( t_id.x );
        info.local_count = pfx_dispatch_data.count;
        info.global_count = pfx_dispatch_data.elem_limit;
        info.rnd_seed = info.global_id + pfx_dispatch_data.rnd_seed;
        info.life_limit = pfx_dispatch_data.elem_life_Limit;

        float4 elem[pfx_elem_size];
        pfx_load_elem( info.global_id, elem );
        pfx_particles_simulation_cb( elem, info );
        pfx_save_elem( info.global_id, elem );
      }
    }
  }
  compile( "cs_5_0", "pfx_particles_simulation_cs" );
endmacro

macro PFX_RENDER_INIT_FORWARD()
  PFX_DISPATCH_DATA()
  PFX_GLOBAL_PARAM_DATA()
  PFX_SYSTEM_DATA()
  PFX_EMITTER_DATA()
  PFX_PARTICLES_DATA()
  PFX_PARTICLES_RENDER_DISPATCH_INFO()
  PFX_UTILS()
endmacro

macro PFX_RENDER_INIT_REVERSE()
  hlsl { #define PFX_USE_REVERSE_DATA }
  PFX_RENDER_INIT_FORWARD()
endmacro

macro PFX_RENDER_USE(pfx_elem_size, pfx_vs_output)
  PFX_LOAD_ELEM( pfx_elem_size, pfx_particles_data )

  hlsl(vs)
  {
    pfx_vs_output pfx_render_vs( uint vertex_id : SV_VertexID HW_USE_INSTANCE_ID )
    {
      PfxInfo info;
      info.vertex_id = vertex_id;
      info.local_id = instance_id;
      info.global_id = pfx_get_elem_id( instance_id );
      info.rnd_seed = info.local_id + pfx_dispatch_data.rnd_seed;
      info.life_limit = pfx_dispatch_data.elem_life_Limit;
      float4 elem[pfx_elem_size];
      pfx_load_elem( info.global_id, elem );
      return pfx_render_vs_cb( elem, info );
    }
  }

  hlsl(ps)
  {
    float4 pfx_render_ps( pfx_vs_output vs_input ) : SV_Target0
    {
      return pfx_render_ps_cb( vs_input );
    }
  }

  compile( "target_vs", "pfx_render_vs" );
  compile( "target_ps", "pfx_render_ps" );

endmacro