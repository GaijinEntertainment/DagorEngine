
tc = screen_pos_to_tc(screenpos.xy);
w = linearize_z(readGbufferDepth(tc), zn_zfar.zw);//both faster and better quality
float3 viewVect = lerp(lerp(view_vecLT, view_vecRT, tc.x), lerp(view_vecLB, view_vecRB, tc.x), tc.y);
float3 pointToEye = -viewVect * w;
float4 worldPos = float4(world_view_pos.xyz - pointToEye, 1);
float4 pos_and_radius = input.pos_and_radius;
#if OMNI_SHADOWS
float4 shadowTcToAtlas = input.shadow_tc_to_atlas;
#else
float4 shadowTcToAtlas = float4(0, 0, 0, 0);
#endif
float3 moveFromPos = pos_and_radius.xyz-worldPos.xyz;
view = 0;
dist = 0;

bool shouldExit = dot(moveFromPos, moveFromPos) > pos_and_radius.w*pos_and_radius.w;
#if WAVE_INTRINSICS
  shouldExit = (bool)WaveReadFirstLane(WaveAllBitAnd(uint(shouldExit)));
#endif
BRANCH
if (shouldExit)
  return false;//discard; //discard is faster, but also fails early depth

ProcessedGbuffer gbuffer = readProcessedGbuffer(tc);

float distSq = dot(pointToEye,pointToEye);
float invRsqrt = rsqrt(distSq);
view  = pointToEye*invRsqrt;
dist = rcp(invRsqrt);
float NdotV = dot(gbuffer.normal, view);
float3 reflectionVec = 2 * NdotV * gbuffer.normal - view;
float NoV = abs( NdotV ) + 1e-5;

half dynamicLightsSpecularStrength = gbuffer.extracted_albedo_ao;
half ssao = 1;//fixme: we should use SSAO here!
half enviAO = gbuffer.ao*ssao;//we still modulate by albedo color, so we don't need micro AO
half pointLightsFinalAO = (enviAO*0.5+0.5);
half specularAOcclusion = computeSpecOcclusion(saturate(NdotV), enviAO, gbuffer.linearRoughness*gbuffer.linearRoughness);// dice spec occlusion
half3 specularColor = gbuffer.specularColor*(specularAOcclusion*gbuffer.extracted_albedo_ao);
