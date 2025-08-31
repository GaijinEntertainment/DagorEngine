#ifndef RT_TEXTURE_OUTPUTS
  BINDLESS_TEX_ARRAY(Texture2D, rt_texture_outputs, 3);
  BINDLESS_SMP_ARRAY(SamplerComparisonState, rt_comparison_texture_samplers, 2);

  #ifndef GLOBAL_SAMPLES_ARRAY_DEFINED
    BINDLESS_SMP_ARRAY(SamplerState, global_samplers_array, 1);
    #define GLOBAL_SAMPLES_ARRAY_DEFINED 1
  #endif

  #define RT_TEXTURE_OUTPUTS
#endif
