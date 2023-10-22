include "hero_matrix_inc.sh"
include "taa_inc.sh"
include "reprojected_motion_vectors.sh"
include "force_ignore_history.sh"

float4 prev_globtm_no_ofs_psf_0;
float4 prev_globtm_no_ofs_psf_1;
float4 prev_globtm_no_ofs_psf_2;
float4 prev_globtm_no_ofs_psf_3;

float4 prev_view_vecLT;
float4 prev_view_vecRT;
float4 prev_view_vecLB;
float4 prev_view_vecRB;
float4 prev_world_view_pos;

texture downsampled_motion_vectors_tex;
texture prev_downsampled_motion_vectors_tex;

float4 move_world_view_pos;

float ssao_reprojection_neighborhood_bias_tight = 0.0;
float ssao_reprojection_neighborhood_bias_wide = 0.05;
float ssao_reprojection_neighborhood_velocity_difference_scale = 100;

macro SSAO_REPROJECTION(stage, prev_tex_name)
(stage) {
  ssao_prev_tex@smp2d = prev_tex_name;
  prev_globtm_no_ofs_psf@f44 = { prev_globtm_no_ofs_psf_0, prev_globtm_no_ofs_psf_1, prev_globtm_no_ofs_psf_2, prev_globtm_no_ofs_psf_3 };
  prev_downsampled_far_depth_tex@smp2d = prev_downsampled_far_depth_tex;
  prev_view_vecLT@f3 = prev_view_vecLT;
  prev_view_vecRT_minus_view_vecLT@f3 = (prev_view_vecRT-prev_view_vecLT);
  prev_view_vecLB_minus_view_vecLT@f3 = (prev_view_vecLB-prev_view_vecLT);
  prev_view_vecRT@f3 = prev_view_vecRT;
  prev_view_vecLB@f3 = prev_view_vecLB;
  prev_view_vecRB@f3 = prev_view_vecRB;
  prev_world_view_pos@f3 = prev_world_view_pos;
  prev_move_world_view_pos@f3= (-move_world_view_pos.x, -move_world_view_pos.y, -move_world_view_pos.z,0);
  neighborhood_params@f3 = (ssao_reprojection_neighborhood_bias_tight,
                            ssao_reprojection_neighborhood_bias_wide,
                            ssao_reprojection_neighborhood_velocity_difference_scale,
                            0);
}


hlsl(stage) {
  #define neighborhood_bias_tight neighborhood_params.x
  #define neighborhood_bias_wide neighborhood_params.y
  #define neighborhood_velocity_difference_scale neighborhood_params.z
}

if (downsampled_motion_vectors_tex != NULL)
{
  (stage) {
    downsampled_motion_vectors_tex@smp2d = downsampled_motion_vectors_tex;
    prev_downsampled_motion_vectors_tex@smp2d = prev_downsampled_motion_vectors_tex;
  }
  INIT_MOTION_VEC_DECODE(stage)
  USE_MOTION_VEC_DECODE(stage)
  bool has_prev_motion_vectors = prev_downsampled_motion_vectors_tex != NULL;
} else if (motion_gbuf != NULL)
{
  (stage) { motion_gbuf@smp2d = motion_gbuf; }
  INIT_MOTION_VEC_DECODE(stage)
  USE_MOTION_VEC_DECODE(stage)
}

INIT_HERO_MATRIX(stage)
USE_HERO_MATRIX(stage)
INIT_REPROJECTED_MOTION_VECTORS(stage)
USE_REPROJECTED_MOTION_VECTORS(stage)
USE_IGNORE_HISTORY(stage)

hlsl(stage) {
##if in_editor_assume == yes
#define FIX_DISCONTINUTIES 0
##else
#define FIX_DISCONTINUTIES 1
#define DIFFERENCE_CLAMPING 1
##endif
}

