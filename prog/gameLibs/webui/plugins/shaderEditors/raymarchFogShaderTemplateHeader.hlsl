
/////////////////////////////////////////////////////////
//  Fog Shader
/////////////////////////////////////////////////////////
// [[mrt_outputs_decl]]

// [[shader_local_functions]]

RWTexture2D<float4>  OutputInscatter : register(u7); // .a is 2 bits == raymarching pattern offset
RWTexture2D<float>  OutputExtinction : register(u6);
RWTexture2D<float>  OutputDist : register(u5);

RWTexture2D<float2>  OutputRaymarchStartWeights : register(u4);

#if DEBUG_DISTANT_FOG_RAYMARCH
RWTexture2D<float4>  OutputDebug : register(u3);
#endif



#define distant_fog_half_res_depth_tex distant_fog_downsampled_depth_tex

#define distant_fog_half_res_depth_tex_far downsampled_far_depth_tex
#define distant_fog_half_res_depth_tex_far_prev prev_downsampled_far_depth_tex
#define distant_fog_half_res_depth_tex_close downsampled_close_depth_tex



#undef IS_DISTANT_FOG
#define IS_DISTANT_FOG 1

