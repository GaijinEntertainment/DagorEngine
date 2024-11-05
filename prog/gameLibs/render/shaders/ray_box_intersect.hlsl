#ifndef RAY_BOX_INTERSECT_HLSL_INCLUDED
#define RAY_BOX_INTERSECT_HLSL_INCLUDED 1
//localOrigin = rayOrigin - boxCenter
//boxW = (box.bmax-box.bmin)/2;
//returns min, max diapazon
// if min>max - there is no intersection
// otherwise it is intersection with inifinite ray
// to test semi infinite ray, check for furthestDistance > 0
//
float2 rayBoxIntersect(float3 boxExt, float3 localOrigin, float3 localRay)
{
  float3 firstPlaneIntersect    = (boxExt - localOrigin) / localRay;
  float3 secondPlaneIntersect = (-boxExt - localOrigin) / localRay;

  float3 furthestPlane = max(firstPlaneIntersect, secondPlaneIntersect);
  float3 closestPlane = min(firstPlaneIntersect, secondPlaneIntersect);
  float furthestDistance = min3(furthestPlane.x, furthestPlane.y, furthestPlane.z);
  float closestDistance = max3(closestPlane.x, closestPlane.y, closestPlane.z);
  return float2(closestDistance, furthestDistance);
}

//return min<max && max>0
float2 rayBoxIntersect(float3 bmin, float3 bmax, float3 wpos, float3 wdir)
{
  float3 boxO = 0.5*(bmin + bmax);
  return rayBoxIntersect(bmax-boxO, wpos-boxO, wdir);
}

#endif