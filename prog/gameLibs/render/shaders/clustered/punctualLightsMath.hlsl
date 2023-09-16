#include <fast_shader_trig.hlsl>

  //Window1 from http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
  float smoothDistanceAtt ( float squaredDistance, float invSqrAttRadius )
  {
    float factor = squaredDistance * invSqrAttRadius ;
    float smoothFactor = saturate (1.0f - factor * factor );
    return smoothFactor * smoothFactor;
  }

  float getDistanceAtt ( float sqrDist, float invSqrAttRadius )
  {
    float attenuation = rcp(max(sqrDist, 0.0001));
    attenuation = saturate(attenuation * smoothDistanceAtt ( sqrDist, invSqrAttRadius ));
    return attenuation;
  }
  float getAngleAtt ( float3 normalizedLightVector, float3 lightDir, float lightAngleScale , float lightAngleOffset)
  {
    // On the CPU
    // float lightAngleScale = 1.0f / max (0.001f, ( cosInner - cosOuter ));
    // float lightAngleOffset = -cosOuter * angleScale ;

    float cd = dot ( lightDir , normalizedLightVector );
    float attenuation = saturate (cd * lightAngleScale + lightAngleOffset );
    // smooth the transition
    return attenuation * attenuation ;
  }


// A right disk is a disk oriented to always face the lit surface .
// Solid angle of a sphere or a right disk is 2 PI (1 - cos( subtended angle )).
// Subtended angle sigma = arcsin (r / d) for a sphere
// and sigma = atan (r / d) for a right disk
// sinSigmaSqr = sin( subtended angle )^2, it is (r^2 / d^2) for a sphere
// and (r^2 / ( r^2 + d ^2) ) for a disk
// cosTheta is not clamped
float illuminanceSphereOrDisk ( float cosTheta , float sinSigmaSqr )
{
  float sinTheta = sqrt (1.0f - cosTheta * cosTheta );

  float illuminance = 0.0f;
  // Note : Following test is equivalent to the original formula .
  // There is 3 phase in the curve : cosTheta > sqrt ( sinSigmaSqr ),
  // cosTheta > -sqrt ( sinSigmaSqr ) and else it is 0
  // The two outer case can be merge into a cosTheta * cosTheta > sinSigmaSqr
  // and using saturate ( cosTheta ) instead .
  if ( cosTheta * cosTheta > sinSigmaSqr )
  {
    illuminance = sinSigmaSqr * saturate ( cosTheta );
  }
  else
  {
    float x = sqrt (1.0f / sinSigmaSqr - 1.0f); // For a disk this simplify to x = d / r
    float y = -x * ( cosTheta / sinTheta );
    float sinThetaSqrtY = sinTheta * sqrt (1.0f - y * y);
    illuminance = ( cosTheta * acosFast4 (y) - x * sinThetaSqrtY ) * sinSigmaSqr + atan (sinThetaSqrtY / x);
    illuminance /= PI;
  }
  return max ( illuminance , 0.0f);
}

float illuminanceSphereAttenuation ( float3 worldNormal, float3 dirToLight, float lightRadius, float sqrDist)
{
  // Sphere evaluation
  float cosTheta = clamp ( dot ( worldNormal, dirToLight), -0.999, 0.999) ; // Clamp to avoid edge case
  // We need to prevent the object penetrating into the surface
  // and we must avoid divide by 0, thus the 0.9999 f

  float sqrLightRadius = lightRadius * lightRadius ;
  float sinSigmaSqr = min( sqrLightRadius / sqrDist, 0.9999f);
  float illuminance = illuminanceSphereOrDisk ( cosTheta , sinSigmaSqr );
  return illuminance;
}


float illuminanceDiskAttenuation ( float3 worldNormal, float3 dirToLight, float3 planeNormal, float lightRadius, float sqrDist)
{
  // Disk evaluation
  float cosTheta = dot ( worldNormal, dirToLight);
  float sqrLightRadius = lightRadius * lightRadius ;
  // Do not let the surface penetrate the light
  float sinSigmaSqr = sqrLightRadius / ( sqrLightRadius + max ( sqrLightRadius , sqrDist ));
  // Multiply by saturate ( dot ( planeNormal , -dirToLight)) to better match ground truth .
  float illuminance = illuminanceSphereOrDisk ( cosTheta , sinSigmaSqr ) * saturate ( dot( planeNormal , -dirToLight));
  return illuminance;
}

void spot_light_params(float3 worldPos, float4 pos_and_radius, float3 light_direction, float lightAngleScale, float lightAngleOffset, out half geomAttenuation, out float3 dirFromLight, out float3 point2light)
{
  point2light = pos_and_radius.xyz-worldPos.xyz;
  float distSqFromLight = dot(point2light, point2light);
  float rcpDistFromLight = rsqrt(0.0000001+distSqFromLight);
  dirFromLight = point2light*rcpDistFromLight;
  float invSqrRad = rcp(pow2(pos_and_radius.w));
  
  geomAttenuation = getDistanceAtt( distSqFromLight, invSqrRad );
  geomAttenuation = geomAttenuation*getAngleAtt ( -dirFromLight, light_direction, lightAngleScale, lightAngleOffset);
}

half areaSphereNormalization( float len, float lightSize, float m )
{
  // Compute the normalization factors.
  // Note: just using sphere normalization (todo: come up with proper disk/plane normalization)
  half dist = saturate(lightSize / len);
  half normFactor = m / ( m + 0.5 * dist );
  return normFactor * normFactor;
}

half3 areaSphereLight(half3 R, half3 L, half m, half lightSize)
{	
  // Intersect the sphere.
  half3 centerDir = L - dot(L, R) * R;
  L = L - centerDir * saturate( lightSize / (length(centerDir)+1e-6) );			
  return L.xyz;
}

half4 SphereAreaLightIntersection( half3 N, half3 V, half3 L, half ggx_alpha, half lightSize )
{
  half4 lightVec = half4(L.xyz, 1.0f);
  half3 R = reflect(V, N);

  lightVec.xyz = areaSphereLight(R, L, ggx_alpha, lightSize);

  // Normalize.
  half len = max(length( lightVec.xyz ),  1e-6);
  lightVec.xyz /= len;

  // Energy normalization
  lightVec.w = areaSphereNormalization( len, lightSize, ggx_alpha );

  return lightVec;
}


