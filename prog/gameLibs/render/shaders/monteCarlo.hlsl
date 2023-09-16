#ifndef MONTE_CARLO_INCLUDED
#define MONTE_CARLO_INCLUDED 1

float3 tangent_to_world( float3 vec, float3 tangentZ )
{
  float3 up = abs(tangentZ.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
  float3 tangentX = normalize( cross( up, tangentZ ) );
  float3 tangentY = cross( tangentZ, tangentX );
  return tangentX * vec.x + tangentY * vec.y + tangentZ * vec.z;
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

// http://jcgt.org/published/0007/04/01/paper.pdf by Eric Heitz
// Input Ve: view direction
// Input alpha_x, alpha_y: ggxroughness parameters
// Input U1, U2: uniform random numbers
// Output Ne: normal sampled with PDF D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
float3 SampleGGXVNDF(float3 Ve, float alpha_x, float alpha_y, float U1, float U2)
{
  // Section 3.2: transforming the view direction to the hemisphere configuration
  float3 Vh = normalize(float3(alpha_x * Ve.x, alpha_y * Ve.y, Ve.z));
  // Section 4.1: orthonormal basis (with special case if cross product is zero)
  float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
  float3 T1 = lensq > 0 ? float3(-Vh.y, Vh.x, 0) * rsqrt(lensq) : float3(1, 0, 0);
  float3 T2 = cross(Vh, T1);
  // Section 4.2: parameterization of the projected area
  float r = sqrt(U1);
  float phi = 2.0 * PI * U2;
  float t1 = r * cos(phi);
  float t2 = r * sin(phi);
  float s = 0.5 * (1.0 + Vh.z);
  t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;
  // Section 4.3: reprojection onto hemisphere
  float3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;
  // Section 3.4: transforming the normal back to the ellipsoid configuration
  float3 Ne = normalize(float3(alpha_x * Nh.x, alpha_y * Nh.y, max(0.0, Nh.z)));
  return Ne;
}

float3 importance_sample_GGX_VNDF( float2 E, float3 Ve, float linear_roughness )
{
  float ggx_alpha = max(1e-4, linear_roughness*linear_roughness);
  return SampleGGXVNDF(Ve, ggx_alpha, ggx_alpha, E.x, E.y);
}

#endif