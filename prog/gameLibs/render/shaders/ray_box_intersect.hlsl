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
  float furthestDistance = min(furthestPlane.x, min(furthestPlane.y, furthestPlane.z));
  float3 closestPlane = min(firstPlaneIntersect, secondPlaneIntersect);
  float closestDistance = max(closestPlane.x, max(closestPlane.y, closestPlane.z));
  return float2(closestDistance, furthestDistance);
}

//return min<max && max>0
float2 rayBoxIntersect(float3 bmin, float3 bmax, float3 wpos, float3 wdir)
{
  float3 cb = (wdir >= 0.0f) ? bmin : bmax;

  float3 invDir = rcp(wdir);
  //float3 l1 = (bmin-wpos)*invDir, l2 = (bmax-wpos)*invDir;
  float3 wposDivDir = -wpos*invDir;
  float3 l1 = bmin*invDir + wposDivDir, l2 = bmax*invDir + wposDivDir;
  float3 lmin = min(l1, l2), lmax = max(l1, l2);
  return float2(max3(lmin.x,lmin.y,lmin.z), min3(lmax.x,lmax.y,lmax.z));
}

#endif