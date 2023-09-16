
float3 volfog_shadow_tc_to_world_pos(float3 tc)
{
  tc = tc * 2.0 - 1.0;
  return world_view_pos.xyz + tc * (volfog_shadow_res.w * volfog_shadow_res.xyz);
}

float calc_volfog_shadow(float3 world_pos)
{
  const float3 fakeScreenTc = float3(0.5, 0.5, 1); // center pos, but it shouldn't matter (unless used by the graph)

  const int fixedSampleCnt = 16;
  float fixedStepLen = volfog_shadow_res.w; // can be independent from voxel size, but it's better for jittering

  float densitySum = 0; // fixed step length, we can pull the exp to the outside
  float3 step = -from_sun_direction.xyz * fixedStepLen;
  for (int i = 0; i < fixedSampleCnt; ++i)
  {
    float density = get_media_no_modifiers(world_pos, fakeScreenTc, world_view_pos.w).a;
    densitySum += density;
    world_pos += step;
  }
  return exp(-densitySum * fixedStepLen);
}

float calc_final_volfog_shadow(uint3 dtId)
{
  float3 invRes = 1.0 / volfog_shadow_res.xyz;

  float3 prevTc = (dtId + 0.5) * invRes + volfog_shadow_prev_frame_tc_offset.xyz;
  float prevShadow = tex3Dlod(prev_volfog_shadow, float4(prevTc,0)).x;

  float3 jitteredTc = calc_jittered_tc(dtId, (uint)jitter_ray_offset.w, invRes);
  float3 jitteredWorldPos = volfog_shadow_tc_to_world_pos(jitteredTc);
  float shadow = calc_volfog_shadow(jitteredWorldPos);

  shadow = lerp(shadow, prevShadow, volfog_shadow_prev_frame_tc_offset.w);

  return shadow;
}
