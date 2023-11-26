macro INIT_AND_USE_EMISSIVE_ANIM( illum_program_texno, phase_shift_texno)
  texture illum_program = material.texture[illum_program_texno];
  texture phase_shift = material.texture[phase_shift_texno];

  static float illum_scroll_speed = 1.0;
  static float illum_scroll_scale = 2.0;
  static float use_additive_blend = 0.0;

  if (phase_shift != NULL)
  {
    (ps) { phase_shift@static = phase_shift; }
  }
  (ps) { illum_program@static = illum_program; }
  (ps) { scroll_param@f3 = (illum_scroll_speed, illum_scroll_scale-1.0, use_additive_blend, 0); }

  hlsl(ps) {
    void get_emissive_anim(in float2 texcoord, in float in_time, inout float4 diffuseColor, inout float emissionStrength)
    {
      //read emission strength and program
      ##if (phase_shift != NULL)
        // R - phase shift, G - program shift, B - scrolling scale
        float3 phaseShift = tex2DLodBindless(get_phase_shift(), float4(texcoord, 0.0f, 0.0f)).rgb;
      ##else
        float3 phaseShift = 0;
      ##endif
      float phase = in_time*get_scroll_param().x*(phaseShift.b*get_scroll_param().y + 1.0f) + phaseShift.r;
      float3 emissionColor = tex2DLodBindless(get_illum_program(), float4(frac(phase), phaseShift.g, 0.0f, 0.0f)).rgb * diffuseColor.a;
      emissionStrength = length(emissionColor.rgb);
      emissionColor.rgb+=0.0001f; // to avoid zero value
      BRANCH
      if (get_scroll_param().z > 0.0f)
        diffuseColor.rgb = diffuseColor.rgb + emissionColor.rgb;
      else
        diffuseColor.rgb = lerp(diffuseColor.rgb, diffuseColor.rgb*normalize(emissionColor.rgb), saturate(2*emissionStrength));
    }
  }
endmacro