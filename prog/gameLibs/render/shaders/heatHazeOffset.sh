float4 haze_params        = (0, 0, 0, 0);
float   haze_target_width  = 1;
float   haze_target_height = 1;

texture haze_offset_tex;
texture haze_color_tex;
texture haze_noise_tex;
texture haze_depth_tex;
texture haze_scene_depth_tex;

macro INIT_HEAT_HAZE_OFFSET()
  (ps)
  {
    haze_params@f4             = haze_params; // xy - global noise animation factors, zw - global noise scale factors
    haze_target_texel_size@f2  = (1.0 / haze_target_width, 1.0 / haze_target_height, 0, 0);
    haze_offset_tex@smp2d      = haze_offset_tex;
    haze_color_tex@tex2d       = haze_color_tex;
    haze_noise_tex@smp2d       = haze_noise_tex;
    haze_depth_tex@smp2d       = haze_depth_tex;
    haze_scene_depth_tex@smp2d = haze_scene_depth_tex;
  }
endmacro

macro USE_HEAT_HAZE_OFFSET()
  hlsl(ps)
  {
      static const half distortionBlurScale     = 0.0025;
      static const half distortionBlurThreshold = 0.000026;

      void sample_heat_haze_offset_mul_blur(half2 tc, out half2 distortionOffset, out half3 distortionOffsetMul, out half distortionBlur)
      {
        distortionBlur      = 0;
        distortionOffset    = 0;
        distortionOffsetMul = 1;

        ##if (haze_offset_tex != NULL)
          half4 offset = h4tex2Dlod(haze_offset_tex, half4(tc, 0.0, 0.0));

          BRANCH
          if (any(saturate((abs(offset.xy) * haze_params.z * haze_params.w) - haze_target_texel_size.xy))
            ||any(saturate(offset.aa - haze_target_texel_size.xy)))
          {
            float depthHaze = h4tex2Dlod(haze_depth_tex, half4(tc,0,0)).x;
            half2 hazeNoise = h4tex2Dlod(haze_noise_tex, half4((tc + haze_params.xy).xy * half2(3, 2.3),0,0)).xy;
            hazeNoise = hazeNoise * hazeNoise * hazeNoise;
            hazeNoise = lerp(half2(1, 1), hazeNoise, saturate(offset.z)) * haze_params.z;
            offset.xy *= haze_params.w * hazeNoise;
            offset.xy = max(min(offset.xy, half2(0.5, 0.5)), half2(-0.5, -0.5));

            float depthSceneAtOffset = h4tex2Dlod(haze_scene_depth_tex, half4(tc + offset.xy,0,0)).x;
            distortionOffset = depthHaze > depthSceneAtOffset ? offset.xy : half2(0, 0);
            distortionBlur   = saturate(offset.a) * distortionBlurScale;

            ##if (haze_color_tex != NULL)
              distortionOffsetMul = haze_color_tex.SampleLevel(haze_offset_tex_samplerstate, tc, 0).xyz;
            ##else
              distortionOffsetMul = 1;
            ##endif
          }
        ##endif
      }
  }
endmacro
