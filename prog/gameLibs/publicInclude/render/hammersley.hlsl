#ifndef HAMMERSLEY_INCLUDED
#define HAMMERSLEY_INCLUDED 1

uint reverse_bits32( uint bits )
{
#if HAS_REVERSEBITS
  return reversebits( bits );
#else
  bits = ( bits << 16) | ( bits >> 16);
  bits = ( (bits & 0x00ff00ffU) << 8 ) | ( (bits & uint(0xff00ff00U)) >> 8 );
  bits = ( (bits & 0x0f0f0f0fU) << 4 ) | ( (bits & uint(0xf0f0f0f0U)) >> 4 );
  bits = ( (bits & 0x33333333U) << 2 ) | ( (bits & uint(0xccccccccU)) >> 2 );
  bits = ( (bits & 0x55555555U) << 1 ) | ( (bits & uint(0xaaaaaaaaU)) >> 1 );
  return bits;
#endif
}

float2 hammersley( uint sample_no, uint num_samples, uint2 random )
{
  float E1 = frac( ((float)sample_no) / num_samples + float( random.x & 0xffff ) * (1. / (1<<16)) );
  float E2 = float( reverse_bits32(sample_no) ^ random.y ) * 2.3283064365386963e-10;
  return float2( E1, E2 );
}

//random is 16 bits
float2 hammersley_rand16( uint sample_no, uint num_samples, uint2 random )
{
  float E1 = frac( (float)sample_no / num_samples + float( random.x ) * (1.0 / 65536.0) );
  float E2 = float( ( reverse_bits32(sample_no) >> 16 ) ^ random.y ) * (1.0 / 65536.0);
  return float2( E1, E2 );
}

float2 hammersley( uint sample_no, uint num_samples)
{
  return float2( float(sample_no) / float(num_samples), float(reverse_bits32(sample_no)) * 2.3283064365386963e-10 );
}

// Source:
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

float3 hammersley_hemisphere_uniform(uint sample_no, uint num_samples)
{
  float2 h = hammersley(sample_no, num_samples);
  float phi = h.y * 2.0 * 3.14159265;
  float sinPhi, cosPhi;
  sincos(phi, sinPhi, cosPhi);
  float cosTheta = 1.0 - h.x;
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
  return float3(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);
}

float3 hammersley_hemisphere_cos(uint sample_no, uint num_samples)
{
  float2 h = hammersley(sample_no, num_samples);
  float phi = h.y * 2.0 * 3.14159265;
  float sinPhi, cosPhi;
  sincos(phi, sinPhi, cosPhi);
  float cosTheta = sqrt(1.0 - h.x);
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
  return float3(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta);
}

#endif