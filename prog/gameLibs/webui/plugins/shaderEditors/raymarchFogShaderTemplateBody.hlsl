

[numthreads(RAYMARCH_WARP_SIZE, RAYMARCH_WARP_SIZE, 1)]
void node_based_volumetric_fog_cs(uint2 dtId : SV_DispatchThreadID, uint2 tid : SV_GroupThreadID)
{
  DistantFogRaymarchResult distantFogRaymarchResult = accumulateFog(dtId, tid);

  if (any(dtId >= (uint2)distant_fog_raymarch_resolution.xy))
    return;

  OutputInscatter[dtId] = distantFogRaymarchResult.inscatter;
  OutputExtinction[dtId] = distantFogRaymarchResult.extinction;
  OutputDist[dtId] = distantFogRaymarchResult.weightedEndDist;
  OutputRaymarchStartWeights[dtId] = float2(distantFogRaymarchResult.fogStart, distantFogRaymarchResult.raymarchStepWeight);

#if DEBUG_DISTANT_FOG_RAYMARCH
  OutputDebug[dtId] = distantFogRaymarchResult.debug;
#endif
}
