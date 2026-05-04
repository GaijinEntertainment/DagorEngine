#ifndef DAGI_INVALIDATION_FRUSTUM_DIST_HLSL
#define DAGI_INVALIDATION_FRUSTUM_DIST_HLSL 1
float getInvalidFrustumOuterDistanceNorm(float3 p, float4 planes[6])
{
  float4 dist03 = min(p.x * planes[0] + p.y * planes[1] + p.z * planes[2] + planes[3], 0.0);
  float2 dist45 = min(float2(dot(float4(p, 1), planes[4]), dot(float4(p, 1), planes[5])), 0.0);
  return sqrt(dot(dist03, dist03) + dot(dist45, dist45));
}
#endif
