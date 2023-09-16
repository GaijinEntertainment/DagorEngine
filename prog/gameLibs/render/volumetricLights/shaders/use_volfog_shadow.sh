
texture volfog_shadow;
float4 volfog_shadow_res = (1,1,1,1); // .w is voxel size

macro INIT_VOLFOG_SHADOW(code)
  if (volfog_shadow != NULL)
  {
    local float4 volfog_shadow_tc_scale_inv = float4(
      1.0 / (volfog_shadow_res.x * volfog_shadow_res.w),
      1.0 / (volfog_shadow_res.y * volfog_shadow_res.w),
      1.0 / (volfog_shadow_res.z * volfog_shadow_res.w),
      0);
    (code)
    {
      volfog_shadow@smp3d = volfog_shadow;
      volfog_shadow_res@f4 = volfog_shadow_res;
      volfog_shadow_tc_mul@f3 = float4(
        volfog_shadow_tc_scale_inv.x * 0.5,
        volfog_shadow_tc_scale_inv.y * 0.5,
        volfog_shadow_tc_scale_inv.z * 0.5,
        0);
      volfog_shadow_tc_add@f3 = float4(
        (-world_view_pos.x * volfog_shadow_tc_scale_inv.x + 1) * 0.5,
        (-world_view_pos.y * volfog_shadow_tc_scale_inv.y + 1) * 0.5,
        (-world_view_pos.z * volfog_shadow_tc_scale_inv.z + 1) * 0.5,
        0);
    }
  }
endmacro

macro USE_VOLFOG_SHADOW(code)
  hlsl(code)
  {
  ##if volfog_shadow != NULL
      float get_volfog_shadow(float3 world_pos)
      {
        float3 tc =  world_pos * volfog_shadow_tc_mul + volfog_shadow_tc_add;

        BRANCH
        if (any(tc < 0 || tc > 1))
          return 1;

        float3 vignette = pow2(saturate(abs(tc * 2 - 1) * 10 - 9));
        float vignetteEffect = dot(vignette, vignette);
        float volfogShadow = tex3Dlod(volfog_shadow, float4(tc,0)).x;
        return lerp(volfogShadow, 1.0, vignetteEffect);
      }
  ##else
      float get_volfog_shadow(float3 world_pos)
      {
        return 1;
      }
  ##endif
  }
endmacro

