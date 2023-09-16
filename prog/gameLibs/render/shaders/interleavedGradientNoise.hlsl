#ifndef INTERLEAVED_GRADIENT_INCLUDED
#define INTERLEAVED_GRADIENT_INCLUDED 1

//http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
float interleavedGradientNoise(float2 xy)
{
  return frac(52.9829189*frac(xy.x*0.06711056 + xy.y*0.00583715));
}

//we should better use frameId mod something (not infinite)
float interleavedGradientNoiseFramed( float2 xy, float frameId )
{
  // magic values are found by experimentation
  xy += frameId * (float2(47, 17) * 0.695f);
  return frac(52.9829189f * frac(xy.x*0.06711056 + xy.y*0.00583715));
}

#endif