module blood_decals_common shared
require ecs
require ProjectiveDecals
require Dacoll
require DagorShaders
require DagorMath

let DEPTH_TEX_SIZE = 4
let DEPTH_ATLAS_SIZE = 256
let MAX_SPLASH_DISTANCE = 10.f
let MAX_DECAL_SIZE = 10.0f
let TRACE_RESULT_BUFFER_SIZE = DEPTH_TEX_SIZE * DEPTH_TEX_SIZE

let PARABOLIC_V0 = 5.0f
let PARABOLIC_GRAVITY = 10.0f
let PARABOLIC_FAKE_AERODYNAMIC_DRAG_OFFSET = 0.5f

let HUMAN_SIZE = 2.f
let DEPTH_TRACE_SUCCESS_THRESHOLD = 8

let GROUND_TRACE_THRESHOLD = 12
let GROUND_HEIGHT_THRESHOLD = 0.5f
let GROUND_NORMAL_THRESHOLD = sin(PI / 4f) // 45 degrees
let GROUND_SPLASH_OFFSET = 0.5f

let DECAL_ATLAS_ROWS = 2
let DECAL_ATLAS_COLUMNS = 2

def trace_depth_tex(var blood_decals_traces_result : FloatList; traces_offset : int; eye, dir_normalized : float3; decal_size : float)
  let texelSize = decal_size / float(DEPTH_TEX_SIZE)
  // same as this, but cheaper: Point3 rightStep = normalize(cross(Point3(0, 1, 0), dir_normalized)) * texelSize;
  var rightStep = normalize(float3(dir_normalized.z, 0.f, -dir_normalized.x)) * texelSize
  var downStep = normalize(cross(rightStep, dir_normalized)) * texelSize
  if (length_sq(rightStep) < 0.01 * texelSize * texelSize) // edge-case for vertical directions

    rightStep = float3(1, 0, 0)
    downStep = float3(0, 0, 1)

  let stepsFromMiddleToCornerTexelMiddle = float(DEPTH_TEX_SIZE) * 0.5f - 0.5f
  let topLeftRayTarget = eye + dir_normalized * MAX_SPLASH_DISTANCE - (rightStep + downStep) * stepsFromMiddleToCornerTexelMiddle

  var numHits = 0
  var norm = float3()

  for j in range(DEPTH_TEX_SIZE)
    for i in range(DEPTH_TEX_SIZE)
      let rayTarget = topLeftRayTarget + rightStep * float(i) + downStep * float(j)
      let dir = normalize(rayTarget - eye)
      var t = MAX_SPLASH_DISTANCE
      if (traceray_normalized(eye, dir, t, norm, ETF_DEFAULT))
        numHits += 1

        blood_decals_traces_result[traces_offset + j * DEPTH_TEX_SIZE + i] = t

  return numHits

def calculate_parabolic_trajectory(prev_look_at : float3; var eye, norm : float3&)

  let launchSpeedSqr = PARABOLIC_V0 * PARABOLIC_V0
  // normalize() is implemented safely, so vertical case here will return a null vector, which is correct in this case:
  let launchDirHorizontal = normalize(float3(-norm.x, 0.f, -norm.z))
  let cosLaunchAngle = dot(-norm, launchDirHorizontal)
  // rounding errors can cause cosLaunchAngle to be > 1, which causes sqrt to be called with negative number, which was observed to cause
  // floating-point invalid operation exception, so safe_sqrt() is used instead of sqrtf().
  let sinLaunchAngle = safe_sqrt(1f - cosLaunchAngle * cosLaunchAngle)
  let sinSqrTheta = sinLaunchAngle * sinLaunchAngle
  let sin2Theta =  2.f * sinLaunchAngle * cosLaunchAngle
  let arcHeight = launchSpeedSqr * sinSqrTheta / (2f * PARABOLIC_GRAVITY)
  let arcHalfDistance = launchSpeedSqr * sin2Theta / PARABOLIC_GRAVITY

  eye = prev_look_at + launchDirHorizontal * arcHalfDistance + float3(0f, arcHeight, 0f)
  let newLookAt = prev_look_at + launchDirHorizontal * arcHalfDistance * (2f - PARABOLIC_FAKE_AERODYNAMIC_DRAG_OFFSET)

  let pointToEye = eye - newLookAt
  norm = normalize(pointToEye)

