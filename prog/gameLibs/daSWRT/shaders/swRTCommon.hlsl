#ifndef RT_COMMON_HLSLI
#define RT_COMMON_HLSLI 1

#define kEpsilon 0.00001

bool RayIntersectsBoxT0T1(float3 t0, float3 t1, float ray_extent)
{
  const float3 tmax = max(t0, t1);
  const float3 tmin = min(t0, t1);

  const float a1 = min( min(tmax.x,tmax.y), min(tmax.z, ray_extent));
  const float a0 = max( max(tmin.x,tmin.y), max(tmin.z, 0.0f) );

  return a1 >= a0;
}

bool RayIntersectsBoxT0TInf(float3 t0, float3 t1)
{
  const float3 tmax = max(t0, t1);
  const float3 tmin = min(t0, t1);

  const float a1 = min3(tmax.x, tmax.y, tmax.z);
  const float a0 = max( max(tmin.x,tmin.y), max(tmin.z, 0.0f) );

  return a1 >= a0;
}

bool RayIntersectsBoxT0T1(float3 t0, float3 t1, float ray_extent, inout float t)
{
  const float3 tmax = max(t0, t1);
  const float3 tmin = min(t0, t1);

  const float a1 = min( min(tmax.x,tmax.y), min(tmax.z, ray_extent));
  const float a0 = max( max(tmin.x,tmin.y), max(tmin.z, 0.0f) );
  t = a0;
  return a1 >= a0;
}

bool RayIntersectsBox(float3 origin, float3 rayDirInv, float3 BboxMin, float3 BboxMax, float ray_extent)
{
  return RayIntersectsBoxT0T1((BboxMin - origin) * rayDirInv, (BboxMax - origin) * rayDirInv, ray_extent);
}

bool RayIntersectsBoxInf(
    float3 ray_origin,
    float3 ray_inv_dir,
    float3 bmin,
    float3 bmax)
{
  // When an element of Direction is 0, its inverse needs to be NAN rather than INF.
  // IEEE float math won't propogate NANs, so this element won't affect min_t or max_t.

  float3 t_plane_min = (bmin - ray_origin) * ray_inv_dir;
  float3 t_plane_max = (bmax - ray_origin) * ray_inv_dir;

  float3 min_interval = select(ray_inv_dir > 0.0, t_plane_min, t_plane_max);
  float3 max_interval = select(ray_inv_dir > 0.0, t_plane_max, t_plane_min);

  float min_t = max(max(min_interval.x, min_interval.y), max(min_interval.z, 0.0));
  float max_t = min3(max_interval.x, max_interval.y, max_interval.z);
  return min_t <= max_t;
}

bool RayIntersectsBoxT(
    float3 ray_origin,
    float3 ray_inv_dir,
    float3 bmin,
    float3 bmax,
    float ray_extent,
    out float t)
{
  // When an element of Direction is 0, its inverse needs to be NAN rather than INF.
  // IEEE float math won't propogate NANs, so this element won't affect min_t or max_t.

  float3 t_plane_min = (bmin - ray_origin) * ray_inv_dir;
  float3 t_plane_max = (bmax - ray_origin) * ray_inv_dir;

  float3 min_interval = select(ray_inv_dir > 0.0, t_plane_min, t_plane_max);
  float3 max_interval = select(ray_inv_dir > 0.0, t_plane_max, t_plane_min);

  float min_t = max(max(min_interval.x, min_interval.y), max(min_interval.z, 0.0));
  float max_t = min(min(max_interval.x, max_interval.y), min(max_interval.z, ray_extent));
  t = min_t;
  return min_t <= max_t;
}

float RayIntersectsBoxInfT(
    float3 ray_origin,
    float3 ray_inv_dir,
    float3 bmin,
    float3 bmax)
{
  // When an element of Direction is 0, its inverse needs to be NAN rather than INF.
  // IEEE float math won't propogate NANs, so this element won't affect min_t or max_t.

  float3 t_plane_min = (bmin - ray_origin) * ray_inv_dir;
  float3 t_plane_max = (bmax - ray_origin) * ray_inv_dir;

  float3 min_interval = select(ray_inv_dir > 0.0, t_plane_min, t_plane_max);
  float3 max_interval = select(ray_inv_dir > 0.0, t_plane_max, t_plane_min);

  float min_t = max(max(min_interval.x, min_interval.y), max(min_interval.z, 0.0));
  float max_t = min3(max_interval.x, max_interval.y, max_interval.z);
  return min_t <= max_t ? min_t : -1;
}

