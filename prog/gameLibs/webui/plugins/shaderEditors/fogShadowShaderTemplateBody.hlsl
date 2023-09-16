
[numthreads( MEDIA_WARP_SIZE_X, MEDIA_WARP_SIZE_Y, MEDIA_WARP_SIZE_Z )]
void node_based_volumetric_fog_cs( uint3 dtId : SV_DispatchThreadID )
{
  uint3 res = (uint3)volfog_shadow_res;
  if (any(dtId >= res))
    return;

  volfog_shadow[dtId] = calc_final_volfog_shadow(dtId);
}
