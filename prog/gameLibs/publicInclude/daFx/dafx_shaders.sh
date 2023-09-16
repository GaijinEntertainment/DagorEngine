hlsl
{
  #include "dafx_def.hlsli"
}

buffer dafx_global_data;
buffer dafx_system_data;
buffer dafx_render_calls;
buffer dafx_dispatch_descs;
float4 dafx_screen_pos_to_tc = (1, 1, 1, 1);

buffer dafx_frame_boundary_buffer;
int dafx_frame_boundary_buffer_enabled = 0; // checking 'buf != NULL' for buffers in stcode is not working currently
interval dafx_frame_boundary_buffer_enabled: off < 1, on;

int dafx_sbuffer_supported = 1;
interval dafx_sbuffer_supported: off < 1, on;

int dafx_update_gpu_culling = 1;
interval dafx_update_gpu_culling: off < 1, on;

macro DAFX_GLOBAL_DATA_INIT(stage)

  (stage)
  {
    dafx_global_data@cbuf = dafx_global_data hlsl
    {
      #include "dafx_global_data_desc.hlsli"
      cbuffer dafx_global_data@cbuf
      {
        uint4 dafx_global_data[DAFX_GLOBAL_DATA_SIZE/4];
      };
    }
  }

endmacro

macro DAFX_COMPUTE_INIT()
  DAFX_GLOBAL_DATA_INIT(cs)

  (cs) { immediate_dword_count = 2; }
  (cs)
  {
    dafx_dispatch_descs@cbuf = dafx_dispatch_descs hlsl
    {
      #include "dafx_dispatch_desc.hlsli"
      cbuffer dafx_dispatch_descs@cbuf
      {
        DispatchDesc dafx_dispatch_descs[DAFX_BUCKET_GROUP_SIZE];
      }
    }
  }

  hlsl(cs)
  {
    #define DAFX_RW_ENABLED

    RWStructuredBuffer<uint> dafx_system_data : register( DAFX_SYSTEM_DATA_UAV_SLOT );
    #if _HARDWARE_VULKAN
      //pipe right type via name suffix
      //otherwise it will be uint and bug out AtomicMin/Max ops
      RWStructuredBuffer<int> dafx_culling_data__hlslcc_int : register( DAFX_CULLING_DATA_UAV_SLOT );
      #define dafx_culling_data dafx_culling_data__hlslcc_int
    #else
      RWStructuredBuffer<int> dafx_culling_data : register( DAFX_CULLING_DATA_UAV_SLOT );
    #endif
  }

  if (dafx_update_gpu_culling == on)
  {
    hlsl { #define DAFX_UPDATE_GPU_CULLING 1 }
  }
endmacro

macro DAFX_EMISSION_USE()
  hlsl(cs)
  {
    #define DAFX_EMISSION_SHADER_ENABLED 1
    #include "dafx_shaders.hlsli"
  }
  compile( "cs_5_0", "dafx_emission_shader" );
endmacro

macro DAFX_SIMULATION_USE()
  hlsl(cs)
  {
    #define DAFX_SIMULATION_SHADER_ENABLED 1
    #include "dafx_shaders.hlsli"
  }
  compile( "cs_5_0", "dafx_simulation_shader" );
endmacro

macro DAFX_RENDER_DISPATCH_INIT(stage)

  (stage)
  {
    dafx_render_calls@cbuf = dafx_render_calls hlsl
    {
      #include "dafx_render_dispatch_desc.hlsli"
      cbuffer dafx_render_calls@cbuf
      {
        RenderDispatchDesc dafx_render_calls[DAFX_RENDER_GROUP_SIZE];
      }
    }
  }

endmacro


macro DAFX_RENDER_INIT()
  DAFX_GLOBAL_DATA_INIT(vs)
  DAFX_GLOBAL_DATA_INIT(ps)

  DAFX_RENDER_DISPATCH_INIT(vs)
  DAFX_RENDER_DISPATCH_INIT(ps)

  if ( dafx_sbuffer_supported == on ) // sbuffer not supported everywhere, but it almost always faster
  {
    (vs) { dafx_system_data@buf = dafx_system_data hlsl { StructuredBuffer<uint> dafx_system_data@buf; } }
    (ps) { dafx_system_data@buf = dafx_system_data hlsl { StructuredBuffer<uint> dafx_system_data@buf; } }
  }
  else
  {
    (vs) { dafx_system_data@buf = dafx_system_data hlsl { Buffer<uint> dafx_system_data@buf; } }
    (ps) { dafx_system_data@buf = dafx_system_data hlsl { Buffer<uint> dafx_system_data@buf; } }
  }

  if (in_editor_assume == no && dafx_sbuffer_supported == on && dafx_frame_boundary_buffer_enabled == on)
  {
    (vs) { dafx_frame_boundary_buffer@buf = dafx_frame_boundary_buffer hlsl { StructuredBuffer<float4> dafx_frame_boundary_buffer@buf; } }
    hlsl(vs) { #define MODFX_USE_FRAMEBOUNDS 1 }
  }

endmacro

macro DAFX_RENDER_USE()
  (vs) { immediate_dword_count = 2; }
  (ps) { immediate_dword_count = 2; }
  hlsl
  {
    void dafx_gen_render_data( uint instance_id, out uint gid, out uint data_ofs, out uint parent_ofs )
    {
      uint call_id = get_immediate_dword_0();
      RenderDispatchDesc desc = dafx_render_calls[call_id];

      uint start = desc.startAndLimit & 0xffff;
      uint limit = desc.startAndLimit >> 16;

      uint dataRenStride = desc.dataAndParentRenStride & 0xffff;
      // uint parentRenStride = desc.dataAndParentRenStride >> 16;

      gid = ( start + instance_id ) % limit;
      data_ofs = ( desc.dataRenOffset + gid * dataRenStride );
      parent_ofs = desc.parentRenOffset;
    }

    struct DafxRenderData
    {
      uint start;
      uint count;
      uint limit;
      uint instance_id;

      uint data_stride;
      // uint parent_stride;

      uint data_ofs;
      uint parent_ofs;
    };

    void dafx_get_render_info( uint instance_id, out DafxRenderData r )
    {
      r.instance_id = instance_id;

      uint call_id = get_immediate_dword_0();
      r.count = get_immediate_dword_1();

      RenderDispatchDesc desc = dafx_render_calls[call_id];

      r.start = desc.startAndLimit & 0xffff;
      r.limit = desc.startAndLimit >> 16;

      r.data_stride = desc.dataAndParentRenStride & 0xffff;
      // r.parent_stride = desc.dataAndParentRenStride >> 16;

      r.data_ofs = desc.dataRenOffset;
      r.parent_ofs = desc.parentRenOffset;
    }

    uint dafx_get_render_data_rel_offset( DafxRenderData r, bool is_forward )
    {
      uint iid = is_forward ? r.instance_id : r.count - r.instance_id - 1;
      uint gid = ( r.start + iid ) % r.limit;
      return gid * r.data_stride;
    }

    uint dafx_get_render_data_offset( DafxRenderData r, bool is_forward )
    {
      return r.data_ofs + dafx_get_render_data_rel_offset( r, is_forward );
    }

    uint dafx_get_render_data_offset_direct( DafxRenderData r )
    {
      return dafx_get_render_data_offset( r, true );
    }
  }
endmacro


macro DAFX_CULLING_DISCARD_USE()
  (cs) { immediate_dword_count = 1; }
  hlsl(cs)
  {
    #define DAFX_CULLING_DISCARD_SHADER_ENABLED 1
    RWStructuredBuffer<int> dafx_culling_data : register( DAFX_CULLING_DATA_UAV_SLOT );
    #include "dafx_shaders.hlsli"
  }
  compile( "cs_5_0", "dafx_culling_discard" );
endmacro


macro DAFX_SCREEN_POS_TO_TC()
  (ps) { dafx_screen_pos_to_tc@f4 = dafx_screen_pos_to_tc; }
  hlsl(ps)
  {
    float2 get_screen_tc(float2 pixel_pos)
    {
      return dafx_screen_pos_to_tc.xy * pixel_pos + dafx_screen_pos_to_tc.zw;
    }
  }
endmacro