bool RayIntersectsBoxTNormal(
    float3 ray_origin,
    float3 ray_inv_dir,
    float3 bmin,
    float3 bmax,
    inout float t,
    out float3 normal)
{
  // When an element of Direction is 0, its inverse needs to be NAN rather than INF.
  // IEEE float math won't propogate NANs, so this element won't affect min_t or max_t.

  float3 t_plane_min = (bmin - ray_origin) * ray_inv_dir;
  float3 t_plane_max = (bmax - ray_origin) * ray_inv_dir;

  float3 min_interval = select(ray_inv_dir > 0.0, t_plane_min, t_plane_max);
  float3 max_interval = select(ray_inv_dir > 0.0, t_plane_max, t_plane_min);

  float min_t = max(max(min_interval.x, min_interval.y), max(min_interval.z, 0.0));
  float max_t = min(min(max_interval.x, max_interval.y), min(max_interval.z, t));
  t = min_t;
  normal = -(min_t == min_interval.x ? float3(sign(ray_inv_dir.x),0,0) : min_t == min_interval.y ? float3(0, sign(ray_inv_dir.y),0) : float3(0, 0, sign(ray_inv_dir.z)));
  return min_t <= max_t;
}


bool RayIntersectsBoxTNormal(
    float3 ray_origin,
    float3 ray_inv_dir,
    float3 bmin,
    float3 bmax,
    float ray_extent,
    inout float t,
    inout float3 normal)
{
  // When an element of Direction is 0, its inverse needs to be NAN rather than INF.
  // IEEE float math won't propogate NANs, so this element won't affect min_t or max_t.

  float3 t_plane_min = (bmin - ray_origin) * ray_inv_dir;
  float3 t_plane_max = (bmax - ray_origin) * ray_inv_dir;

  float3 min_interval = select(ray_inv_dir > 0.0, t_plane_min, t_plane_max);
  float3 max_interval = select(ray_inv_dir > 0.0, t_plane_max, t_plane_min);

  float min_t = max(max(min_interval.x, min_interval.y), max(min_interval.z, 0.0));
  float max_t = min(min(max_interval.x, max_interval.y), min(max_interval.z, ray_extent));
  FLATTEN
  if (min_t <= max_t)
  {
    t = min_t;
    normal = -(min_t == min_interval.x ? float3(sign(ray_inv_dir.x),0,0) : min_t == min_interval.y ? float3(0, sign(ray_inv_dir.y),0) : float3(0, 0, sign(ray_inv_dir.z)));
  }
  return min_t <= max_t;
}


//Adapted from https://github.com/kayru/RayTracedShadows/blob/master/Source/Shaders/RayTracedShadows.comp
bool RayTriangleIntersect(
    float3 orig,
    float3 dir,
    float3 v0,
    float3 e0,
    float3 e1,
    inout float t,
    out float2 bc)
{
  float3 s1 = cross(dir.xyz, e1);
  float det  = dot(s1, e0);
  float invd = rcp(det);
  float3 d = orig.xyz - v0;
  bc.x = dot(d, s1) * invd;
  const float3 s2 = cross(d, e0);
  bc.y = dot(dir.xyz, s2) * invd;
  float ct = dot(e1, s2) * invd;
  FLATTEN
  if (
#if BACKFACE_CULLING
    det < -kEpsilon ||
#endif
    bc.x < 0.0 || bc.y < 0.0 || (bc.x + bc.y) > 1.0 || ct < 0.0 || ct >= t)
  {
    return false;
  }
  else
  {
    t = ct;
    return true;
  }
}

bool RayTriangleIntersectShadow(
    float3 orig,
    float3 dir,
    float3 v0,
    float3 e0,
    float3 e1)
{
  float3 s1 = cross(dir.xyz, e1);
  float  invd = rcp(dot(s1, e0));
  float3 d = orig.xyz - v0;
  float2 bc;
  bc.x = dot(d, s1) * invd;
  const float3 s2 = cross(d, e0);
  bc.y = dot(dir.xyz, s2) * invd;
  float ct = dot(e1, s2) * invd;
  return bc.x >= 0.0 & bc.y >= 0.0 & (bc.x + bc.y) <= 1.0 & ct >= 0.0;
}

#endif