

[numthreads(MEDIA_WARP_SIZE_X, MEDIA_WARP_SIZE_Y, MEDIA_WARP_SIZE_Z)]
void node_based_volumetric_fog_cs(uint3 dtId : SV_DispatchThreadID)
{
  BRANCH
  if (is_voxel_id_occluded(dtId))
  {
    volfog_ff_initial_media[dtId] = 0;
    return;
  }
  MRTOutput result;
  // [[mrt_outputs_init]]

  float4 inv_resolution = float4(inv_volfog_froxel_volume_res.xyz, volfog_froxel_range_params.x);
  float3 screenTcJittered = computeScreenTc(dtId, inv_resolution);
  float3 world_pos = computeworld_pos(screenTcJittered, inv_resolution);

  //[[shader_code]]

  apply_volfog_modifiers(result.col0, screenTcJittered, world_pos);

  result.col0 = (result.col0 < 1.0e10 && result.col0 >= 0.0f) ? result.col0 : 0;

#if DEBUG_DISTANT_FOG_MEDIA_EXTRA_MUL
  result.col0 *= volfog_media_fog_input_mul;
#endif
  volfog_ff_initial_media[dtId] = result.col0;
}
