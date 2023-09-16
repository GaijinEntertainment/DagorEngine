module ang shared
require math
require %dasGameLibs.math.base
require DagorMath
require DagorMathUtils


[generic]
def safe_acos(x : float) : float
  return acos(clamp(x, -1.0, 1.0))

[generic]
def clamp_s_ang(angle, baseAngle, minDelta, maxDelta : float) : float
  let delta = norm_s_ang(baseAngle - angle)
  return angle + min(-min(minDelta, 0.f) + delta, 0.f) - min(max(maxDelta, 0.f) - delta, 0.f)

[generic]
def clamp_s_ang(angle : float;
                min_angle : float;
                max_angle : float) : float
  let normMin = norm_s_ang(min_angle - angle)
  let normMax = norm_s_ang(max_angle - angle)
  return (normMin <= 0.f && normMax >= 0.f) ? angle : (abs(normMax) - abs(normMin) >= 0.f ?  min_angle : max_angle)

[generic]
def clamp_angles_by_dir(angles : float2; max_angles : float4; dir : float3) : float2
  let maxAngleX = min(deg_to_rad(abs(max_angles.x)), SAFE_HALF_PI)
  let minAngleX = min(deg_to_rad(abs(max_angles.y)), SAFE_HALF_PI)
  let maxAngleY = min(deg_to_rad(abs(max_angles.z)), SAFE_HALF_PI)
  let minAngleY = min(deg_to_rad(abs(max_angles.w)), SAFE_HALF_PI)
  let dirAngles : float2 = dir_to_angles(dir)
  return float2(
    clamp_s_ang(angles.x, dirAngles.x - minAngleX, dirAngles.x + maxAngleX),
    clamp_s_ang(angles.y, dirAngles.y - minAngleY, dirAngles.y + maxAngleY)
  )

[generic]
def clamp_angles_by_dir(angles, max_angles : float2; dir : float3) : float2
  let x = max_angles.x
  let y = max_angles.y
  return clamp_angles_by_dir(angles, float4(x, x, y, y), dir)

[generic]
def make_rot_cw_zyx(angles : float3) : float3x4
  var tmX, tmY, tmZ : float3x4
  rotxTM(angles.x, tmX)
  rotyTM(angles.y, tmY)
  rotzTM(angles.z, tmZ)
  return tmZ * tmY * tmX

[generic]
def norm_ang(a : float) : float// [0..TWOPI]
  let na = a < 0. ? a + TWOPI : a
  return na - TWOPI * floor(na / TWOPI)

[generic]
def norm_ang(a : float2) : float2
  return float2(norm_ang(a.x), norm_ang(a.y))

[generic]
def dist_ang(a, b) // [0..TWOPI]
  return norm_ang(norm_ang(b) - norm_ang(a))

[generic]
def clamp_ang(a, from, to : float) : float// [0..TWOPI]
  let na = norm_ang(a)
  let d = dist_ang(from, to)
  if d <= 1e-3 || dist_ang(from, na) <= d
    return na
  return dist_ang(na, from) < dist_ang(to, na) ? norm_ang(from) : norm_ang(to)

// [0..TWOPI]
[generic]
def clamp_ang(a : float; limit : float2) : float
  return clamp_ang(a, limit.x, limit.y)

//will also multiply their lengths
[generic]
def sum_rotation(a, b : float2) : float2
  return float2(a.x * b.x - a.y * b.y,
                a.y * b.x + a.x * b.y)

[generic]
def rotate(vec : float2; angle : float) : float2
  var sc : float2
  sincos(angle, sc.y, sc.x)
  return sum_rotation(vec, sc)


[generic]
def renorm_ang(ang : float;
               pivot_ang : float) : float
  let delta = pivot_ang - ang
  return abs(delta) > PI ? ang + (delta > 0.0 ? 1.0 : -1.0) * TWOPI : ang


[generic]
def renorm_ang(ang : float2;
               pivot_ang : float2) : float2
  return float2(renorm_ang(ang.x, pivot_ang.x), renorm_ang(ang.y, pivot_ang.y))

[generic]
def angle(vec1 : float3;
          vec2 : float3) : float
  return atan2(length(cross(vec1, vec2)), dot(vec1, vec2))

[generic]
def angle(vec1 : float2;
          vec2 : float2) : float
  let vec1_3d = float3(vec1, 0.)
  let vec2_3d = float3(vec2, 0.)
  return angle(vec1_3d, vec2_3d)