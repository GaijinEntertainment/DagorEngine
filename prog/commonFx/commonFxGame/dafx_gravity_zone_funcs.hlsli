#ifndef DAFX_GRAVITY_ZONE_FUNCS_HLSLI
#define DAFX_GRAVITY_ZONE_FUNCS_HLSLI
#if DAFX_USE_GRAVITY_ZONE

// Based on active_matter/prog/scripts/game/es/grav_zones_common.das

DAFX_INLINE
float3 safe_normalize(float3_cref V)
{
  return V / sqrt(max(dot(V, V), 0.000001f));
}

DAFX_INLINE
float3x3 dafx_build_grav_tm(float3_cref up)
{
  if (up.y >= 0.99)
    return m33_ident();
  if (up.y <= -0.99)
    return m33_from(
      0,  0, 1,
      0, -1, 0,
      1,  0, 0);

  // can use normalize, because zero horizontal vectors got early exit
  float3 forward = normalize(float3(-up.z, 0.f, up.x));
  float3 right = cross(up, forward);
  return m33_from(forward, up, right);
}

struct GravityZoneUnpack {
  float3 center;
  float3 size;
  float3 dir;
  float3x3 itm;
  float weight;
  uint shape;
  uint type;
};


DAFX_INLINE
GravityZoneUnpack unpack_gravity_zone(GravityZoneDescriptor_cref inp)
{
  GravityZoneUnpack res;

  res.center = f16tof32(inp.center__size >> 16U);
  res.size = f16tof32(inp.center__size);
  res.dir = float3(inp.dir_x, inp.dir_y, inp.dir_z);
  res.itm = m33_from(
    f16tof32(inp.itm0__itm1 >> 16U),
    f16tof32(inp.itm0__itm1),
    f16tof32(inp.itm2__extra >> 16U));
  res.shape = inp.itm2__extra.x & 0xFFFFu;
  res.type = inp.itm2__extra.y & 0xFFFFu;
  res.weight = f16tof32(inp.itm2__extra.z);

  return res;
}


DAFX_INLINE
float3 dafx_gravity_force(uint type, float3_cref dir, float3_cref rpos, float3_cref pos_ls)
{
  if (type == GRAVITY_ZONE_TYPE_LINEAR)
    return dir;

  if (type == GRAVITY_ZONE_TYPE_CYLINDRICAL)
    return safe_normalize(dir * pos_ls.z - rpos);

  float3 rdir = safe_normalize(rpos);
  if (type == GRAVITY_ZONE_TYPE_RADIAL)
    return -rdir;
  if (type == GRAVITY_ZONE_TYPE_RADIAL_INVERTED)
    return rdir;

  return float3(0.0f, 0.0f, 0.0f);
}


DAFX_INLINE
float dafx_calc_gravity_for_sphere(float3_cref size, float weight, float dist_sq)
{
  float dist_to_weight_mul = size.x;
  float dist_to_weight_add = size.y;
  float outerRadiusSq = size.z;
  if (dist_sq > 1e-6f && dist_sq < outerRadiusSq) {
    float dist = sqrt(dist_sq);
    return weight * saturate(dist * dist_to_weight_mul + dist_to_weight_add);
  }
  return 0.0f;
}


DAFX_INLINE
float dafx_calc_gravity_for_box(float3_cref size, float weight, float3_cref pos_ls)
{
  if (lessEq(abs(pos_ls), size)) {
    // TODO: weight should interpolate from 0 to zone weight near borders
    // see calc_gravity_for_box in grav_zones_common.das
    return weight;
  }
  return 0.0f;
}


DAFX_INLINE
float dafx_calc_gravity_for_cylinder(float3_cref size, float weight, float3_cref pos_ls)
{
  float length = size.x;
  float radiusSq = size.y;
  bool isOkByLen = abs(pos_ls.z) <= length;
  bool isOkByRad = lengthSq(float2(pos_ls.x, pos_ls.y)) <= radiusSq;
  return isOkByLen && isOkByRad ? weight : 0.0f;
}

// slerp(float3(0, 1, 0), up, t)
DAFX_INLINE
float3 up_slerp(float3_cref up, float t)
{
  if (t >= 0.95)
    return up;

  if (t <= 0.05)
    return float3(0, 1, 0); // Almost up anyway

  if (abs(up.y) >= 0.95) // Cannot lerp opposite vectors
    return t >= 0.5 ? up : float3(0, 1, 0);

  float cos_theta = up.y;
  float theta = acos(cos_theta);
  float xzLen, y;
  sincos(theta * t, xzLen, y);
  // we can use normalize here, because zero vectors got early exit
  float2 hor = normalize(float_xz(up));
  return float3(xzLen * hor.x, y, xzLen * hor.y);
}


DAFX_INLINE
float3 dafx_get_grav_dir(float3_cref wpos, uint gravity_zone_count)
{
  float3 totalUp = float3(0.0, 0.0, 0.0);
  float totalWeight = 0.0;

  for (uint i = 0; i < gravity_zone_count; ++ i) {
    GravityZoneUnpack zone = unpack_gravity_zone(dafx_gravity_zone_buffer[i]);
    float3 rpos = wpos - zone.center;

    float distSq = lengthSq(rpos);
    float weight = 0.0f;

    if (zone.shape == GRAVITY_ZONE_SHAPE_SPHERE) {
      weight = dafx_calc_gravity_for_sphere(zone.size, zone.weight, distSq);
      if (weight <= 0.01) continue;
    }

    // NOTE: matrix is on right side, because it is inverse transform
    float3 posLS = mul(rpos, zone.itm);

    if (zone.shape == GRAVITY_ZONE_SHAPE_BOX) {
      weight = dafx_calc_gravity_for_box(zone.size, zone.weight, posLS);
      if (weight <= 0.01) continue;
    }

    if (zone.shape == GRAVITY_ZONE_SHAPE_CYLINDER) {
      weight = dafx_calc_gravity_for_cylinder(zone.size, zone.weight, posLS);
      if (weight <= 0.01) continue;
    }

    totalWeight += weight;
    totalUp += weight * dafx_gravity_force(zone.type, zone.dir, rpos, posLS);
  }

  return up_slerp(safe_normalize(totalUp), totalWeight);
}


DAFX_INLINE
float3x3 dafx_gravity_zone_tm(float3_cref wpos, uint gravity_zone_count)
{
  BRANCH
  if (gravity_zone_count == 0)
    return m33_ident();

  return dafx_build_grav_tm(dafx_get_grav_dir(wpos, gravity_zone_count));
}


#endif
#endif