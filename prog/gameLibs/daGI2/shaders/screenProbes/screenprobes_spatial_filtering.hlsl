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
  #else
    cW = weight;
  #endif
  cWeight += cW;
  #if SP_SPATIAL_FAVOR_DARK
  neighboorRadiance.xyz = sqrt(neighboorRadiance.xyz); // favor dark
  #endif
  radiance_distance.xyz += cW*neighboorRadiance.xyz;
}
#include <sp_def_precision.hlsl>
float sp_neighboor_probe_weight(float4 scene_plane_scaled, float inv_normalized_w, DecodedProbe probe, float3 probe_cam_pos,
  float depth_exp, bool use_position_averaging)
{
  float relativeDepthDifference = pow2(dot(float4(probe_cam_pos, -1), scene_plane_scaled));
  depth_exp *= depth_exp_precision_scale(probe.normalizedW*sp_zn_zfar.y, sp_zn_zfar.y);
  float depthWeight = exp2(depth_exp * relativeDepthDifference);
  if (use_position_averaging)
  {
    depth_exp/=8;
    depthWeight = exp2(depth_exp * (4*relativeDepthDifference+pow2(1 - probe.normalizedW*inv_normalized_w)));
  }
  return depthWeight;
}

struct ScreenProbeFilteringSettings
{
  bool usePositionAveraging;
  float depth_exp;
};

ScreenProbeFilteringSettings default_screen_probes_filtering_settings()
{
  ScreenProbeFilteringSettings s;
  s.usePositionAveraging = true;
  s.depth_exp = SP_DEFAULT_BILINEAR_DEPTH_EXP/2;
  return s;
}

// A group filters contiguous texels of at most SP_FILTER_PROBES_PER_GROUP probes,
// and each probe's neighborhood is texel-invariant, so probe and neighbor
// positions are loaded and decoded once per group instead of per texel (the
// per-texel loads made the filter load-bound).
#define SP_FILTER_NEIGHBORS 12
#if SP_RADIANCE_SIZE_IS_GROUP
#define SP_FILTER_PROBES_PER_GROUP 1
#else
#define SP_FILTER_PROBES_PER_GROUP (SP_FILTER_GROUP_SIZE/16 + 1) // 16 = smallest radiance area (oct res 4)
#endif
// near cross always contributes; diagonal and far cross only for moving/new probes
static const int2 sp_filter_nb_ofs[SP_FILTER_NEIGHBORS] = {
  int2(0,-1), int2(0,1), int2(-1,0), int2(1,0),
  int2(-1,-1), int2(1,-1), int2(-1,1), int2(1,1),
  int2(0,-2), int2(0,2), int2(-2,0), int2(2,0)};

groupshared float4 sp_filter_probe_pos_w[SP_FILTER_PROBES_PER_GROUP]; // xyz camPos, w normalizedW; w <= 0 - invalid
groupshared uint2 sp_filter_probe_tile_tag[SP_FILTER_PROBES_PER_GROUP]; // x tile coord 16:16, y allTag
groupshared float4 sp_filter_nb_pos_w[SP_FILTER_PROBES_PER_GROUP*SP_FILTER_NEIGHBORS];

