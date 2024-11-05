#include <fast_shader_trig.hlsl>

  //Window1 from http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
  half smoothDistanceAtt ( half squaredDistance, half invSqrAttRadius )
  {
    half factor = squaredDistance * invSqrAttRadius ;
    half smoothFactor = saturate (1.0h - factor * factor );
    return smoothFactor * smoothFactor;
  }


  half getDistanceAtt ( half sqrDist, half invSqrAttRadius )
  {
    half attenuation = rcp(max(sqrDist, 0.0001h));
    attenuation = saturate(attenuation * smoothDistanceAtt ( sqrDist, invSqrAttRadius ));
    return attenuation;
  }
  half getAngleAtt ( half3 normalizedLightVector, half3 lightDir, half lightAngleScale , half lightAngleOffset)
  {
    // On the CPU
    // float lightAngleScale = 1.0f / max (0.001f, ( cosInner - cosOuter ));
    // float lightAngleOffset = -cosOuter * angleScale ;

    half cd = dot ( lightDir , normalizedLightVector );
    half attenuation = saturate (cd * lightAngleScale + lightAngleOffset );
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
half illuminanceSphereOrDisk ( half cosTheta , half sinSigmaSqr )
{
  half sinTheta = sqrt (1.0h - cosTheta * cosTheta );

  half illuminance = 0.0h;
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
    half x = sqrt (1.0h / sinSigmaSqr - 1.0h); // For a disk this simplify to x = d / r
    half y = -x * ( cosTheta / sinTheta );
    half sinThetaSqrtY = sinTheta * sqrt (1.0h - y * y);
    illuminance = ( cosTheta * acosFast4 (y) - x * sinThetaSqrtY ) * sinSigmaSqr + atan (sinThetaSqrtY / x);
    illuminance /= PI;
  }
  return max ( illuminance , 0.0h);
}

half illuminanceSphereAttenuation ( half3 worldNormal, half3 dirToLight, half lightRadius, half sqrDist)
{
  // Sphere evaluation
  half cosTheta = clamp ( dot ( worldNormal, dirToLight), -0.999h, 0.999h) ; // Clamp to avoid edge case
  // We need to prevent the object penetrating into the surface
  // and we must avoid divide by 0, thus the 0.9999 f

  half sqrLightRadius = lightRadius * lightRadius ;
  half sinSigmaSqr = min( sqrLightRadius / sqrDist, 0.9999h);
  half illuminance = illuminanceSphereOrDisk ( cosTheta , sinSigmaSqr );
  return illuminance;
}


half illuminanceDiskAttenuation ( half3 worldNormal, half3 dirToLight, half3 planeNormal, half lightRadius, half sqrDist)
{
  // Disk evaluation
  half cosTheta = dot ( worldNormal, dirToLight);
  half sqrLightRadius = lightRadius * lightRadius ;
  // Do not let the surface penetrate the light
  half sinSigmaSqr = sqrLightRadius / ( sqrLightRadius + max ( sqrLightRadius , sqrDist ));
  // Multiply by saturate ( dot ( planeNormal , -dirToLight)) to better match ground truth .
  half illuminance = illuminanceSphereOrDisk ( cosTheta , sinSigmaSqr ) * saturate ( dot( planeNormal , -dirToLight));
  return illuminance;
}

void spot_light_params(float3 worldPos, float4 pos_and_radius, half3 light_direction, half lightAngleScale, half lightAngleOffset, out half geomAttenuation, out half3 dirFromLight, out half3 point2light)
{
  point2light = half3(pos_and_radius.xyz-worldPos.xyz);
  half distSqFromLight = dot(point2light, point2light);
  half rcpDistFromLight = rsqrt(0.0000001h+distSqFromLight);
  dirFromLight = point2light*rcpDistFromLight;
  half invSqrRad = rcp(pow2(half(pos_and_radius.w)));

  geomAttenuation = getDistanceAtt( distSqFromLight, invSqrRad );
  geomAttenuation = geomAttenuation*getAngleAtt ( -dirFromLight, light_direction, lightAngleScale, lightAngleOffset);
}

half areaSphereNormalization( half len, half lightSize, half m )
{
  // Compute the normalization factors.
  // Note: just using sphere normalization (todo: come up with proper disk/plane normalization)
  half dist = saturate(lightSize / len);
  half normFactor = m / ( m + 0.5h * dist );
  return normFactor * normFactor;
}

half3 areaSphereLight(half3 R, half3 L, half m, half lightSize)
{
  // Intersect the sphere.
  half3 centerDir = L - dot(L, R) * R;
  L = L - centerDir * saturate( lightSize / (length(centerDir)+1e-6h) );
  return L.xyz;
}

half4 SphereAreaLightIntersection( half3 N, half3 V, half3 L, half ggx_alpha, half lightSize )
{
  half4 lightVec = half4(L.xyz, 1.0h);
  half3 R = reflect(V, N);

  lightVec.xyz = areaSphereLight(R, L, ggx_alpha, lightSize);

  // Normalize.
  half len = max(length( lightVec.xyz ),  1e-6h);
  lightVec.xyz /= len;

  // Energy normalization
  lightVec.w = areaSphereNormalization( len, lightSize, ggx_alpha );

  return lightVec;
}


