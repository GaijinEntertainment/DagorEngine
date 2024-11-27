#ifndef SP_UPSAMPLE_WEIGHTS_INCLUDED
#define SP_UPSAMPLE_WEIGHTS_INCLUDED 1
#include <sp_calc_common.hlsl>
#define SP_INVALID_ADDITIONAL_PROBE_NEVER (SP_MAX_ADDITIONAL_PROBES_MASC<<SP_ADDITIONAL_ITERATION_INDEX_SHIFT)

struct UpsampleCornerWeights
{
  float4 sampleBilWeights, relDepth, depthWeights, normalizedLinearDepth;
  float4 cornerDepthBilWeights;
  uint4 encodedProbes;
  uint2 atlasSampleProbeCoord[4];
};
struct UpsamplePointInfo
{
  float3 camPos, worldNormal;
  float sceneLinearDepth;
  float2 screenCoord;
  float bilinearExpand;
};
struct ViewInfo
{
  float3 lt, hor, ver, view_z;
  float4 zn_zfar;
  int4 screenspace_probe_res;
  float4 tile_to_center;
  float2 screenspace_probe_screen_coord_to_probe_coord;
  float2 screen_limit;
  uint4 screenspace_probes_count__added__total;
};
struct SRVBufferInfo
{
  ByteAddressBuffer posBuffer;
};
struct UAVBufferInfo
{
  RWByteAddressBuffer posBuffer;
};

ViewInfo sp_getScreenViewInfo()
{
  ViewInfo vInfo;
  vInfo.lt = sp_view_vecLTNormalized;
  vInfo.hor = sp_view_vecRT_minus_view_vecLTNormalized;
  vInfo.ver = sp_view_vecLB_minus_view_vecLTNormalized;
  vInfo.screenspace_probe_res = screenspace_probe_res;
  vInfo.zn_zfar = sp_zn_zfar;
  vInfo.view_z = sp_view_z;
  vInfo.tile_to_center = sp_tile_center_to_screen_uv;
  vInfo.screen_limit = screenspace_probe_screen_limit.xy;
  vInfo.screenspace_probe_screen_coord_to_probe_coord = screenspace_probe_screen_coord_to_probe_coord;
  vInfo.screenspace_probes_count__added__total = screenspace_probes_count__added__total;
  return vInfo;
}

bool processAdditionalProbe(DecodedProbe additionalProbe, ViewInfo vInfo, uint2 tile_coord, float sceneLinearDepth, float4 scenePlane, float depth_exp, inout UpsampleCornerWeights corners, uint i)
{
  float2 uv = getScreenProbeCenterScreenUV(tile_coord, vInfo.tile_to_center, vInfo.screen_limit);
  //uv = screenICoordToScreenUV(tile_coord*screenspace_probe_res.z + additionalProbe.coord_ofs);
  float3 otherProbeCamPos = additionalProbe.normalizedW*getViewVecFromTc(uv, vInfo.lt, vInfo.hor, vInfo.ver);
  float newRelativeDepthDifference = pow2(dot(float4(otherProbeCamPos, -1), scenePlane));
  newRelativeDepthDifference += sp_get_additional_rel_depth_sample(additionalProbe.normalizedW*vInfo.zn_zfar.y, sceneLinearDepth, depth_exp);
  float newDepthWeight = exp2(depth_exp*newRelativeDepthDifference);
  if (newDepthWeight <= corners.depthWeights[i])
    return false;
  corners.relDepth[i] = newRelativeDepthDifference;
  corners.depthWeights[i] = newDepthWeight;
  corners.normalizedLinearDepth[i] = additionalProbe.normalizedW;
  return true;
}
#define UseBufferInfo SRVBufferInfo
#include "sp_upsample_weights_inc.hlsl"
#undef UseBufferInfo

#define UseBufferInfo UAVBufferInfo
#include "sp_upsample_weights_inc.hlsl"
#undef UseBufferInfo

