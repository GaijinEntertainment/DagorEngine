  #ifndef num_parallax_iterations
   #define num_parallax_iterations 4
  #endif

  #if SEPARATE_SAMPLER_OBJECT

  #define  get_texture_parallax(texname, ts, tc, str) get_texture_parallax_base(texname, texname##_samplerstate, ts, tc, str)

  float2 get_texture_parallax_base(Texture2D parallax, SamplerState parallax_samplerstate, float2 parallaxTS, float2 texCoord, float str)
  #else
  float2 get_texture_parallax(sampler2D parallax, float2 parallaxTS, float2 texCoord, float str)
  #endif
  {
    //half2 btc = input.tc.xy;
    //half lod = ComputeTextureLOD(texCoord, half2(256,256), dx,dy);
    parallaxTS *= (str/num_parallax_iterations);
    float2 tc = texCoord;
    //half4 tc = half4(texCoord,0,lod);
    for (int i=0; i<num_parallax_iterations; ++i)
    {
      float height = 1-tex2D(parallax, tc).PARALLAX_ATTR;
      tc.xy += parallaxTS.xy*height;
    }
    return tc;
  }

