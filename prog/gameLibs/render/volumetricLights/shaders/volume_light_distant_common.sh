include "volume_light_common.sh"


int fog_raymarch_frame_id = 0;

macro USE_RAYMARCH_FRAME_INDEX(code)
  (code)
  {
    fog_raymarch_frame_id@i1 = (fog_raymarch_frame_id);
  }
  hlsl(code)
  {
    #include "volume_lights_distant_common.hlsl"

    uint2 calc_raymarch_offset(uint2 raymarch_id)
    {
      return calc_raymarch_offset(raymarch_id, fog_raymarch_frame_id);
    }
    uint2 calc_reconstruction_id(uint2 raymarch_id)
    {
      return calc_reconstruction_id(raymarch_id, fog_raymarch_frame_id);
    }

  }
endmacro



texture prev_downsampled_far_depth_tex;

texture downsampled_close_depth_tex;

macro INIT_HALF_RES_DEPTH(stage)
  (stage)
  {
    distant_fog_half_res_depth_tex@smp2d = distant_fog_downsampled_depth_tex;
    distant_fog_half_res_depth_tex_far@smp2d = downsampled_far_depth_tex;
    distant_fog_half_res_depth_tex_far_prev@smp2d = prev_downsampled_far_depth_tex;
    distant_fog_half_res_depth_tex_close@smp2d = downsampled_close_depth_tex;
  }
endmacro


