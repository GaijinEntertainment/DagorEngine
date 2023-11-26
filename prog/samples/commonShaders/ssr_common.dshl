macro SETUP_SSR(code)
endmacro

texture prev_frame_tex;

float4 ssr_target_size=(1280/2, 720/2,0,0);
float4 ssr_frameNo = (0,0,0,0);
float4 ssr_world_view_pos;

float4 SSRParams = (0.91, -2.5, 0,0);

float4 globtm_no_ofs_psf_0;
float4 globtm_no_ofs_psf_1;
float4 globtm_no_ofs_psf_2;
float4 globtm_no_ofs_psf_3;
float4 prev_globtm_no_ofs_psf_0;
float4 prev_globtm_no_ofs_psf_1;
float4 prev_globtm_no_ofs_psf_2;
float4 prev_globtm_no_ofs_psf_3;

texture downsampled_close_depth_tex;
int downsampled_depth_mip_count;
float4 lowres_rt_params = (1280, 720, 0, 0);

macro SSR_CALCULATE(code)
  hlsl(code) {
    #define linearSmoothnessToLinearRoughness(_param) (1.0f - _param)
    #include "ssr_common.hlsl"
  }
endmacro