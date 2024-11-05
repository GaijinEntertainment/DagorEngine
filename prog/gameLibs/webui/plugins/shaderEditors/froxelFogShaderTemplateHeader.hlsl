
/////////////////////////////////////////////////////////
//  Fog Shader
/////////////////////////////////////////////////////////
// [[mrt_outputs_decl]]

// [[shader_local_functions]]

RWTexture3D<float4>  volfog_ff_initial_media : register(u6); // register must match volfog_ff_initial_media_const_no


float volume_pos_to_depth(float v, float4 inv_resolution)
{
  return volume_pos_to_depth(v, inv_resolution.w);
}

float3 computeworld_pos(float3 screenTcJittered, float4 inv_resolution)
{
  float3 view = lerp_view_vec(screenTcJittered.xy);
  float cdepth = volume_pos_to_depth(screenTcJittered.z, inv_resolution.w);
  return cdepth * view + world_view_pos.xyz;
}

float3 computeScreenTc(uint3 dtId, float4 inv_resolution)
{
  return calc_jittered_tc(dtId, (uint)jitter_ray_offset.w, inv_resolution.xyz);
}
