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
    #define TANK_DECAL_SMOOTHNESS 0.4f
    #define DIFFUSE_TO_DECAL 0.01f
    #define DIFFUSE_MUL_RCP  (1.0f / (DIFFUSE_TO_DECAL * DIFFUSE_TO_DECAL + 1e-6))
    float GetInfluence(float influence)
    {
      float minAng80 = 0.174;
      float maxAng45 = 0.707;
      return saturate((influence - minAng80) / (maxAng45 - minAng80));
    }
  }

  if (planar_decals_use_vertex_shader == yes)
  {
    hlsl(vs) {
      #define DECAL_GET_DYNREND_PARAM(param_no, dynrend_params_offset_count) ((int)param_no < (int)dynrend_params_offset_count.y ? \
        loadBuffer(per_chunk_render_data, dynrend_params_offset_count.x + param_no) : float4(0, 0, 0, 0))

      void planar_decals_vs_common(
        inout int4 dec_mul, inout float4 type_scale, float2 dynrend_params_offset_count)
      {
        float2 params = dynrend_params_offset_count;
        float4 decal_encoded_widht__multiplier = DECAL_GET_DYNREND_PARAM(start_params_no, params);
        dec_mul = (-2 * (frac(decal_encoded_widht__multiplier) >= 0.5) + 1);
        dec_mul *= (frac(decal_encoded_widht__multiplier * 2) >= 0.5) + 1;
        dec_mul *= frac(decal_encoded_widht__multiplier * 4) >= 0.5;
        type_scale = decal_encoded_widht__multiplier / 1024;
      }

      void planar_decals_vs(
        inout float4 decals_uv_depth_influence,
        int decal_idx, int dec_mul, float type_scale, float3 model_pos,
        float3 model_normal, float2 dynrend_params_offset_count)
      {
        float2 params = dynrend_params_offset_count;
        float4 modelPos4 = float4(model_pos, 1);
        int i = decal_idx;

        int line_idx = start_params_no + 1 + 4 * i;
        int norm_idx = start_params_no + 1 + 16 + i * 2;
        int uv_idx = start_params_no + 1 + 16 + 8 + i;

        float4 n0 = DECAL_GET_DYNREND_PARAM(norm_idx + 0, params);
        float4 n1 = DECAL_GET_DYNREND_PARAM(norm_idx + 1, params);
        float influence0 = dot(model_normal, n0.xyz);
        float influence1 = dot(model_normal, n1.xyz);
        FLATTEN
        if (dec_mul < 0 && influence1 > influence0)
        {
          decals_uv_depth_influence.z = dot(modelPos4, n1) / type_scale;
          decals_uv_depth_influence.w = GetInfluence(influence1);
          decals_uv_depth_influence.xy = float2(
            dot(modelPos4, DECAL_GET_DYNREND_PARAM(line_idx + 2, params)),
            dot(modelPos4, DECAL_GET_DYNREND_PARAM(line_idx + 3, params)));
          if (dec_mul < -1)
          {
            decals_uv_depth_influence.x = 1 - decals_uv_depth_influence.x;
          }
        }
        else
        {
          decals_uv_depth_influence.z = dot(modelPos4, n0) / type_scale;
          decals_uv_depth_influence.w = GetInfluence(influence0);
          decals_uv_depth_influence.xy = float2(
            dot(modelPos4, DECAL_GET_DYNREND_PARAM(line_idx + 0, params)),
            dot(modelPos4, DECAL_GET_DYNREND_PARAM(line_idx + 1, params)));
        }
      }
    }
  }

  hlsl(ps) {
    ##if (planar_decals_use_vertex_shader == yes)
    void process_decal_indexed(
      int decalIndex, float4 decal_uv_depth_influence, half diffMul, float2 params,
      inout half4 decalEffect, inout half3 diffuseColor)
    {
      int i = decalIndex;

      int line_idx = start_params_no + 1 + 4 * i;
      int norm_idx = start_params_no + 1 + 16 + i * 2;
      int uv_idx = start_params_no + 1 + 16 + 8 + i;

      float4 uvScaleOffset = GET_DYNREND_PARAM(uv_idx, params);
      float2 uvScale = uvScaleOffset.zw - uvScaleOffset.xy;

      #define CLAMP_BORDER_ATLAS(a, name, val) if (a.x <= 0.0f || a.x >= 1.0f || a.y <= 0.0f || a.y >= 1.0f) {name = val;}
      float2 uv = decal_uv_depth_influence.xy;
      float depth = decal_uv_depth_influence.z;
      float influence = decal_uv_depth_influence.w;
      half4 decal = all((half2)uvScale) ? h4tex2D(atlas, uv * uvScale + uvScaleOffset.xy) : half4(0.h, 0.h, 0.h, 0.h);
      CLAMP_BORDER_ATLAS(uv, decal, half4(0.h, 0.h, 0.h, 0.h));
      #undef CLAMP_BORDER_ATLAS
      decalEffect[i] = abs((half)depth) < 1.h ? (half)influence * decal.a : 0.h;
      diffuseColor.rgb = lerp(diffuseColor.rgb, diffMul * decal.rgb, decalEffect[i]);
    }

    half apply_planar_decals_ps(
      inout half3 diffuseColor,
      float4 decals_uv_depth_influence_0,
      float4 decals_uv_depth_influence_1,
      float4 decals_uv_depth_influence_2,
      float4 decals_uv_depth_influence_3,
      float2 dynrend_params_offset_count)
    {
      float2 params = dynrend_params_offset_count;
      half diffMul = saturate(luminance(diffuseColor.rgb) * (half)DIFFUSE_MUL_RCP);
      half4 decalEffect = 0.h;

      #define PROCESS_DECAL(idx) process_decal_indexed(\
        idx, decals_uv_depth_influence_##idx, diffMul, dynrend_params_offset_count, decalEffect, diffuseColor)

      PROCESS_DECAL(0);
      PROCESS_DECAL(1);
      PROCESS_DECAL(2);
      PROCESS_DECAL(3);

      #undef PROCESS_DECAL

      return saturate((decalEffect.x*decalEffect.y)*(decalEffect.z*decalEffect.w));
    }
    ##endif

    half apply_decals(inout half3 diffuseColor, float3 model_pos, float3 model_normal, float2 dynrend_params_offset_count)
    {
      float2 params = dynrend_params_offset_count;
      float4 modelPos4 = float4(model_pos, 1);
      float4 decal_encoded_widht__multiplier = GET_DYNREND_PARAM(start_params_no, params);
      int4 dec_mul = (-2 * (frac(decal_encoded_widht__multiplier) >= 0.5) + 1);
      dec_mul *= (frac(decal_encoded_widht__multiplier * 2) >= 0.5) + 1;
      dec_mul *= frac(decal_encoded_widht__multiplier * 4) >= 0.5;

      float4 type_scale = decal_encoded_widht__multiplier / 1024;
      float decDepth[4], decTexInfluence[4];
      float2 uv[4];
      float2 uvElem[4];
      float uvMul[4];
      for (int i = 0; i < 4; i++)
      {
        int line_idx = start_params_no + 1 + 4 * i;
        int norm_idx = start_params_no + 1 + 16 + i * 2;
        int uv_idx = start_params_no + 1 + 16 + 8 + i;
        float depth0 = dot(modelPos4, GET_DYNREND_PARAM(norm_idx + 0, params)) / type_scale[i];
        float depth1 = dot(modelPos4, GET_DYNREND_PARAM(norm_idx + 1, params)) / type_scale[i];
        float influence0 = dot(model_normal, GET_DYNREND_PARAM(norm_idx + 0, params).xyz);
        float influence1 = dot(model_normal, GET_DYNREND_PARAM(norm_idx + 1, params).xyz);
        if (dec_mul[i] < 0 && influence1 > influence0)
        {
          decDepth[i] = depth1;
          decTexInfluence[i] = GetInfluence(influence1);
          uv[i] = float2(dot(modelPos4, GET_DYNREND_PARAM(line_idx + 2, params)),
                         dot(modelPos4, GET_DYNREND_PARAM(line_idx + 3, params)));
          if (dec_mul[i] < -1)
            uv[i].x = 1 - uv[i].x;
        } else {
          decDepth[i] = depth0;
          decTexInfluence[i] = GetInfluence(influence0);
          uv[i] = float2(dot(modelPos4, GET_DYNREND_PARAM(line_idx + 0, params)),
                         dot(modelPos4, GET_DYNREND_PARAM(line_idx + 1, params)));
        }

        float4 uvScaleOffset = GET_DYNREND_PARAM(uv_idx, params);
        float2 uvScale = uvScaleOffset.zw - uvScaleOffset.xy;
        uvElem[i] = uv[i] * uvScale + uvScaleOffset.xy;
        uvMul[i] = all(uvScale);
      }

      half diffMul = saturate(half(luminance(diffuseColor.rgb) * DIFFUSE_MUL_RCP));
      half4 decalEffect = 0.h;
      half4 decal[4];
      decal[0] = h4tex2D(atlas, uvElem[0]) * half(uvMul[0]);
      decal[1] = h4tex2D(atlas, uvElem[1]) * half(uvMul[1]);
      decal[2] = h4tex2D(atlas, uvElem[2]) * half(uvMul[2]);
      decal[3] = h4tex2D(atlas, uvElem[3]) * half(uvMul[3]);

      for(int j = 0 ; j < 4; j++)
      {
        #define CLAMP_BORDER_ATLAS(a, name, val) if (a.x <= 0.0f || a.x >= 1.0f || a.y <= 0.0f || a.y >= 1.0f) {name = val;}
        CLAMP_BORDER_ATLAS(uv[j], decal[j], 0.h);
        decalEffect[j] = half(decTexInfluence[j] * (abs(decDepth[j]) < 1) * decal[j].a);
        diffuseColor.rgb = lerp(diffuseColor.rgb, diffMul * decal[j].rgb, decalEffect[j]);
      }
      decalEffect = 1.h-decalEffect;

      return saturate((decalEffect.x*decalEffect.y)*(decalEffect.z*decalEffect.w));
    }
  }
endmacro



