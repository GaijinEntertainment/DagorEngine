hlsl(ps) {
  #define COMPLICATED_DOF 1
  #define MAX_COC  4
}

texture dof_near_layer;
texture dof_far_layer;
float4 dof_focus_params;
float4 dof_focus_linear_params;
float4 dof_rt_size;
float4 dlss_jitter_offset;

int dof_focus_near_mode = 0;
interval dof_focus_near_mode: off < 1, on < 2, linear;

int dof_focus_far_mode = 0;
interval dof_focus_far_mode: off < 1, on < 2, linear;

texture dof_blend_tex;
texture dof_blend_depth_tex;

macro USE_DOF_COMPOSITE_NEAR()
  (ps) { dof_near_layer@smp2d = dof_near_layer; }
  hlsl(ps) {
    #define HAS_NEAR_DOF 1
    #include <tex2d_bicubic.hlsl>
    half4 get_dof_near(float2 tc, float2 dof_resolution)
    {
      half4 cLayerNear = tex2D_bicubic(dof_near_layer, dof_near_layer_samplerstate, tc.xy, dof_resolution);
      cLayerNear.rgb = cLayerNear.rgb / (cLayerNear.a > 0 ? cLayerNear.a : 1.0f);

    ##if dof_focus_near_mode == linear
      float depth = tex2Dlod(dof_blend_depth_tex, float4(tc.xy, 0, 0)).x;
      float zVal = linearize_z(depth, zn_zfar.zw);
      cLayerNear.a = 1.0 - saturate(zVal * dof_focus_linear_params.x + dof_focus_linear_params.y);
    ##else
      cLayerNear.a = saturate(cLayerNear.a);
    ##endif

      return cLayerNear;
    }
  }
endmacro

macro USE_DOF_COMPOSITE_FAR()
  (ps) {
    dlss_jitter_offset_screen_size@f4 = (dlss_jitter_offset.x, dlss_jitter_offset.y, rendering_res.z, rendering_res.w);
    dof_far_layer@smp2d = dof_far_layer;
    dof_focus_params@f4 = dof_focus_params;
    dof_blend_tex@smp2d = dof_blend_tex;
  }
  hlsl(ps) {
    #define HAS_FAR_DOF 1
    #include <dof/circleOfConfusion.hlsl>
    #include <tex2d_bicubic.hlsl>
    half4 get_dof_far(float2 tc, float2 dof_resolution)
    {
      half4 cLayerFar = tex2D_bicubic(dof_far_layer, dof_far_layer_samplerstate, tc.xy, dof_resolution);

    ##if dof_focus_far_mode == linear
      float depth = tex2Dlod(dof_blend_depth_tex, float4(tc.xy, 0, 0)).x;
      float zVal = linearize_z(depth, zn_zfar.zw);
      cLayerFar.a = saturate(zVal * dof_focus_linear_params.z + dof_focus_linear_params.w);
    ##else
      #if DOF_FAR_POSTFX_DLSS
        // DLSS requires special handling. It can't upscale and unjitter mask texture for DoF
        // (this is done in current solution for TAA). Trying to put mask to alpha channel
        // of DLSS upscaled target also doesn't result in desired behaviour (mask edges are still jittering).
        // So current solution for DLSS simply mades transition to DoF more blurry and less noticeable.
        // It is based on computing FarCircleOfConfusion for 9 depth values and returning weighted average of results.
        float2 jitteredTc = tc.xy + dlss_jitter_offset_screen_size.xy*dlss_jitter_offset_screen_size.zw;
        cLayerFar.a = ComputeFarCircleOfConfusion(tex2Dlod(dof_blend_depth_tex, float4(jitteredTc, 0, 0)).x, dof_focus_params) * 0.25;
        float4 depth4 = float4(
          tex2Dlod(dof_blend_depth_tex, float4(jitteredTc + float2(-1, -1)*dlss_jitter_offset_screen_size.zw, 0, 0)).x,
          tex2Dlod(dof_blend_depth_tex, float4(jitteredTc + float2(-1, 1)*dlss_jitter_offset_screen_size.zw, 0, 0)).x,
          tex2Dlod(dof_blend_depth_tex, float4(jitteredTc + float2(1, -1)*dlss_jitter_offset_screen_size.zw, 0, 0)).x,
          tex2Dlod(dof_blend_depth_tex, float4(jitteredTc + float2(1, 1)*dlss_jitter_offset_screen_size.zw, 0, 0)).x
        );
        cLayerFar.a += dot(ComputeFarCircleOfConfusion4(depth4, dof_focus_params), float4(0.0625, 0.0625, 0.0625, 0.0625));
        depth4 = float4(
          tex2Dlod(dof_blend_depth_tex, float4(jitteredTc + float2(0, -1)*dlss_jitter_offset_screen_size.zw, 0, 0)).x,
          tex2Dlod(dof_blend_depth_tex, float4(jitteredTc + float2(0, 1)*dlss_jitter_offset_screen_size.zw, 0, 0)).x,
          tex2Dlod(dof_blend_depth_tex, float4(jitteredTc + float2(-1, 0)*dlss_jitter_offset_screen_size.zw, 0, 0)).x,
          tex2Dlod(dof_blend_depth_tex, float4(jitteredTc + float2(1, 0)*dlss_jitter_offset_screen_size.zw, 0, 0)).x
        );
        cLayerFar.a += dot(ComputeFarCircleOfConfusion4(depth4, dof_focus_params), float4(0.125, 0.125, 0.125, 0.125));
      #elif DOF_FAR_POSTFX_TAA
        cLayerFar.a = tex2Dlod(dof_blend_tex, float4(tc.xy, 0, 0)).x;
      #else
        float depth = tex2Dlod(dof_blend_depth_tex, float4(tc.xy, 0, 0)).x;
        float farCoC = ComputeFarCircleOfConfusion(depth, dof_focus_params);
        cLayerFar.a = saturate(min(cLayerFar.a, farCoC.x) * MAX_COC);
      #endif
    ##endif

      return cLayerFar;
    }
  }
endmacro

macro USE_DOF_COMPOSITE_NEAR_FAR()
  (ps) { dof_blend_depth_tex@smp2d = dof_blend_depth_tex; }
  USE_DOF_COMPOSITE_FAR()
  USE_DOF_COMPOSITE_NEAR()
  hlsl(ps) {
    half4 cLayerNear,cLayerFar;
    void get_dof_near_far(float2 tc, float2 dof_resolution, out half4 cLayerNear, out half4 cLayerFar)
    {
      cLayerNear = get_dof_near(tc, dof_resolution);
      cLayerFar = get_dof_far(tc, dof_resolution);
    }
  }
endmacro

macro USE_DOF_COMPOSITE_NEAR_FAR_OPTIONAL()
  (ps) { dof_focus_linear_params@f4 = dof_focus_linear_params; }

  if (dof_focus_near_mode != off || dof_focus_far_mode != off)
  {
    (ps) { dof_blend_depth_tex@smp2d = dof_blend_depth_tex; }
  }
  if (dof_focus_near_mode != off)
  {
    USE_DOF_COMPOSITE_NEAR()
  }
  if (dof_focus_far_mode != off)
  {
    USE_DOF_COMPOSITE_FAR()
  }
endmacro
