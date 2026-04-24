half ComputeTextureLOD(in float2 texDim, float2 dx, float2 dy)
{
  half2 mag = (abs(dx) + abs(dy)) * texDim;
  half lod = log2(max(mag.x, mag.y));
  return lod;
}

#ifndef parallax_tex_sampler
  #define parallax_tex_sampler(sample, tex, tc) sample = tex2Dlod(tex, tc)
#endif

half2 ParallaxOcclusionMap(in half4 baseTC, in half3 viewDirNrm, in int numSteps, in int numSteps_bil, in half displacement, out half4 outTex)
{
  half stepSize =  1.0 / numSteps;
  half bumpScale = displacement;

  half2 delta = viewDirNrm.xy * bumpScale / (-viewDirNrm.z * numSteps);
  half height = 1 - stepSize;
  half4 offset = baseTC;
  offset.xy += delta;

  half4 intersect = half4(delta * numSteps, delta * numSteps + baseTC.xy);

  half4 NB0;
  parallax_tex_sampler(NB0, PARALLAX_TEX, baseTC);
  parallax_tex_sampler(outTex, PARALLAX_TEX, offset);

  #ifdef PARALLAX_DIR
    float dir = (outTex.PARALLAX_ATTR > 1) ? -1 : 1; // for bumps
    stepSize *= dir;
    delta *= dir;
  #endif

  int i;
  for (i = 0; i < numSteps; i++)
  {
    #ifdef PARALLAX_DIR
      FLATTEN
      if (outTex.PARALLAX_ATTR * dir >= height * dir)
        break;
    #else
      FLATTEN
      if (outTex.PARALLAX_ATTR >= height)
        break;
    #endif

    NB0 = outTex;

    height -= stepSize;
    offset.xy += delta;

    parallax_tex_sampler(outTex, PARALLAX_TEX, offset);
  }

  half4 offsetBest = offset;
  half error = 1.0;

  half t1 = height;
  half t0 = t1 + stepSize;

  half delta1 = t1 - outTex.PARALLAX_ATTR;
  half delta0 = t0 - NB0.PARALLAX_ATTR;

  for (i = 0; i < numSteps_bil; i++)
  {
    FLATTEN
    if (abs(error) <= 0.01)
      break;

    half denom = delta1 - delta0;
    half t = (t0 * delta1 - t1 * delta0) / denom;
    offsetBest.xy = -t * intersect.xy + intersect.zw;

    parallax_tex_sampler(outTex, PARALLAX_TEX, offsetBest);

    error = t - outTex.PARALLAX_ATTR;
    if (error < 0)
    {
      delta1 = error;
      t1 = t;
    }
    else
    {
      delta0 = error;
      t0 = t;
    }
  }
  return offsetBest.xy;
}

#ifndef num_parallax_iterations
  #define num_parallax_iterations 4
#endif

float2 get_parallax(float2 parallaxTS, float4 texCoord, float str)
{
  parallaxTS *= str / num_parallax_iterations;
  float4 tc = texCoord;
  for (int i = 0; i < num_parallax_iterations; i++)
  {
    float4 texSample;
    parallax_tex_sampler(texSample, PARALLAX_TEX, tc);
    float height = 1 - texSample.PARALLAX_ATTR;
    tc.xy += parallaxTS.xy * height;
  }
  return tc.xy;
}