hlsl(stage) {
  #include <fp16_aware_lerp.hlsl>

  float3 getPrevViewVecOptimized(float2 tc)
  {
    return prev_view_vecLT + prev_view_vecRT_minus_view_vecLT*tc.x + prev_view_vecLB_minus_view_vecLT*tc.y;
  }

  float2 get_motion_vector(float2 uv, float3 eye_to_point)
  {
##if downsampled_motion_vectors_tex != NULL
    return decode_motion_vector(tex2Dlod(downsampled_motion_vectors_tex, float4(uv, 0, 0)).rg);
##else
    float2 ret = get_reprojected_motion_vector(uv, eye_to_point, prev_globtm_no_ofs_psf);
##if motion_gbuf != NULL
    float2 motionVector = tex2Dlod(motion_gbuf, float4(uv, 0, 0)).rg;
    if (motionVector.x != 0)
      ret = decode_motion_vector(motionVector);
##endif
    return ret;
##endif
  }

  void reproject_ssao(inout SSAO_TYPE ssao, float3 cameraToOrigin, float2 screenTC, float linear_depth)
  {
    bool isHero = apply_hero_matrix(cameraToOrigin);

    float defaultBlend = 0.93;

    float2 motionVector = get_motion_vector(screenTC, cameraToOrigin);
    float2 historyUV = screenTC + motionVector;

    SSAO_TYPE historyWeight = 1;

#if FIX_DISCONTINUTIES
    #if READ_W_DEPTH
      float prevDepth1 = tex2Dlod(prev_downsampled_far_depth_tex, float4(historyUV, 0, 0)).x;
    #else
      float prevDepth1 = linearize_z(tex2Dlod(prev_downsampled_far_depth_tex, float4(historyUV, 0, 0)).x, zn_zfar.zw);
    #endif
    float3 prevViewVect = getPrevViewVecOptimized(historyUV);
    float3 depthPrevViewVec = prev_move_world_view_pos + prevViewVect * prevDepth1;//todo: (prev_world_view_pos-world_view_pos) to be passed as constant and calculated in double!!!
    float3 move = depthPrevViewVec - cameraToOrigin;

    float invalidate_move = pow2(8);
    historyWeight = saturate(1 - sqrt(dot(move, move)*invalidate_move)*rcp(linear_depth));
    float prevDepthSame = linearize_z(tex2Dlod(prev_downsampled_far_depth_tex, float4(screenTC.xy, 0, 0)).x, zn_zfar.zw);
    float2 distToScreenEnd = screenTC.xy - float2(0.5, 1);
    if (abs(prevDepthSame - linear_depth) < 0.01 && linear_depth < 0.5 && !isHero && dot(distToScreenEnd, distToScreenEnd) < 0.125)//hands
    {
      historyUV.xy = screenTC.xy;
      historyWeight = 0.9;
    }
#endif

    float prevTcAbs = max(abs(historyUV.x * 2 - 1), abs(historyUV.y * 2 - 1));
    float curTcAbs = max(abs(screenTC.x * 2 - 1), abs(screenTC.y * 2 - 1));
    historyWeight *= prevTcAbs < 0.97 || curTcAbs >= 0.97 ? defaultBlend : 0;

    SSAO_TYPE historySample = tex2Dlod(ssao_prev_tex, float4(historyUV, 0, 0)).SSAO_ATTRS;

#if DIFFERENCE_CLAMPING
    #define VALUE_CLAMP_RANGE 0.5f
    #define VALUE_CLAMP_OFFSET 0.5f
    SSAO_TYPE diff = ssao - historySample;
    SSAO_TYPE valueClampFactor = saturate(diff * (1.0f / VALUE_CLAMP_RANGE) - VALUE_CLAMP_OFFSET);
    historyWeight *= 1.0f - valueClampFactor;
#endif
    ssao = fp16_aware_lerp(historySample, ssao, 1-historyWeight, 1/0.03);
    //ssao = lerp(ssao, historySample, historyWeight);
  }

  float get_disocclusion(float linear_depth, float2 history_uv)
  {
    float historyDepth = tex2Dlod(prev_downsampled_far_depth_tex, float4(history_uv, 0, 0)).x;
    #if !READ_W_DEPTH
      historyDepth = linearize_z(historyDepth, zn_zfar.zw);
    #endif
    return saturate(linear_depth - historyDepth); // Could be scaled but scale of 1.0 seems fine;
  }

  float get_velocity_weight(float2 current_velocity, float2 history_velocity)
  {
    float2 diff = current_velocity - history_velocity;
    return saturate((abs(diff.x) + abs(diff.y)) * neighborhood_velocity_difference_scale);
  }

  void reproject_ssao_with_neighborhood(inout SSAO_TYPE ssao, SSAO_TYPE min_ao, SSAO_TYPE max_ao, float3 eye_to_point, float2 uv, float depth)
  {
    if (force_ignore_history == 1)
      return;
##if downsampled_motion_vectors_tex == NULL && motion_gbuf == NULL
    apply_hero_matrix(eye_to_point);
##endif

    float2 mv = get_motion_vector(uv, eye_to_point);
    float2 historyUV = uv + mv;

    SSAO_TYPE ssaoHistory = tex2Dlod(ssao_prev_tex, float4(historyUV, 0, 0)).SSAO_ATTRS;

    ##if maybe(ssao_contact_shadows) || !maybe(has_prev_motion_vectors)
      float disocclusion = get_disocclusion(depth, historyUV);
    ##endif

    // Reject history for disoccluded pixels, but only for contact shadows
    // - For AO, it produces more unwanted noise than useful anti-ghosting
    // - Contact shadows cause trailing shadow outlines without this
    ##if maybe(ssao_contact_shadows)
      ssaoHistory.CONTACT_SHADOWS_ATTR = lerp(ssaoHistory.CONTACT_SHADOWS_ATTR, ssao.CONTACT_SHADOWS_ATTR, disocclusion);
    ##endif

    // Neighborhood clamp
    {
      ##if maybe(has_prev_motion_vectors)
        // Velocity weighting if motion vectors history is available
        float2 currentVelocity = mv;
        float2 rawHistoryVelocity = tex2Dlod(prev_downsampled_motion_vectors_tex, float4(historyUV, 0, 0)).rg;
        float2 historyVelocity = decode_motion_vector(rawHistoryVelocity);
        float velocityWeight = get_velocity_weight(currentVelocity, historyVelocity);
        float neighborhoodBiasWeight = velocityWeight;
      ##else
        // Use disocclusion for neighborhood tightening if motion vectors history is not available
        float neighborhoodBiasWeight = disocclusion;
      ##endif
      float neighborhoodBias = lerp(neighborhood_bias_wide, neighborhood_bias_tight, neighborhoodBiasWeight);
      min_ao -= neighborhoodBias;
      max_ao += neighborhoodBias;
      ssaoHistory = clamp(ssaoHistory, min_ao, max_ao);
    }

    // Clip off-screen history
    {
      float historyUvAbs = max(abs(historyUV.x * 2 - 1), abs(historyUV.y * 2 - 1));
      FLATTEN if (historyUvAbs > 1)
        ssaoHistory = ssao;
    }

    ssao = fp16_aware_lerp(ssaoHistory, ssao, 1-0.93, 1/0.03);
  }
}
endmacro