// contains group barriers: every thread of the group must reach it.
// probe_slots is the group's exact probe span: the sizing bound above includes
// a slack slot that belongs to the next group and must not be loaded twice
void sp_filter_fill_group_cache(uint probe_base, uint probe_slots, uint tid, uint placement_new_ofs)
{
  if (tid < probe_slots)
  {
    uint probeIndex = probe_base + tid;
    float4 posW = 0;
    uint2 tileTag = 0;
    if (probeIndex < sp_getNumTotalProbes())
    {
      uint encodedProbe = sp_loadEncodedProbe(screenspace_probe_pos, probeIndex);
      if (encodedProbe)
      {
        DecodedProbe probe = sp_decodeProbeInfo(encodedProbe);
        uint2 tile = uint2(probeIndex%uint(screenspace_probe_res.x), probeIndex/uint(screenspace_probe_res.x));
        if (probeIndex >= sp_getNumScreenProbes())
        {
          uint screenProbeIndex = loadBuffer(screenspace_tile_classificator, (sp_getScreenTileClassificatorOffsetDwords() + probeIndex - sp_getNumScreenProbes())*4);
          tile = uint2(screenProbeIndex%uint(screenspace_probe_res.x), screenProbeIndex/uint(screenspace_probe_res.x));
        }
        uint encodedProbeNormal = sp_loadEncodedProbeNormalCoord(screenspace_probe_pos, probeIndex, sp_getNumTotalProbes());
        float3 camPos = probe.normalizedW*sp_getViewVecOptimizedNormalized(getScreenProbeAnchorScreenUV(tile, decodeCoordOfs(encodedProbeNormal)));
        posW = float4(camPos, probe.normalizedW);
        tileTag = uint2(tile.x|(tile.y<<16), probe.allTag);
      }
    }
    sp_filter_probe_pos_w[tid] = posW;
    sp_filter_probe_tile_tag[tid] = tileTag;
  }
  GroupMemoryBarrierWithGroupSync();
  LOOP
  for (uint i = tid; i < probe_slots*SP_FILTER_NEIGHBORS; i += SP_FILTER_GROUP_X*SP_FILTER_GROUP_Y)
  {
    uint slot = i/SP_FILTER_NEIGHBORS;
    uint nb = i - slot*SP_FILTER_NEIGHBORS;
    float4 posW = 0;
    uint2 tileTag = sp_filter_probe_tile_tag[slot];
    // static probes consume only the near cross (entries 0-3): mirror the
    // filter's moving/new gate so the diagonal and far entries are not loaded
    if (sp_filter_probe_pos_w[slot].w > 0 && (nb < 4 || sp_is_moving_or_new(tileTag.y)))
    {
      int ofs = sp_is_relatively_new(tileTag.y) ? int(placement_new_ofs) : 1;
      int2 tileCoord = int2(tileTag.x&0xFFFF, tileTag.x>>16) + sp_filter_nb_ofs[nb]*ofs;
      if (all(uint2(tileCoord) < uint2(screenspace_probe_res.xy)))
      {
        uint screenProbeIndex = uint(tileCoord.x) + uint(tileCoord.y)*uint(screenspace_probe_res.x);
        uint encodedProbe = sp_loadEncodedProbe(screenspace_probe_pos, screenProbeIndex);
        if (encodedProbe)
        {
          DecodedProbe nbProbe = sp_decodeProbeInfo(encodedProbe);
          uint2 nbCoordOfs = decodeCoordOfs(sp_loadEncodedProbeNormalCoord(screenspace_probe_pos, screenProbeIndex, sp_getNumTotalProbes()));
          posW = float4(nbProbe.normalizedW*sp_getViewVecOptimizedNormalized(getScreenProbeAnchorScreenUV(uint2(tileCoord), nbCoordOfs)), nbProbe.normalizedW);
        }
      }
    }
    sp_filter_nb_pos_w[i] = posW;
  }
  GroupMemoryBarrierWithGroupSync();
}

void sp_filter_add_cached_neighboor(uint cache_slot, uint nb, int2 nb_tile_coord, float4 scenePlaneScaled, float invNormalizedW, float3 camPos, float3 rayDir, uint2 octCoord, inout float3 radiance_distance, inout float cWeight, ScreenProbeFilteringSettings s)
{
  float4 nbPosW = sp_filter_nb_pos_w[cache_slot*SP_FILTER_NEIGHBORS + nb];
  BRANCH
  if (nbPosW.w <= 0)
    return;
  DecodedProbe nbProbe;
  nbProbe.normalizedW = nbPosW.w;
  nbProbe.allTag = 0;
  float depthWeight = sp_neighboor_probe_weight(scenePlaneScaled, invNormalizedW, nbProbe, nbPosW.xyz, s.depth_exp, s.usePositionAveraging);
  if (depthWeight < 1e-2)
    return;
  add_spatial_radiance(uint2(nb_tile_coord), nbPosW.xyz, camPos, rayDir, depthWeight, octCoord, radiance_distance, cWeight, nbPosW.w);
}
