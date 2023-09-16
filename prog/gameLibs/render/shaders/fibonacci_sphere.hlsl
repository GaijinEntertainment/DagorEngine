float2 fibonacci_sphere( uint Index, uint NumSamples )
{
  float E1 = (float)Index / NumSamples;
  float E2 = frac(Index * 0.6180339887498949);
  return float2( E1, E2 );
}

float2 fibonacci_sphere_randomized( uint Index, uint NumSamples, uint2 Random )
{
  float E1 = frac( (float)Index / NumSamples + float( Random.x & 0xffff ) / (1<<16) );
  float E2 = frac( ((Index + Random.y) % NumSamples) * 0.6180339887498949);//(sqrt(5.f)+1)/2.f -1 = 0.6180339887498949, golden section
  return float2( E1, E2 );
}