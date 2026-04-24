/////////////////////////////////////////////////////////
//  Envi Cover Shader
/////////////////////////////////////////////////////////

// [[shader_local_functions]]

[numthreads(ENVI_COVER_THREADS_X, ENVI_COVER_THREADS_Y, ENVI_COVER_THREADS_Z)]
void node_based_envi_cover_cs(uint2 dtId : SV_DispatchThreadID)
{
  float2 screenCoordCenter = dtId + 0.5;
  float2 tc = screenCoordCenter * rendering_res.zw;

  DISCARD_IF_INVALID_VIEW_AREA_CS(tc);

  NBSGbuffer initialNBSGbuffer = init_NBSGbuffer(dtId);
  //Need to save initial version for reads in permutation
  NBSGbuffer nbsGbuffer = initialNBSGbuffer;

  float rawDepth = texelFetch(depth_gbuf, dtId, 0).r;

  float w = linearize_z(rawDepth, get_transformed_zn_zfar().zw);

  float2 viewVecTc = saturate(tc);
  float3 viewVec = lerp_view_vec(viewVecTc);

  float3 pointToEye = -viewVec * w;
  float4 world_pos = float4(world_view_pos.xyz - pointToEye, 1);

  //[[shader_code]]

  writeNBSGbuffer(nbsGbuffer, dtId);
}
