module base shared
require math
require DagorMath

let
  DEG_TO_RAD = PI / 180.
  RAD_TO_DEG = 180. / PI
  TWOPI = 2. * PI
  HALF_PI = PI * 0.5
  SAFE_HALF_PI = HALF_PI - 1e-3f


let
  IDENT_TM = identity3x4()
  VERY_SMALL_NUMBER = 4e-19

let [[ignore_heap_usage]] IDENT_BBOX = BBox3(float3(), 1f) // ignore heap usage because it is used c++ heap

[generic]
def deg_to_rad(val)
  return val * DEG_TO_RAD

[generic]
def rad_to_deg(val)
  return val * RAD_TO_DEG

[generic]
def square(x)
  return x * x


[generic, alias_cmres]
def approach(from, to : auto(algebraic); dt : float; viscosity : float)
  return viscosity < 1e-9 ? to : from + (1.0 - exp(-dt / viscosity)) * (to - from)

[generic]
def approach_move(from, to : float3; dt : float; tau : float; min_vel : float) : float3
  if length(to - from) < min_vel * dt
    return to
  let newVal = approach(from, to, dt, tau)
  if length(newVal - from) < min_vel * dt
    return move_to(from, to, dt, min_vel)
  return approach(from, to, dt, tau)

// move_to version for float
[generic]
def move_to_scalar(from, to, dt, vel : float)
  let d = vel * dt
  if abs(from - to) < d
    return to

  if to < from
    return from - d
  return from + d

[generic]
def move_to(from, to, dt, vel : float)
  let d = vel * dt
  if abs(from - to) < d
    return to

  return to < from ? from - d : from + d

// move_to version for vector types
[generic]
def move_to(from, to : auto(algebraic); dt, vel : float)
  let d = vel * dt
  let dir = to - from
  let dlen = length(dir)
  if dlen <= d
    return to
  return from + (dir / dlen) * d

[generic]
def invsqrt(x)
  return rcp(sqrt(x))

[generic]
def safeinvsqrt(x)
  return x > 0.0 ? invsqrt(x) : 1.0

[generic]
def safeinv(val : float3) : float3
  return float3(safeinv(val.x), safeinv(val.y), safeinv(val.z))

[generic]
def normalize_default(vec, default_vec)
  let len = length(vec)
  return len > 1e-9 ? vec / len : default_vec

// das implementation of C++ function from dag_mathBase.h
[generic]
def is_relative_equal_float(a, b : float; max_diff : float = 1e-5; max_rel_diff : float = FLT_EPSILON)
  let diff = abs(a - b)
  if diff <= max_diff
    return true
  let absA = abs(a)
  let absB = abs(b)
  return diff <= (absA < absB ? absB : absA) * max_rel_diff

// das implementation of C++ function from dag_mathBase.h
// if you need are_approximately_equal, use this one instead
[generic]
def is_equal_float(a, b : float; eps : float = 1e-5f)
  return is_relative_equal_float(a, b, eps, FLT_EPSILON)

[generic]
def is_equal_float(a, b : float2; eps : float = 1e-5f)
  return is_equal_float(a.x, b.x, eps) && is_equal_float(a.y, b.y, eps)

[generic]
def is_equal_float(a, b : float3; eps : float = 1e-5f)
  return is_equal_float(a.x, b.x, eps) && is_equal_float(a.y, b.y, eps) && is_equal_float(a.z, b.z, eps)

[generic]
def is_equal_float(a, b : float4; eps : float = 1e-5f)
  return is_equal_float(a.x, b.x, eps) && is_equal_float(a.y, b.y, eps) && is_equal_float(a.z, b.z, eps) && is_equal_float(a.w, b.w, eps)

// returns signed magnitude of the vector that would result from a regular 3D cross product of the input vectors
[generic]
def cross2d(a, b : float2) : float
  return a.y * b.x - a.x * b.y

[generic]
def is_nan(a : float4)
  return is_nan(a.x) || is_nan(a.y) || is_nan(a.z) || is_nan(a.w)

[generic]
def is_nan(a : float3)
  return is_nan(a.x) || is_nan(a.y) || is_nan(a.z)

[generic]
def is_nan(a : float2)
  return is_nan(a.x) || is_nan(a.y)


[generic]
def scale_tm(scale : float3) : float3x4
  var scaleTm : float3x4
  scaleTm[0] = float3(scale.x, 0.0, 0.0)
  scaleTm[1] = float3(0.0, scale.y, 0.0)
  scaleTm[2] = float3(0.0, 0.0, scale.z)
  return <- scaleTm

[generic]
def scale_tm(scale : float) : float3x4
  var scaleTm : float3x4
  scaleTm[0] = float3(scale, 0.0, 0.0)
  scaleTm[1] = float3(0.0, scale, 0.0)
  scaleTm[2] = float3(0.0, 0.0, scale)
  return <- scaleTm

[generic]
def pos_tm(pos : float3) : float3x4
  var res = IDENT_TM
  res[3] = pos
  return res

[generic]
def is_inside_tm(pos : float3; tm : float3x4) : bool
  let p = pos - tm[3]
  return (abs(dot(p, tm[0])) < dot(tm[0], tm[0]) * 0.5 &&
          abs(dot(p, tm[1])) < dot(tm[1], tm[1]) * 0.5 &&
          abs(dot(p, tm[2])) < dot(tm[2], tm[2]) * 0.5)


[generic]
def tm_volume(tm : float3x4) : float
  return sqrt(length_sq(tm[0]) * length_sq(tm[1]) * length_sq(tm[2]))


[generic]
def cvt(val : float; from : float2; to : float2) : float
  return cvt(val, from.x, from.y, to.x, to.y)


[generic]
def frac(x)
  return x - floor(x)

[generic]
def get_normal(dir : float3)
  let altDir = abs(dot(dir, float3(1, 0, 0))) > 0.5 ? float3(0, 0, 1) : float3(1, 0, 0)
  return cross(dir, altDir)

[generic]
def x0z(f)
  return float3(f.x, 0., f.z)

[generic]
def xVz(f; v)
  return float3(f.x, v, f.z)

[generic]
def fsel(a, b, c : float)
  return a >= 0. ? b : c

[generic]
def max(a, b, c)
  return max(max(a, b), c)

[generic]
def min(a, b, c)
  return min(min(a, b), c)

[generic]
def perpendicular(var norm : float3)
  norm = normalize(norm)
  let threshold = 0.5
  if (abs(norm.z) < threshold)
    return cross(norm, float3(-norm.y, norm.x, norm.z))
  elif (abs(norm.y) < threshold)
    return cross(norm, float3(-norm.z, norm.y, norm.x))
  else
    return cross(norm, float3(norm.x, -norm.z, norm.y))