UpsampleCornerWeights calc_upsample_weights(SRVBufferInfo srvInfo, ViewInfo vInfo, UpsamplePointInfo pointInfo, float depth_exp = -100, bool processAdditional = true)
{
  float2 probeCoord2DF = pointInfo.screenCoord.xy*vInfo.screenspace_probe_screen_coord_to_probe_coord.x + vInfo.screenspace_probe_screen_coord_to_probe_coord.y;
  uint2 probeCoord2D = uint2(clamp(probeCoord2DF, 0, vInfo.screenspace_probe_res.xy - 2));
  //uint2 probeCoord2D = uint2(max(probeCoord2DF, 0));
  float2 probeCoordFract = saturate(probeCoord2DF - float2(probeCoord2D));
  //bilinear weights enhanced a bit, to avoid being 0
  if (pointInfo.bilinearExpand)//fixme: can be made two madd
    probeCoordFract = saturate((pointInfo.screenCoord.xy - ((probeCoord2D+0.5)*vInfo.screenspace_probe_res.z+0.5) + pointInfo.bilinearExpand)/(vInfo.screenspace_probe_res.z + 2*pointInfo.bilinearExpand));

  float4 bilWeights = max(1e-3, float4(1-probeCoordFract, probeCoordFract));
  float4 scenePlane = float4(pointInfo.worldNormal, dot(pointInfo.camPos, pointInfo.worldNormal)) / pointInfo.sceneLinearDepth;
  #if DEPTH_ONLY_WEIGHTENING
  scenePlane = -float4(vInfo.view_z, dot(pointInfo.camPos, vInfo.view_z)) / pointInfo.sceneLinearDepth;
  #endif

  UpsampleCornerWeights corners = (UpsampleCornerWeights)0;
  corners.sampleBilWeights = float4(bilWeights.x*bilWeights.y, bilWeights.z*bilWeights.y, bilWeights.x*bilWeights.w, bilWeights.z*bilWeights.w);
  corners.depthWeights = corners.normalizedLinearDepth = 0;
  //scenePlane *= depth_exp_precision_scale(pointInfo.sceneLinearDepth/vInfo.zn_zfar.y);
  depth_exp *= depth_exp_precision_scale(pointInfo.sceneLinearDepth, vInfo.zn_zfar.y);
  corners.relDepth = 1e6;

  uint4 encodedScreenProbes = load_encoded_screen_probes(srvInfo, probeCoord2D, vInfo.screenspace_probe_res.x);

  UNROLL
  for (uint i = 0; i < 4; ++i)
  {
    uint2 coordOffset = uint2(i&1, (i&2)>>1);
    uint2 sampleProbeCoord = coordOffset + probeCoord2D;
    uint2 atlasSampleProbeCoord = sampleProbeCoord;
    uint screenProbeIndex = sampleProbeCoord.x + vInfo.screenspace_probe_res.x*sampleProbeCoord.y;
    uint encodedProbe = encodedScreenProbes[i];
    FLATTEN
    if (!encodedProbe)
      corners.sampleBilWeights[i] = 0;
    else
    {
      DecodedProbe baseProbe = sp_decodeProbeInfo(encodedProbe);
      float3 probeCamPos = baseProbe.normalizedW*getViewVecFromTc(getScreenProbeCenterScreenUV(sampleProbeCoord, vInfo.tile_to_center, vInfo.screen_limit), vInfo.lt, vInfo.hor, vInfo.ver);
      corners.relDepth[i] = pow2(dot(float4(probeCamPos, -1), scenePlane));
      corners.relDepth[i] += sp_get_additional_rel_depth_sample(baseProbe.normalizedW*vInfo.zn_zfar.y, pointInfo.sceneLinearDepth, depth_exp);
      corners.depthWeights[i] = exp2(depth_exp*corners.relDepth[i]);
      corners.normalizedLinearDepth[i] = baseProbe.normalizedW;
      corners.atlasSampleProbeCoord[i] = atlasSampleProbeCoord;
      corners.encodedProbes[i] = encodedProbe;

      if (processAdditional && sp_has_additinal_probes(baseProbe.allTag))//we can also use it as count
      {
        //this is faster for bilinear filter approach, but requires re-processing added probes
        uint addCount, addAt;
        getAdditionalScreenProbesCount(srvInfo.posBuffer, screenProbeIndex, addCount, addAt);
        LOOP
        for (uint addI = addAt, addE = addAt + addCount; addI != addE; ++addI )
        {
          uint addProbeIndex = vInfo.screenspace_probes_count__added__total.x + addI;
          uint additionalEncodedProbe = sp_loadEncodedProbe(srvInfo.posBuffer, addProbeIndex);
          DecodedProbe additionalProbe = sp_decodeProbeInfo(additionalEncodedProbe);

          if (processAdditionalProbe(additionalProbe, vInfo, sampleProbeCoord, pointInfo.sceneLinearDepth, scenePlane, depth_exp, corners, i))
          {
            corners.encodedProbes[i] = additionalEncodedProbe;
            corners.atlasSampleProbeCoord[i] = uint2(addProbeIndex%uint(vInfo.screenspace_probe_res.x), addProbeIndex/uint(vInfo.screenspace_probe_res.x));
          }
        }
      }
    }
  }

  corners.cornerDepthBilWeights = max(1e-5, corners.depthWeights)*corners.sampleBilWeights;
  return corners;
}

#endif