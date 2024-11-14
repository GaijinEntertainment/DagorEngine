#ifndef RT_TEXTURE_OUTPUTS
  BINDLESS_TEX_ARRAY(Texture2D, rt_texture_outputs, 3);
  BINDLESS_SMP_ARRAY(SamplerComparisonState, rt_texture_samplers, 2);
  #define RT_TEXTURE_OUTPUTS
#endif
