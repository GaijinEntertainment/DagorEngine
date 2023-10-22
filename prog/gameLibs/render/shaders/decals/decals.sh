texture dynamic_decals_atlas;

int planar_decals_use_vertex_shader = 0;
interval planar_decals_use_vertex_shader : no < 1, yes;

macro INIT_PLANAR_DECALS()
  (ps) {
    atlas@smp2d = dynamic_decals_atlas;
  }
endmacro

macro USE_PLANAR_DECALS(start_params_no)
  hlsl {
    // minAng80 = 0.174 maxAng45 = 0.707 (influence - minAng80) / (maxAng45 - minAng80)
    #define INFLUENCE_MUL 1.87617
    #define INFLUENCE_ADD 0.32645

    #define TYPE_SCALE_MUL 1024

    float GetInfluence(float influence)
    {
      return saturate(influence * INFLUENCE_MUL + INFLUENCE_ADD);
    }

    #define DECAL_GET_DYNREND_PARAM(param_no, dynrend_params_offset_count) ((int)param_no < (int)dynrend_params_offset_count.y ? \
        loadBuffer(per_chunk_render_data, dynrend_params_offset_count.x + param_no) : float4(0, 0, 0, 0))

    void planar_decals_vs(
        inout float4 decals_uv_depth_influence,
        int decal_idx, float type_scale_inv, float3 model_pos,
        float3 model_normal, float2 dynrend_params_offset_count)
    {
      float2 params = dynrend_params_offset_count;
      float4 modelPos4 = float4(model_pos, 1);

      int i = decal_idx;
      int line_idx = start_params_no + 1 + 4 * i;
      int norm_idx = start_params_no + 1 + 16 + i * 2;

      float4 n0 = DECAL_GET_DYNREND_PARAM(norm_idx + 0, params);
      float influence0 = dot(model_normal, n0.xyz);

      decals_uv_depth_influence.z = dot(modelPos4, n0) * type_scale_inv;
      decals_uv_depth_influence.w = GetInfluence(influence0);
      decals_uv_depth_influence.xy = float2(
        dot(modelPos4, DECAL_GET_DYNREND_PARAM(line_idx + 0, params)),
        dot(modelPos4, DECAL_GET_DYNREND_PARAM(line_idx + 1, params)));
    }

    void apply_planar_decals_vs(
      inout float4 decals_uv_depth_influence_0,
      inout float4 decals_uv_depth_influence_1,
      inout float4 decals_uv_depth_influence_2,
      inout float4 decals_uv_depth_influence_3,
      float3 model_pos,
      float3 model_normal,
      float2 dynrend_params_offset_count)
    {
      float4 decal_encoded_widht__multiplier = DECAL_GET_DYNREND_PARAM(start_params_no, dynrend_params_offset_count);
      float4 typeScaleInv = TYPE_SCALE_MUL / decal_encoded_widht__multiplier;

      #define VS_CALC_DECAL(idx) \
        planar_decals_vs(decals_uv_depth_influence_##idx, idx, typeScaleInv[idx], \
        model_pos, model_normal, dynrend_params_offset_count);

      VS_CALC_DECAL(0);
      VS_CALC_DECAL(1);
      VS_CALC_DECAL(2);
      VS_CALC_DECAL(3);

      #undef VS_CALC_DECAL
    }
  }

  hlsl(ps) {
    #define TANK_DECAL_SMOOTHNESS 0.4f
    #define DIFFUSE_TO_DECAL 0.01f
    #define DIFFUSE_MUL_RCP  (1.0f / (DIFFUSE_TO_DECAL * DIFFUSE_TO_DECAL + 1e-6))

    void process_decal_indexed(
      int decalIndex, float4 decal_uv_depth_influence, half diffMul, float4 modelPos4, float3 model_normal, float2 params,
      int dec_mul, float type_scale_inv, inout half4 decalEffect, inout half3 diffuseColor)
    {
      int i = decalIndex;
      int uv_idx = start_params_no + 1 + 16 + 8 + i;

      float2 uv = decal_uv_depth_influence.xy;
      float depth = decal_uv_depth_influence.z;
      float influence = decal_uv_depth_influence.w;

      // apply a mirrored decal in the pixel shader only to avoid interpolation artifacts
      BRANCH
      if (dec_mul < 0)
      {
        int line_idx = start_params_no + 1 + 4 * i;
        int norm_idx = start_params_no + 1 + 16 + i * 2;
        float4 n1 = DECAL_GET_DYNREND_PARAM(norm_idx + 1, params);
        float influence1 = dot(model_normal, n1.xyz);
        FLATTEN
        if (influence1 > influence)
        {
          depth = dot(modelPos4, n1) * type_scale_inv;
          influence = GetInfluence(influence1);
          uv = float2(
            dot(modelPos4, DECAL_GET_DYNREND_PARAM(line_idx + 2, params)),
            dot(modelPos4, DECAL_GET_DYNREND_PARAM(line_idx + 3, params)));
          if (dec_mul < -1)
          {
            uv.x = 1 - uv.x;
          }
        }
      }

      float4 uvScaleOffset = GET_DYNREND_PARAM(uv_idx, params);
      float2 uvScale = uvScaleOffset.zw - uvScaleOffset.xy;

      #define CLAMP_BORDER_ATLAS(a, name, val) if (a.x <= 0.0f || a.x >= 1.0f || a.y <= 0.0f || a.y >= 1.0f) {name = val;}
      half4 decal = all((half2)uvScale) ? h4tex2D(atlas, uv * uvScale + uvScaleOffset.xy) : half4(0.h, 0.h, 0.h, 0.h);
      CLAMP_BORDER_ATLAS(uv, decal, half4(0.h, 0.h, 0.h, 0.h));
      #undef CLAMP_BORDER_ATLAS
      decalEffect[i] = abs((half)depth) < 1.h ? (half)influence * decal.a : 0.h;
      diffuseColor.rgb = lerp(diffuseColor.rgb, diffMul * decal.rgb, decalEffect[i]);
    }

    half apply_planar_decals_ps(
      inout half3 diffuseColor,
      float3 model_pos,
      float3 model_normal,
      float4 decals_uv_depth_influence_0,
      float4 decals_uv_depth_influence_1,
      float4 decals_uv_depth_influence_2,
      float4 decals_uv_depth_influence_3,
      float2 dynrend_params_offset_count)
    {
      float4 decal_encoded_widht__multiplier = DECAL_GET_DYNREND_PARAM(start_params_no, dynrend_params_offset_count);
      int4 decMul = (-2 * (frac(decal_encoded_widht__multiplier) >= 0.5) + 1);
      decMul *= (frac(decal_encoded_widht__multiplier * 2) >= 0.5) + 1;
      decMul *= frac(decal_encoded_widht__multiplier * 4) >= 0.5;
      float4 typeScaleInv = TYPE_SCALE_MUL / decal_encoded_widht__multiplier;

      half diffMul = saturate(luminance(diffuseColor.rgb) * (half)DIFFUSE_MUL_RCP);
      half4 decalEffect = 0.h;

      #define PROCESS_DECAL(idx) process_decal_indexed(\
        idx, decals_uv_depth_influence_##idx, diffMul, float4(model_pos, 1), model_normal, dynrend_params_offset_count, decMul[idx], typeScaleInv[idx], decalEffect, diffuseColor)

      PROCESS_DECAL(0);
      PROCESS_DECAL(1);
      PROCESS_DECAL(2);
      PROCESS_DECAL(3);

      #undef PROCESS_DECAL

      decalEffect = 1.h - decalEffect;
      return saturate((decalEffect.x*decalEffect.y)*(decalEffect.z*decalEffect.w));
    }

    half apply_decals(inout half3 diffuseColor, float3 model_pos, float3 model_normal, float2 dynrend_params_offset_count)
    {
      float4 decals_uv_depth_influence[4];
      apply_planar_decals_vs(decals_uv_depth_influence[0], decals_uv_depth_influence[1], decals_uv_depth_influence[2], decals_uv_depth_influence[3],
        model_pos, model_normal, dynrend_params_offset_count);

      return apply_planar_decals_ps(diffuseColor, model_pos, model_normal, decals_uv_depth_influence[0], decals_uv_depth_influence[1], decals_uv_depth_influence[2],
        decals_uv_depth_influence[3], dynrend_params_offset_count);
    }
  }
endmacro



