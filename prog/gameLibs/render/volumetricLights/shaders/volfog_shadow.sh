include "sky_shader_global.sh"
include "heightmap_common.sh"
include "volume_light_common.sh"
include "volume_light_hardcoded_media.sh"


texture prev_volfog_shadow;

texture volfog_shadow;
int volfog_shadow_no = 7;

float4 volfog_shadow_res = (1,1,1,1);
float4 volfog_shadow_prev_frame_tc_offset = (0,0,0,0.9); // .w is accumulation factor

shader volfog_shadow_cs
{
  INIT_JITTER_PARAMS(cs)
  INIT_AND_USE_VOLFOG_MODIFIERS(cs)
  GET_MEDIA_HARDCODED(cs)
  VOLUME_DEPTH_CONVERSION(cs)
  ENABLE_ASSERT(cs)

  (cs) {
    world_view_pos@f4 = (world_view_pos.x, world_view_pos.y, world_view_pos.z, time_phase(0,0));
    from_sun_direction@f3 = from_sun_direction;

    prev_volfog_shadow@smp3d = prev_volfog_shadow;
    volfog_shadow_res@f4 = volfog_shadow_res;
    volfog_shadow_prev_frame_tc_offset@f4 = volfog_shadow_prev_frame_tc_offset;

    volfog_shadow@uav: register(volfog_shadow_no) hlsl {
      RWTexture3D<float4> volfog_shadow@uav;
    }
  }

  hlsl(cs) {
    #include "volfog_shadow.hlsl"

    [numthreads( MEDIA_WARP_SIZE_X, MEDIA_WARP_SIZE_Y, MEDIA_WARP_SIZE_Z )]
    void fill_volfog_shadow_cs( uint3 dtId : SV_DispatchThreadID )
    {
      uint3 res = (uint3)volfog_shadow_res;
      if (any(dtId >= res))
        return;

      volfog_shadow[dtId] = calc_final_volfog_shadow(dtId);
    }
  }
  compile("target_cs", "fill_volfog_shadow_cs");
}

shader clear_volfog_shadow_cs
{
  ENABLE_ASSERT(cs)

  (cs) {
    volfog_shadow_res@f4 = volfog_shadow_res;

    volfog_shadow@uav: register(volfog_shadow_no) hlsl {
      RWTexture3D<float4> volfog_shadow@uav;
    }
  }

  hlsl(cs) {
    [numthreads( MEDIA_WARP_SIZE_X, MEDIA_WARP_SIZE_Y, MEDIA_WARP_SIZE_Z )]
    void clear_volfog_shadow_cs( uint3 dtId : SV_DispatchThreadID )
    {
      uint3 res = (uint3)volfog_shadow_res;
      if (any(dtId >= res))
        return;

      volfog_shadow[dtId] = 1;
    }
  }
  compile("target_cs", "clear_volfog_shadow_cs");
}
