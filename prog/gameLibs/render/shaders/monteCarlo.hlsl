#ifndef MONTE_CARLO_INCLUDED
#define MONTE_CARLO_INCLUDED 1

float3 tangent_to_world( float3 vec, float3 tangentZ )
{
  float3 up = abs(tangentZ.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
  float3 tangentX = normalize( cross( up, tangentZ ) );
  float3 tangentY = cross( tangentZ, tangentX );
  return tangentX * vec.x + tangentY * vec.y + tangentZ * vec.z;
}

float3x3 tangent_to_world_matrix( float3 tangentZ )
{
  float3 up = abs(tangentZ.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
  float3 tangentX = normalize( cross( up, tangentZ ) );
  float3 tangentY = cross( tangentZ, tangentX );
  float3x3 r;
  r[0] = tangentX;
  r[1] = tangentY;
  r[2] = tangentZ;
  return r;
}

float3x3 create_tbn_matrix(float3 N)
{
  float3 U;
  if (abs(N.z) > 0.0) {
    float k = sqrt(N.y * N.y + N.z * N.z);
    U.x = 0.0; U.y = -N.z / k; U.z = N.y / k;
  }
  else
  {
    float k = sqrt(N.x * N.x + N.y * N.y);
    U.x = N.y / k; U.y = -N.x / k; U.z = 0.0;
  }

  float3x3 TBN;
  TBN[0] = U;
  TBN[1] = cross(N, U);
  TBN[2] = N;
  return transpose(TBN);
}

float4 uniform_sample_sphere( float2 E )
{
  float phi = 2 * PI * E.y;
  float cosTheta = 1 - 2 * E.x;
  float sinTheta = sqrt( 1 - cosTheta * cosTheta );

  float3 H;
  H.x = sinTheta * cos( phi );
  H.y = sinTheta * sin( phi );
  H.z = cosTheta;

  float PDF = 1.0 / (4 * PI);

  return float4( H, PDF );
}

float4 uniform_sample_hemisphere( float2 E )
{
  float phi = 2 * PI * E.y;
  float cosTheta = 1 - E.x;
  float sinTheta = sqrt( 1 - cosTheta * cosTheta );

  float3 H;
  H.x = sinTheta * cos( phi );
  H.y = sinTheta * sin( phi );
  H.z = cosTheta;

  float PDF = 1.0 / (2 * PI);

  return float4( H, PDF );
}

float2 uniform_sample_disk(float2 Random)
{
  float theta = 2.0f * (float)PI * Random.y;
  float Radius = sqrt(Random.x);
  return float2(Radius * cos(theta), Radius * sin(theta));
}

float4 cosine_sample_hemisphere( float2 E )
{
  float phi = 2 * PI * E.y;
  float cosTheta = sqrt( 1 - E.x );
  float sinTheta = sqrt( 1 - cosTheta * cosTheta );

  float3 H;
  H.x = sinTheta * cos( phi );
  H.y = sinTheta * sin( phi );
  H.z = cosTheta;

  float PDF = cosTheta / PI;

  return float4( H, PDF );
}

float4 uniform_sample_cone( float2 E, float CosThetaMax )
{
  float phi = 2 * PI * E.y;
  float cosTheta = lerp( 1.f, CosThetaMax, E.x );
  float sinTheta = sqrt( 1 - cosTheta * cosTheta );

  float3 L;
  L.x = sinTheta * cos( phi );
  L.y = sinTheta * sin( phi );
  L.z = cosTheta;

  float PDF = 1.0 / ( 2 * PI * (1 - CosThetaMax) );

  return float4( L, PDF );
}

float4 importance_sample_Blinn( float2 E, float linear_roughness )
{
  float m = max(1e-4f, linear_roughness * linear_roughness);
  float n = 2 / (m*m) - 2;

  float phi = 2 * PI * E.y;
  float cosTheta = clampedPow( E.x, 1 / (n + 1) );
  float sinTheta = sqrt( 1 - cosTheta * cosTheta );

  float3 H;
  H.x = sinTheta * cos( phi );
  H.y = sinTheta * sin( phi );
  H.z = cosTheta;

  float D = (n+2)/ (2*PI) * clampedPow( cosTheta, n );
  float PDF = D * cosTheta;

  return float4( H, PDF );
}

float4 importance_sample_GGX_NDF( float2 E, float linear_roughness )
{
  float m = max(1e-4f, linear_roughness * linear_roughness);
  float m2 = m * m;

  float phi = 2 * PI * E.y;
  float cosTheta = sqrt( (1 - E.x) / ( 1 + (m2 - 1) * E.x ) );
  float sinTheta = sqrt( 1 - cosTheta * cosTheta );

  float3 H;
  H.x = sinTheta * cos( phi );
  H.y = sinTheta * sin( phi );
  H.z = cosTheta;

  float d = ( cosTheta * m2 - cosTheta ) * cosTheta + 1;
  float D = m2 / ( PI*d*d );
  float PDF = D * cosTheta;

  return float4( H, PDF );
}

// Sampling Visible GGX Normals with Spherical Caps
// https://arxiv.org/pdf/2306.05044
float3 importanceSampleVNDF_Hemisphere(float2 u, float3 wi)
{
  // sample a spherical cap in (-wi.z, 1]
  float phi = 2.0f * PI * u.x;
  //original: fma((1.0f - u.y), (1.0f + wi.z), -wi.z), so:
  // (1.0f + wi.z)*(1.0f - u.y) - wi.z
  // (1.0f - u.y) - u.y*wi.z
  // -wi.z*u.y + 1*(1-u.y) == lerp(-wi.z, 1, u.y)
  float z = lerp(-wi.z, 1.0, u.y);
  float sinTheta = sqrt(saturate(1.0f - z * z));
  float x = sinTheta * cos(phi);
  float y = sinTheta * sin(phi);
  float3 c = float3(x, y, z);
  // compute halfway direction;
  float3 h = c + wi;
  // return without normalization (as this is done later)
  return h;
}

float3 importanceSampleVNDF_GGX_CAPPED(float2 u, float3 wi, float2 ggx_alpha)
{
  float3 wiStd = normalize(float3(wi.xy * ggx_alpha, wi.z));
  float3 wmStd = importanceSampleVNDF_Hemisphere(u, wiStd);
  return normalize(float3(wmStd.xy * ggx_alpha, wmStd.z));
}

// https://gpuopen.com/download/publications/Bounded_VNDF_Sampling_for_Smith-GGX_Reflections.pdf
// in tangent space
float3 sampleGGXVNDFBounded(float ggx_alpha, float3 localView, float2 random)
{
  // Stretch the view vector as if roughness==1
  float3 wiStd = normalize(float3(ggx_alpha * localView.xy, localView.z));

  float phi = 2.0f * PI * random.y;
  float a = ggx_alpha; // Use a = saturate(min(ggx_alpha.x, ggx_alpha.y)) for anisotropic roughness.
  float s = 1.0f + sign(1.0f - ggx_alpha) * length(float2(localView.x, localView.y));
  float a2 = a * a;
  float s2 = s * s;
  float k = (1.0f - a2) * s2 / (s2 + a2 * localView.z * localView.z);
  float b = localView.z > 0.0f ? k * wiStd.z : wiStd.z;

  float z = mad(-b, random.x, 1.0f - random.x);
  float sinTheta = sqrt(saturate(1.0f - z * z));
  float x = sinTheta * cos(phi);
  float y = sinTheta * sin(phi);
  float3 c = float3(x, y, z);
  float3 wmStd = c + wiStd;

  // Convert normal to un-stretched and normalise
  return normalize(float3(ggx_alpha * wmStd.xy, wmStd.z));
}

// https://gpuopen.com/download/publications/Bounded_VNDF_Sampling_for_Smith-GGX_Reflections.pdf
// in tangent space

float sampleGGXVNDFBoundedPDF(float ggx_alpha, float dotNH, float3 localView)
{
  float a2 = ggx_alpha*ggx_alpha;
  // GGX distribution
  float d = ( dotNH * a2 - dotNH) * dotNH + 1;
  float ndf = half(a2 / max(1e-8, d * d)) * 1./PI;

  float2 ai = ggx_alpha * localView.xy;
  float len2 = dot(ai, ai);
  float t = sqrt(len2 + localView.z * localView.z);
  if (localView.z >= 0.0f)
  {
    float a = ggx_alpha; // Use a = saturate(min(roughnessAlpha.x, roughnessAlpha.y)) for anisotropic roughness.
    float s = 1.0f + sign(1.0f - a) * length(float2(localView.x, localView.y));
    float s2 = s * s;
    float k = (1.0f - a2) * s2 / (s2 + a2 * localView.z * localView.z);
    return ndf / (2.0f * (k * localView.z + t));
  }
  return ndf * (t - localView.z) / (2.0f * len2);
}


#if 0
float3 importance_sample_GGX_VNDF( float2 E, float3 Ve, float linear_roughness )
{
  float ggx_alpha = max(1e-4, linear_roughness*linear_roughness);
  return importanceSampleVNDF_GGX_CAPPED(E, Ve, ggx_alpha);
}
#else
float3 importance_sample_GGX_VNDF( float2 E, float3 Ve, float linear_roughness )
{
  float ggx_alpha = max(1e-4, linear_roughness*linear_roughness);
  return sampleGGXVNDFBounded(ggx_alpha, Ve, E);
}
#endif

#endif