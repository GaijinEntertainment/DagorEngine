module motorbike_common shared
require ecs
require math.base
require Dacoll
require DagorMath


// game code on the server sends it to stand a fallen bike up on the ground under it
[event(unicast)]
struct CmdMotorbikeGetUp {}


[cpp_event(broadcast)]
struct UpdatePhysEvent
  curTime : float
  dt : float
  curTimeD : double

// the kind reads these bytes as JoltVehicleControls of phys_kind_jolt_vehicle, so the layout must match it
struct MotorbikeControls
  forward : int8
  right : int8
  brake : uint8
  handBrake : uint8


// absolute sine of the lean from up, the reverse of the gravity: 0 upright or upside down, X forward
def motorbike_abs_sin_lean(tm : float3x4; up : float3) : float
  return abs(dot(cross(up, tm[1]), tm[0]))


// the heading of X forward on the plane across up, normalized; zero for a bike pointing along up
def motorbike_flat_forward(tm : float3x4; up : float3) : float3
  let flat = tm[0] - up * dot(tm[0], up)
  return length_sq(flat) < 0.01 ? float3(0.0) : normalize(flat)


// Stands the bike upright against up, heading as tm, on the ground under its wheels at model X wheel_x.x (rear) and wheel_x.y
// (front), traced along the gravity; the model origin goes 5 cm above that ground line. false keeps tm: no ground under a wheel,
// tm along up or wheel_x not rear first.
def motorbike_place_on_ground(var tm : float3x4&; wheel_x : float2; up : float3) : bool
  let fwd = motorbike_flat_forward(tm, up)
  if length_sq(fwd) == 0.0 || wheel_x.y <= wheel_x.x
    return false
  var rear = tm[3] + fwd * wheel_x.x + up
  var front = tm[3] + fwd * wheel_x.y + up
  var tRear = 100.0
  var tFront = 100.0
  var norm : float3
  if !traceray_normalized(rear, -up, tRear, norm, ETF_DEFAULT) || !traceray_normalized(front, -up, tFront, norm, ETF_DEFAULT)
    return false
  rear -= up * tRear
  front -= up * tFront
  tm[0] = normalize(front - rear)
  tm[2] = normalize(cross(tm[0], up))
  tm[1] = cross(tm[2], tm[0])
  tm[3] = rear + (front - rear) * (-wheel_x.x / (wheel_x.y - wheel_x.x)) + tm[1] * 0.05
  return true
