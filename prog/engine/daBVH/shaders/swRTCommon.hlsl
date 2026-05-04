#ifndef RT_COMMON_HLSLI
#define RT_COMMON_HLSLI 1

#define kEpsilon 0.00001
#define TRACE_EPSILON          0.0000004f
#define TRACE_ONE_PLUS_EPSILON (1.f + TRACE_EPSILON)
#define TRACE_ONE_PLUS_EPSILON2 (1.f + 2.f*TRACE_EPSILON)

bool RayIntersectsBoxT0T1(float3 t0, float3 t1, float ray_extent)
{
  const float3 tmax = max(t0, t1);
  const float3 tmin = min(t0, t1);

  const float a1 = min( min(tmax.x,tmax.y), min(tmax.z, ray_extent));
  const float a0 = max( max(tmin.x,tmin.y), max(tmin.z, 0.0f) );

  return a1 >= a0;
}

bool RayIntersectsBoxInf(float3 t0, float3 t1)
{
  const float3 tmax = max(t0, t1);
  const float3 tmin = min(t0, t1);

  const float a1 = min3(tmax.x, tmax.y, tmax.z);
  const float a0 = max3(tmin.x, tmin.y, tmin.z);

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



// watertight edge equation
//doesn't return false if t < 0. Check yourself
bool RayTriangleIntersectInf(
  in  float3 rayOrigin,
  in  float3 rayDirection,
  in  float3 v0, in  float3 v1, in  float3 v2,
  inout float  t,
  inout float3  bary)
{
  // 1. Select dominant axis (kz) to minimize projection error
  float3 absDir = abs(rayDirection);
  uint kz, kx, ky;

  float3 a = v0 - rayOrigin;
  float3 b = v1 - rayOrigin;
  float3 c = v2 - rayOrigin;
  float3 d = rayDirection.xyz;

  // Permute coordinates so z becomes the new z-axis (dominant)
  if (absDir.x >= absDir.y && absDir.x >= absDir.z)
  {
    d = rayDirection.yzx;
    a = a.yzx;
    b = b.yzx;
    c = c.yzx;
  }
  else if (absDir.y >= absDir.z)
  {
    d = rayDirection.zxy;
    a = a.zxy;
    b = b.zxy;
    c = c.zxy;
  }

  // 2. Shear transformation (projects away from dominant axis)
  float3 S = float3(-d.xy, 1.f) * rcp(d.z);

  float3 abcZ = float3(a.z, b.z, c.z);
  float2 axy = a.xy + S.xy * abcZ.x;
  float2 bxy = b.xy + S.xy * abcZ.y;
  float2 cxy = c.xy + S.xy * abcZ.z;

  // 3. 2D edge functions (oriented area tests)
  float3 UVW;
  UVW.x = cxy.x * bxy.y - cxy.y * bxy.x;
  UVW.y = axy.x * cxy.y - axy.y * cxy.x;
  UVW.z = bxy.x * axy.y - bxy.y * axy.x;

  // All three must have the same sign (or zero) -> inside projected triangle
  // We allow exactly zero to support watertight edges/vertices
  if (any(UVW < 0.0) & any(UVW > 0.0)) return false;

  float det = dot(UVW, 1);
  if (abs(det) <= 1e-18)
    return false;               // degenerate projected triangle

  // Optional backface culling (using original normal)
#if BACKFACE_CULLING
  {
    if (dot(cross(v1-v0, v2-v0), rayDirection) >= 0.0) return false;
  }
#endif
  // 5. Output results
  bary = UVW/det;
  // baryW = W * rcpDet;  // usually not needed
  // 4. Depth along ray (in sheared space)
  t = dot(abcZ, bary) * S.z;
  return true;
}

//Adapted from https://github.com/kayru/RayTracedShadows/blob/master/Source/Shaders/RayTracedShadows.comp
bool RayTriangleIntersect(
    float3 orig,
    float3 dir,
    float3 v0,
    float3 v1,
    float3 v2,
    inout float t,
    out float2 bc)
{
  #if USE_WATERTIGHT_TRACE
  float T;
  if (RayTriangleIntersectInf(orig, dir, v0, v1, v2, T, bc))
  {
    if ( T <= 0 || T >= t)
      return false;
    t = T;
    return true;
  }
  return false;
  #endif
  float3 e0 = v1-v0, e1 = v2-v0;
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
    bc.x < -TRACE_EPSILON || bc.y < -TRACE_EPSILON || (bc.x + bc.y) > TRACE_ONE_PLUS_EPSILON2 || ct < 0.0 || ct >= t)
  {
    return false;
  }
  else
  {
    t = ct;
    return true;
  }
}

bool RayTriangleIntersectInfXZ(
    float3 rayOrigin,
    float3 v0,
    float3 v1,
    float3 v2,
    inout float t,
    inout float3 bc)
{
  float3 v012y = float3(v0.y, v1.y, v2.y);
  float3 abc_x = float3(v0.z, v1.z, v2.z) - rayOrigin.z;
  float3 abc_y = float3(v0.x, v1.x, v2.x) - rayOrigin.x;
  // 3. 2D edge functions (oriented area tests)
  float3 UVW = abc_x.zxy*abc_y.yzx - abc_y.zxy*abc_x.yzx;

  // All three must have the same sign (or zero) -> inside projected triangle
  // We allow exactly zero to support watertight edges/vertices
  if (any(UVW < 0.f) & any(UVW > 0.f))
    return false;

  float det = UVW.x + UVW.y + UVW.z;
  if (abs(det) <= 1e-9f) return false;               // degenerate projected triangle

  bc = UVW/det;
  t = dot(v012y, bc);
  return true;
}

bool RayTriangleIntersectShadow(
    float3 orig,
    float3 dir,
    float3 v0,
    float3 v1,
    float3 v2)
{
  float3 e0 = v1-v0, e1 = v2-v0;
  float3 s1 = cross(dir.xyz, e1);
  float  invd = rcp(dot(s1, e0));
  float3 d = orig.xyz - v0;
  float2 bc;
  bc.x = dot(d, s1) * invd;
  const float3 s2 = cross(d, e0);
  bc.y = dot(dir.xyz, s2) * invd;
  float ct = dot(e1, s2) * invd;
  return bc.x >= -TRACE_EPSILON & bc.y >= -TRACE_EPSILON & (bc.x + bc.y) <= TRACE_ONE_PLUS_EPSILON2 & ct >= 0.0;
}

#endif