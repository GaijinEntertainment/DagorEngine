#ifndef PANORAMA_UTIL_HLSL
#define PANORAMA_UTIL_HLSL

#ifdef __cplusplus
#define INLINE inline
#else
#define INLINE
#endif

#define PANORAMA_MIN_CLOUD_DISTANCE 1000.0f

INLINE
float distance_to_clouds(float3 origin, float3 view, float cloud_radius_km, float clouds_layer_sq, float planet_radius_km)
{
  float originInKm = origin.y*0.001f;
  float radius_to_camera_offseted = (planet_radius_km+originInKm);
  float Rh = min(radius_to_camera_offseted, cloud_radius_km);
  float b=Rh*view.y;
  float c=radius_to_camera_offseted*radius_to_camera_offseted-clouds_layer_sq;//Rh*Rh-RH*RH
  float b24c=b*b-c;
  return (-b+sqrt(b24c))*1000;//km to meters
}

INLINE
float3 get_panorama_reprojected_view(float3 origin, float3 view, float3 panorama_world_pos, float reprojection_strength, float cloud_radius_km, float clouds_layer_sq, float planet_radius_km)
{
  float cloudDist = max(PANORAMA_MIN_CLOUD_DISTANCE, distance_to_clouds(origin, view, cloud_radius_km, clouds_layer_sq, planet_radius_km));
  return normalize(view*cloudDist + (origin - panorama_world_pos)*reprojection_strength);
}

float3 get_panorama_inv_reprojection(float3 origin, float3 inv_view, float3 panorama_world_pos, float reprojection_strength, float cloud_radius_km, float clouds_layer_sq, float planet_radius_km)
{
  float3 refPoint = origin - (origin - panorama_world_pos)*reprojection_strength;
  float cloudDist = distance_to_clouds(refPoint, inv_view, cloud_radius_km, clouds_layer_sq, planet_radius_km);

  // Inverse of the reprojection to the extension sphere of PANORAMA_MIN_CLOUD_DISTANCE meters around the origin
  // If the 't' parameter is positive, cloudDist needs to be increased by it to properly extend the cloud sphere surface
  // The panorama center is assumed to be close enough to the origin, that no double intersection happens with the cloud surface
  // If there is no real-value solution, no need to extend the cloudDist
  // Solution for: dot(refPoint + (cloudDist+t) * inv_view - origin, refPoint + (cloudDist+t) * inv_view - origin) = PANORAMA_MIN_CLOUD_DISTANCE^2
  float b = 2.0*dot(inv_view, refPoint-origin);
  float c = dot(refPoint-origin, refPoint-origin) - PANORAMA_MIN_CLOUD_DISTANCE*PANORAMA_MIN_CLOUD_DISTANCE;
  float D = b*b - 4.0*c;
  if (D >= 0)
  {
    // T = (cloudDist+t)
    float T = (-b + sqrt(D)) / 2.0;
    cloudDist = max(cloudDist, T);
  }
  return normalize(inv_view*cloudDist + refPoint - origin);
}

#endif