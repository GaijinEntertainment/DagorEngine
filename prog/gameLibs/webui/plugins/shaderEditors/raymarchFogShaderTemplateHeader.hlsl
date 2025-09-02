
/////////////////////////////////////////////////////////
//  Fog Shader
/////////////////////////////////////////////////////////
// [[mrt_outputs_decl]]

// [[shader_local_functions]]

// register must match the dshl definitions
RWTexture2D<float4>  OutputInscatter : register(u6); // volfog_df_raymarch_inscatter_const_no // .a is 2 bits == raymarching pattern offset
RWTexture2D<float>  OutputExtinction : register(u5); // volfog_df_raymarch_extinction_const_no
RWTexture2D<float>  OutputDist : register(u4); // volfog_df_raymarch_dist_const_no

RWTexture2D<float2>  OutputRaymarchStartWeights : register(u3); // volfog_df_raymarch_start_weights_const_no

#if DEBUG_DISTANT_FOG_RAYMARCH
RWTexture2D<float4>  OutputDebug : register(u2); // volfog_df_raymarch_debug_tex_const_no
#endif



#define distant_fog_half_res_depth_tex downsampled_checkerboard_depth_tex

#define distant_fog_half_res_depth_tex_far downsampled_far_depth_tex
#define distant_fog_half_res_depth_tex_far_prev prev_downsampled_far_depth_tex
#define distant_fog_half_res_depth_tex_close downsampled_close_depth_tex

#define distant_fog_half_res_depth_tex_samplerstate           downsampled_checkerboard_depth_tex_samplerstate
#define distant_fog_half_res_depth_tex_far_samplerstate       downsampled_far_depth_tex_samplerstate
#define distant_fog_half_res_depth_tex_far_prev_samplerstate  prev_downsampled_far_depth_tex_samplerstate
#define distant_fog_half_res_depth_tex_close_samplerstate     downsampled_close_depth_tex_samplerstate

#undef IS_DISTANT_FOG
#define IS_DISTANT_FOG 1

