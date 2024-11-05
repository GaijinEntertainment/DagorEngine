#ifndef FIBONACCI_SPHERE_INCLUDED
#define FIBONACCI_SPHERE_INCLUDED 1

float2 fibonacci_sphere( uint index, uint numSamples )
{
  float E1 = (float)index / numSamples;
  float E2 = frac(index * 0.6180339887498949);
  return float2(E1, E2);
}

float2 fibonacci_sphere_randomized( uint index, uint numSamples, uint2 random )//random is 16bit each
{
  float E1 = frac( (float)index / numSamples + float( random.x & 0xffff ) / (1<<16) );
  float E2 = frac( ((index + random.y) % numSamples) * 0.6180339887498949);//(sqrt(5.f)+1)/2.f -1 = 0.6180339887498949, golden section
  return float2( E1, E2 );
}

#endif