[event(broadcast)]
struct CmdAddBloodDecalInternal
  tm : float3x4
  rad : float
  tex_id : uint16
  mx_id : uint16
  params : float4


[event(broadcast)]
struct CmdClearBloodDecals {}

def blood_decals_add_decal(eye_pos, look_at : float3)
  query() <| $ [es] (var blood_decals_traces_result : FloatList;
                     var blood_decals_traces_queue : IPoint2List;
                     var blood_decals_decals_count : int&)


    let pointToEye = eye_pos - look_at
    var norm = normalize(pointToEye)
    let fromEyeDist = 0.01f
    // Move away a little bit from the camera, otherwise the camera can be glitch into the projective geometry causing flickering
    var eye = eye_pos - norm * fromEyeDist
    let decalSize = min(MAX_DECAL_SIZE, (MAX_SPLASH_DISTANCE / length(pointToEye)) * HUMAN_SIZE)
    let tracesOffset = length(blood_decals_traces_result)
    resize(blood_decals_traces_result, tracesOffset + TRACE_RESULT_BUFFER_SIZE)

    var numHits = trace_depth_tex(blood_decals_traces_result, tracesOffset, eye, -norm, decalSize)
    if numHits < GROUND_TRACE_THRESHOLD && abs(traceht_lmesh(eye) - eye.y) < GROUND_HEIGHT_THRESHOLD && abs(norm.y) < GROUND_NORMAL_THRESHOLD
      // Prevents decal from being placed parallel to ground, it will look bad (or invisible at all)
      // because most parts of the decal will be discarded by normal of the ground in pixel shader.
      // We can still place such decals on the walls if trace results were successful enough.
      norm = normalize(float3(0, 1, 0) + normalize(float3(pointToEye.x, 0, pointToEye.z)))
      eye = look_at + norm * GROUND_SPLASH_OFFSET
      numHits = trace_depth_tex(blood_decals_traces_result, tracesOffset, eye, -norm, decalSize)
    elif (numHits < DEPTH_TRACE_SUCCESS_THRESHOLD && norm.y < 0f)

      // If we don't hit enough geometry, and look_at is higher than eye, calculate a parabolic trajectory,
      // place decal looking down from the top of the parabolic arc
      calculate_parabolic_trajectory(look_at, eye, norm)
      numHits = trace_depth_tex(blood_decals_traces_result, tracesOffset, eye, -norm, decalSize)

    if (numHits < 0)
      return

    let NUM_DEPTH_TEX_PER_ROW = DEPTH_ATLAS_SIZE / DEPTH_TEX_SIZE
    let atlasPos = int2(blood_decals_decals_count % NUM_DEPTH_TEX_PER_ROW, blood_decals_decals_count / NUM_DEPTH_TEX_PER_ROW) * DEPTH_TEX_SIZE

    blood_decals_traces_queue |> push(atlasPos)

    // same as this, but cheaper: float3 right = normalize(cross(norm, float3(0, 1, 0)));
    let right = normalize(float3(-norm.z, 0f, norm.x))
    var transform : float3x4
    transform[0] = float3(0, 0, 0)
    transform[1] = norm
    transform[2] = right
    transform[3] = eye

    let startTime = get_shader_global_time_phase(0f, 0f)
    let frameNo = blood_decals_decals_count % (DECAL_ATLAS_ROWS * DECAL_ATLAS_COLUMNS)
    let params = float4(float(atlasPos.x), float(atlasPos.y), decalSize * 0.5f, float(frameNo))

    // Variable 'params' is packed to uint2 by the generator shader. Because of this, it does not support enough precision for
    // startTime, so decalSize and startTime is swapped. It is decoded again in the vertex shader stage of the rendering shader.

    // Note: intentionally not immediate since this function can be called from thread (ParallelUpdateFrameDelayed)
    broadcastEvent(CmdAddBloodDecalInternal(tm = transform, rad = startTime, tex_id = uint16(0), mx_id = uint16(0), params = params))

    blood_decals_decals_count++

def add_blood_decal(distance : float; pos, dir : float3)
  blood_decals_add_decal(pos, pos + dir * distance)

