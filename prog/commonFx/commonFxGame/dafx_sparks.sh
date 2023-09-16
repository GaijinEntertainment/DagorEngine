include "dafxSparkModules/dafx_shader_common.sh"
include "dafx_helpers.sh"

shader dafx_culling_discard
{
  ENABLE_ASSERT(cs)
  DAFX_CULLING_DISCARD_USE()
}

shader dafx_sparks_ps_emission, dafx_sparks_ps_simulation
{
  ENABLE_ASSERT(cs)
  DAFXEX_USE_DEPTH_FOR_COLLISION(cs)
  USE_DAFX_SYSTEM_COMMON()

  hlsl(cs)
  {
    #include "dafxSparks_decl.hlsl"
    #include "dafx_sparks.hlsl"
  }

  if (shader == dafx_sparks_ps_emission)
  {
    DAFX_EMISSION_USE()
  }
  if (shader == dafx_sparks_ps_simulation)
  {
    DAFX_SIMULATION_USE()
  }
}

shader sparks_ps, sparks_thermal
{
  ENABLE_ASSERT(ps)
  DAFXEX_USE_DEPTH_MASK(ps)
  USE_DAFX_RENDER_COMMON()
  DAFXEX_USE_SCENE_BLOCK()

  hlsl
  {
    #include "dafxSparks_decl.hlsl"
    #include "fx_thermals.hlsl"
    #include "dafx_globals.hlsli"
  }

  DAFX_RENDER_USE()
  DECL_POSTFX_TC_VS_RT()
  INIT_SIMPLE_AMBIENT(vs)
  DAFXEX_USE_FOG()

  blend_src = one; blend_dst = sa;
  blend_asrc = zero; blend_adst = one;

  z_write = false;
  z_test = true;
  cull_mode = none;

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 color: TEXCOORD0;
      float4 tc_size: TEXCOORD1;
      float4 psize_dista_hdr_blend: TEXCOORD2;
      float4 screenTexcoord: TEXCOORD3;
##if shader == sparks_ps
      float3 lighting: TEXCOORD4;
##endif
    };
  }

  hlsl(ps) {
    #define MID_COLOR float3(1, 1, 1)
    #define IS_ADDITIVE (input.psize_dista_hdr_blend.w == 0)
    #define IS_ABLEND (input.psize_dista_hdr_blend.w > 0)

    float4 sparks_fx_ps(VsOutput input) : SV_Target
    {
      GlobalData gdata = global_data_load();

      float len = input.tc_size.z;
      float rad = min(input.tc_size.w * 0.5, len * 0.5);
      float2 distVec = input.tc_size.xy * input.tc_size.zw;
      float2 rvec = distVec - float2(distVec.x < rad ? rad : len - rad, rad);
      float rvecSq = dot(rvec, rvec);
      bool corners = distVec.x < rad || distVec.x > len - rad;
      if (rvecSq > pow2(rad) && corners)
        discard;
      float border = corners ? sqrt(rvecSq) : abs(rvec.y);
      float borderAA = saturate((rad - border) / (input.psize_dista_hdr_blend.x * 0.5));
      float4 color = input.color;
##if shader == sparks_ps
      color.rgb = lerp(color.rgb, MID_COLOR, pow2(pow4(1.0 - border / rad))) * input.psize_dista_hdr_blend.z;
##endif
##if shader == sparks_thermal
      fx_thermals_apply_additive(color);
##endif

      half depthMask = dafx_get_hard_depth_mask(input.screenTexcoord, gdata);
      float4 res = float4(color.rgb, color.a * borderAA * input.psize_dista_hdr_blend.y * depthMask);
      if (res.a <= 0)
        discard;

##if shader == sparks_ps
      if (IS_ABLEND)
      {
        res.rgb *= input.lighting * res.a;
        res.a = 1.0 - res.a;
      }
      else
      {
        res.rgb *= res.a;
        res.a = 1;
      }
##endif
##if shader == sparks_thermal
      res.rgb *= res.a;
      res.a = 1;
##endif
      return res;
    }
  }

  hlsl(vs) {
    #define VEL_BIAS 0.1
    #define VEL_DOT_BIAS 0.2
    #define PIX_BORDER_AA 6
    #define SUN_BACKLIGHT 0.2

    VsOutput sparks_fx_vs(uint vertexId: SV_VertexID, uint instance_id: SV_InstanceID)
    {
      GlobalData gparams = global_data_load();

      uint gid;
      uint data_ofs;
      uint parent_ofs;
      dafx_gen_render_data(instance_id, gid, data_ofs, parent_ofs);

      RenData p = unpack_ren_data( 0, data_ofs );
      ParentRenData ps = unpack_parent_ren_data( 0, parent_ofs );

      float3 wpos = float3(
        dot(float4(p.pos, 1), ps.fxTm[0]),
        dot(float4(p.pos, 1), ps.fxTm[1]),
        dot(float4(p.pos, 1), ps.fxTm[2]));
      // offset for depth buffer collision
      wpos += float3(0, 1, 0) * p.width * 0.5;

      p.velocity = float3(
        dot(float4(p.velocity, 0), ps.fxTm[0]),
        dot(float4(p.velocity, 0), ps.fxTm[1]),
        dot(float4(p.velocity, 0), ps.fxTm[2]));

      uint vPosId = vertexId % 4;
      float2 vPos = float2(vPosId == 0 || vPosId == 1 ? -0.5 : 0.5, vPosId == 0 || vPosId == 3 ? -0.5 : 0.5);
      float3 eyeVec = gparams.world_view_pos - wpos;
      float eyeDist = length(eyeVec);
      float3 eyeVecNorm = (gparams.world_view_pos - wpos) / eyeDist;
      float velLen = length(p.velocity);
      float velDot = velLen > VEL_BIAS ? saturate((1.0 - abs(dot(eyeVecNorm, p.velocity / velLen))) / VEL_DOT_BIAS) : 0;
      float3 sideVec = velLen > VEL_BIAS ? p.velocity : float3(0, 1, 0);
      float3 upVec = normalize(cross(sideVec, eyeVecNorm));
      sideVec = cross(eyeVecNorm, upVec);
      float pixWorldSize = gparams.target_size_rcp.y * eyeDist * rcp(gparams.proj_hk);
      float pixBorderSize = pixWorldSize * PIX_BORDER_AA;
      float ySize = max(p.width, pixBorderSize);
      float xSize = max(ySize, ps.motionScale * velLen * velDot);
      float3 worldPos = wpos + sideVec * vPos.x * xSize + upVec * vPos.y * ySize;

      float4 projPos = mul(float4(worldPos, 1.0), gparams.globtm);
      float distAlpha = saturate(pow2(p.width / pixWorldSize));
      distAlpha *= saturate((projPos.w - projPos.z - 0.5) / 1.0);

      float viewDot = saturate(dot(eyeVecNorm, -from_sun_direction.xyz) * 0.5 + 0.5);

      VsOutput output = (VsOutput)0;
      output.pos = projPos;
      output.color = p.color;
      // Use ySize for x dimension to make an arrowed shape
      output.tc_size = float4(vPos + float2(0.5, 0.5), lerp(xSize, ySize, ps.arrowShape), ySize);
      output.psize_dista_hdr_blend = float4(pixBorderSize, distAlpha, p.hdrScale, ps.blending);
      output.screenTexcoord = float4(
        projPos.xy * RT_SCALE_HALF + float2(0.5, 0.5) * projPos.w,
        projPos.z,
        projPos.w);

      float2 stc = output.pos.w > 0 ? output.screenTexcoord.xy / output.pos.w : 0;
      half3 fog_mul, fog_add;
      get_volfog_with_scattering(stc, stc, eyeVecNorm, eyeDist, output.pos.w, fog_mul, fog_add);
      output.color.a *= dot(float3(1, 1, 1), fog_mul.rgb) / 3.0f;
##if shader == sparks_ps
      float3 lighting = sun_color_0.rgb * (lerp(viewDot, 1, SUN_BACKLIGHT)) + amb_color;
      output.lighting = lighting;
##endif
      if (p.width <= 0)
        output.pos = float4(-2, -2, 1, 1);

      return output;
    }
  }

  compile("target_ps", "sparks_fx_ps");
  compile("target_vs", "sparks_fx_vs");
}