int xray_pass_mode = 0;
interval xray_pass_mode : xray_pass_mode_color < 1, xray_pass_mode_normals;

int use_gbuf_lighting = 0;
buffer xray_part_parameters;

float4 xray_sonar_pos_offset = float4(0, 0, 0, 0);
float xray_sonar_alpha = 0;
float4 sonar_scale_power_speed_time = float4(1, 1, 1, 0);

macro INIT_XRAY_RENDER()
  (ps) {
    use_gbuf_lighting@f1 = (use_gbuf_lighting, 0, 0, 0);
    xray_part_parameters@cbuf = xray_part_parameters hlsl {
      #include <render/xray.hlsli>
      cbuffer xray_part_parameters@cbuf
      {
        XrayPartParams partParams[MAX_XRAY_PARTS];
      }
    };
  }
  (vs) {
    xray_part_parameters@cbuf = xray_part_parameters hlsl {
      #include <render/xray.hlsli>
      cbuffer xray_part_parameters@cbuf
      {
        XrayPartParams partParams[MAX_XRAY_PARTS];
      }
    };
  }
endmacro

macro USE_XRAY_RENDER()
  hlsl(ps) {
    half4 xray_lighting(float3 point_to_eye, float3 world_normal, XrayPartParams part_params)
    {
      float fresnel = saturate(1 - dot(world_normal, normalize(point_to_eye).xyz));
      fresnel = saturate(lerp(part_params.hatching_type.y, part_params.hatching_type.z, pow2(fresnel)) + part_params.hatching_type.x * world_normal.y);
      half4 colorRet = lerp(part_params.hatching_fresnel, part_params.hatching_color * part_params.hatching_type.w, fresnel);
      return half4(colorRet.rgb, part_params.hatching_color.a);
    }
  }
endmacro

macro USE_XRAY_RENDER_DISSOLVE()
  (ps) {
    noise_128_tex_hash@smp2d =  noise_128_tex_hash;
  }

  hlsl(ps) {
    bool apply_xray_dissolve(float4 screen_pos, XrayPartParams part_params, out float4 color)
    {
      BRANCH
      if (part_params.dissolve_rad > 0)
      {
        half2 maskTc = (screen_pos.xy / screen_pos.w) * half2(0.5, -0.5) + half2(0.5, 0.5);
        float vd = saturate(part_params.dissolve_rad);
        if (tex2Dlod(noise_128_tex_hash, float4(maskTc, 0, 0)).r > vd)
        {
          color = half4(0, 0, 0, 0);
          return true;
        }
      }
      return false;
    }
  }
endmacro

macro XRAY_RENDER_SHADER(point_to_eye, normal, use_dissolve)
  USE_XRAY_RENDER()

  (ps)
  {
    sonar_time@f1 = time_phase(0, 0) - sonar_scale_power_speed_time.w;
    xray_sonar_pos_offset@f4 = xray_sonar_pos_offset;
    sonar_scale_power_speed__xray_grad@f4 = (sonar_scale_power_speed_time.x, sonar_scale_power_speed_time.y,
                                          sonar_scale_power_speed_time.z, xray_sonar_alpha);
  }

  if (xray_pass_mode == xray_pass_mode_color)
  {
    blend_src = sa; blend_dst = zero;
    blend_asrc = one; blend_adst = zero;
  }

  if (use_dissolve)
  {
    hlsl(ps) {
      #define USE_DISSOLVE 1
    }
    USE_XRAY_RENDER_DISSOLVE()
  }

  hlsl(ps) {
    #ifndef PARAMS_ID
    #define PARAMS_ID 0
    #endif

    half4 render_pass_xray(VsOutput input) : SV_Target
    {
      float3 worldNormal = normalize(normal.xyz);
      XrayPartParams part_params = partParams[clamp(PARAMS_ID, 0, MAX_XRAY_PARTS-1)];

  ##if in_editor_assume == yes
      part_params.hatching_color = float4(0.68, 0.88, 1.0, 0.4);
      part_params.hatching_fresnel = float4(0.7, 0.7, 0.7, 1.0);
      part_params.hatching_type = float4(0.2, 0.0, 1.2, 1.7);
  ##endif

  #if USE_DISSOLVE
      float4 dissolveColor;
      if (apply_xray_dissolve(input.screenPos, part_params, dissolveColor))
        return dissolveColor;
  #endif

  ##if xray_pass_mode == xray_pass_mode_normals
      if (sonar_scale_power_speed__xray_grad.w < 1)
        return half4(worldNormal * 0.5f + 0.5f, part_params.hatching_color.a);
      float3 worldPos = world_view_pos.xyz - point_to_eye.xyz;
      float dist = length(worldPos.xyz - xray_sonar_pos_offset.xyz) - xray_sonar_pos_offset.w;
      float intensity = sonar_time * sonar_scale_power_speed__xray_grad.z - dist * sonar_scale_power_speed__xray_grad.x;
      return half4(worldNormal.xyz * 0.5f + 0.5f,
            max(1 - pow(abs(intensity), sonar_scale_power_speed__xray_grad.y), 0.0));
  ##else
      #if USE_XRAY_LIGHT
        BRANCH
        if (use_gbuf_lighting > 0)
        {
          return get_xray_gbuffer_lighting(input);
        }
      #endif

      half4 result = xray_lighting(point_to_eye, worldNormal, part_params);
      #if XRAY_RENDER_SHADER_MUL_INPUT_COLOR
        result *= half4(input.color.rgb, 1.0);
      #endif
      return result;
  ##endif
    }
  }
  compile("target_ps", "render_pass_xray");
endmacro