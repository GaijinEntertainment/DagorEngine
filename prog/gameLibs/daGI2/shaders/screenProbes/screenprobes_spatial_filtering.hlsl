#include <fast_shader_trig.hlsl>

void add_spatial_radiance(uint2 atlas_probe_coord, float3 probeCamPos, float3 camPos, float3 rayDir, float weight, uint2 octCoord, inout float3 radiance_distance, inout float cWeight, float normalizedNeighboorW)
{
  float4 neighboorRadiance = read_texture_radiance(atlas_probe_coord.x + atlas_probe_coord.y*screenspace_probe_res.x, atlas_probe_coord*sp_getRadianceRes(), octCoord);
  float cW;
  #if SP_USE_ANGLE_FILTERING
  {
    float hitDist = neighboorRadiance.w;
    float3 neighborHitPos = probeCamPos + rayDir * hitDist;
    float3 toNeighbor = neighborHitPos - camPos;
    float toNeighborLen = length(toNeighbor);
    float neighborAngle = acosFast(dot(toNeighbor*(1./toNeighborLen), rayDir));
    float invSpatialAngleWeight = 1./PI;
    float angleWeight = toNeighborLen > 1e-6f ? 1.0f - saturate(neighborAngle * invSpatialAngleWeight) : 1.0f;
    cW = weight*angleWeight;
  }
  #endif
    cW = weight;
  cWeight += cW;
  #if SP_SPATIAL_FAVOR_DARK
  neighboorRadiance.xyz = sqrt(neighboorRadiance.xyz); // favor dark
  #endif
  radiance_distance.xyz += cW*neighboorRadiance.xyz;
}
#include <sp_def_precision.hlsl>
bool add_neighboor_probe(float4 scenePlaneScaled, float invNormalizedW, float3 camPos, float3 rayDir, uint2 octCoord, DecodedProbe probe, float3 probeCamPos, uint2 atlas_probe_coord, inout float3 radiance_distance, inout float cWeight, float wMul, float depth_exp, bool usePositionAveraging)
{
  float relativeDepthDifference = pow2(dot(float4(probeCamPos, -1), scenePlaneScaled));
  depth_exp *= depth_exp_precision_scale(probe.normalizedW*sp_zn_zfar.y, sp_zn_zfar.y);
  float depthWeight = exp2(depth_exp * relativeDepthDifference);
  if (usePositionAveraging)
  {
    depth_exp/=8;
    depthWeight = exp2(depth_exp * (4*relativeDepthDifference+pow2(1 - probe.normalizedW*invNormalizedW)));
  }
  if (depthWeight < 1e-2)
    return false;
  add_spatial_radiance(atlas_probe_coord, probeCamPos, camPos, rayDir, depthWeight*wMul, octCoord, radiance_distance, cWeight, probe.normalizedW);
  return true;
}

struct ScreenProbeFilteringSettings
{
  bool searchAdditionalProbes;
  bool usePositionAveraging;
  float depth_exp;
};

ScreenProbeFilteringSettings default_screen_probes_filtering_settings()
{
  ScreenProbeFilteringSettings s;
  s.searchAdditionalProbes = false;// it doesn't make much sense to search additional probes (makes filtering ~25% slower, and doesn't contribute much)
  s.usePositionAveraging = true;
  s.depth_exp = SP_DEFAULT_BILINEAR_DEPTH_EXP/2;
  return s;
}

void get_neighboor_radiance(uint cur_atlas_probe_index, float4 scenePlaneScaled, float invNormalizedW, float3 camPos, float3 rayDir, int2 tile_coord, uint2 octCoord, inout float3 radiance_distance, inout float cWeight, float wMul, ScreenProbeFilteringSettings s)
{
  uint screenProbeIndex = tile_coord.x + tile_coord.y*screenspace_probe_res.x;
  if (any(uint2(tile_coord) >= uint2(screenspace_probe_res.xy)))
    return;
  uint encodedProbe = sp_loadEncodedProbe(screenspace_probe_pos, screenProbeIndex);
  if (!encodedProbe)
    return;
  DecodedProbe baseProbe = sp_decodeProbeInfo(encodedProbe);

  float3 probeCamPos = baseProbe.normalizedW*sp_getViewVecOptimizedNormalized(getScreenProbeCenterScreenUV(tile_coord));
  if (add_neighboor_probe(scenePlaneScaled, invNormalizedW, camPos, rayDir, octCoord, baseProbe, probeCamPos, tile_coord, radiance_distance, cWeight, wMul, s.depth_exp, s.usePositionAveraging))
  {
  }
  else if (s.searchAdditionalProbes && sp_has_additinal_probes(baseProbe.allTag))
  {
    uint addCount, addAt;
    getAdditionalScreenProbesCount(screenspace_probe_pos, screenProbeIndex, addCount, addAt);
    for (uint addI = addAt, addE = addAt + addCount; addI != addE; ++addI)
    {
      uint addProbeIndex = addI + sp_getNumScreenProbes();
      DecodedProbe addProbe = getScreenProbeUnsafe(screenspace_probe_pos, addProbeIndex);
      float3 probeCamPos = addProbe.normalizedW*sp_getViewVecOptimizedNormalized(getScreenProbeCenterScreenUV(tile_coord));
      add_neighboor_probe(scenePlaneScaled, invNormalizedW, camPos, rayDir, octCoord, addProbe, probeCamPos, uint2(addProbeIndex%uint(screenspace_probe_res.x), addProbeIndex/uint(screenspace_probe_res.x)), radiance_distance, cWeight, wMul, s.depth_exp, s.usePositionAveraging);
    }
  }
}
