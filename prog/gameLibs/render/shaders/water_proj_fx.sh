include "shader_global.sh"
include "viewVecVS.sh"

texture wfx_prev_frame_tex;
texture wfx_frame_tex;

float4 prev_globtm_0 = (1, 0, 0, 0);
float4 prev_globtm_1 = (0, 1, 0, 0);
float4 prev_globtm_2 = (0, 0, 1, 0);
float4 prev_globtm_3 = (0, 0, 0, 1);

float4 cur_jitter;

shader combined_water_proj_fx
{
  no_ablend;

  (ps) {
    prev_frame_tex@smp2d = wfx_prev_frame_tex;
    frame_tex@smp2d = wfx_frame_tex;
    vpos_wlevel@f4 = (world_view_pos.x, world_view_pos.y, world_view_pos.z, water_level);
    cur_jitter@f2 = cur_jitter;
    prev_globtm@f44 = {prev_globtm_0, prev_globtm_1, prev_globtm_2, prev_globtm_3};
  }

  cull_mode = none;
  z_write = false;
  z_test = false;

  USE_AND_INIT_VIEW_VEC_VS()

  POSTFX_VS_TEXCOORD_VIEWVEC(0, texcoord, viewVect)

  hlsl(ps) {
    float rayCastWaterPlane(float3 ray_pos, float3 ray_dir, float water_y_level)
    {
      float den = dot(ray_dir, float3(0, 1, 0));
      if (den < 0)
        return -(dot(float3(0, 1, 0), ray_pos) - water_y_level) / den;
      return 0;
    }

    float4 combined_water_proj_fx_ps(VsOutput input): SV_Target
    {
      float3 viewVec = normalize(input.viewVect.xyz);
      float worldDist = rayCastWaterPlane(vpos_wlevel.xyz, viewVec.xyz, vpos_wlevel.w);
      float3 worldPos = vpos_wlevel.xyz + viewVec.xyz * worldDist;

      // temporal reprojection first
      float3 prevWorldPos = worldPos;
      float weight = 0.7;
      float defaultBlend = 0.7;
      float2 oldUv = 0;
      {
        float4 prevClip = mul(float4(prevWorldPos, 1), prev_globtm);
        float3 prevScreen = prevClip.w > 0 ? prevClip.xyz * rcp(prevClip.w) : float3(2, 2, 0);
        bool offscreen = max(abs(prevScreen.x), abs(prevScreen.y)) >= 1.0f;
        float invalidateScale = 3;
        oldUv = prevScreen.xy * RT_SCALE_HALF + 0.5;
        weight = offscreen ? 0 : defaultBlend * saturate(1 - dot(invalidateScale.xx, abs(input.texcoord.xy - oldUv.xy)));
      }

      // history sample
      float4 prevTarget = tex2Dlod(prev_frame_tex, float4(oldUv, 0, 0));
      // unjittered frame
      float4 newTarget = tex2Dlod(frame_tex, float4(input.texcoord + cur_jitter.xy * float2(0.5, -0.5), 0, 0));
      // final weighted result with feedback
      return lerp(newTarget, prevTarget, weight);
    }
  }

  compile("target_ps", "combined_water_proj_fx_ps");
}