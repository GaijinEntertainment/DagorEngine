require ecs
require DagorSystem
require math.base
require DagorMath
require MotionMatching
require AnimV20
require danetlibs.renderer.includes.pufd_events


[es(on_event=AnimationDataBaseAssigned)]
def mm_trajectory_init(evt : Event;
                       eid : EntityId;
                       motion_matching__controller : MotionMatchingController;
                       var mm_trajectory__historyPositions : Point3List;
                       var mm_trajectory__historyRotations : Point4List;
                       var mm_trajectory__historyLength : int&;
                       var mm_trajectory__lastHistoryIdx : int&;
                       var mm_trajectory__lastHistoryTime : float&;
                       var mm_trajectory__historyDt : float&;
                       var mm_trajectory__featurePositions : Point3List;
                       var mm_trajectory__featureDirections : Point3List)
  assume dataBase = motion_matching__controller.dataBase
  assert(dataBase != null)

  mm_trajectory__historyPositions |> clear()
  mm_trajectory__historyRotations |> clear()
  mm_trajectory__lastHistoryIdx = -1
  mm_trajectory__lastHistoryTime = 0f
  var historyFrom = 0f
  for t in dataBase.predictionTimes
    if t < historyFrom
      historyFrom = t
  if mm_trajectory__historyLength > 0 && historyFrom == 0f
    logerr("Motion Matching: `mm_trajectory__historyLength` should be 0 when no history is required. {eid} <{getEntityTemplateName(eid)}>")
    mm_trajectory__historyLength = 0
  if historyFrom < 0f
    mm_trajectory__historyLength = max(mm_trajectory__historyLength, 2)
    mm_trajectory__historyDt = -historyFrom / (float(mm_trajectory__historyLength) - 1f)
  let trajectoryFeaturePoints = length(dataBase.predictionTimes)
  mm_trajectory__featurePositions |> resize(trajectoryFeaturePoints)
  mm_trajectory__featureDirections |> resize(trajectoryFeaturePoints)
  mm_trajectory__historyPositions |> reserve(mm_trajectory__historyLength)
  mm_trajectory__historyRotations |> reserve(mm_trajectory__historyLength)

[es(on_appear)]
def mm_trajectory_init_orientation(evt : Event;
                                   animchar : AnimcharBaseComponent;
                                   var mm_trajectory__currPosition : float3&;
                                   var mm_trajectory__currRotation : float4&)
  var tm : float3x4
  animchar |> animchar_get_tm(tm)
  mm_trajectory__currPosition = tm[3]
  mm_trajectory__currRotation = float4(DagorMath::quat(tm))

[es(after=animchar_es, before=mm_trajectory_track_history, REQUIRE_NOT=mm_trajectory__customUpdate)]
def mm_trajectory_update(info : ParallelUpdateFrameDelayed;
                         animchar : AnimcharBaseComponent;
                         var mm_trajectory__prevPosition : float3&;
                         var mm_trajectory__prevRotation : float4&;
                         var mm_trajectory__currPosition : float3&;
                         var mm_trajectory__currRotation : float4&;
                         var mm_trajectory__linearVelocity : float3&;
                         var mm_trajectory__angularVelocity : float3&;
                         mm_trajectory__linearVelocityViscosity : float;
                         mm_trajectory__angularVelocityViscosity : float)
  mm_trajectory__prevPosition = mm_trajectory__currPosition
  mm_trajectory__prevRotation = mm_trajectory__currRotation
  var tm : float3x4
  animchar |> animchar_get_tm(tm)
  mm_trajectory__currPosition = tm[3]
  mm_trajectory__currRotation = float4(DagorMath::quat(tm))
  let currVelocity = (mm_trajectory__currPosition - mm_trajectory__prevPosition) / (info.dt + 0.00001f)
  mm_trajectory__linearVelocity = approach(mm_trajectory__linearVelocity, currVelocity, info.dt, mm_trajectory__linearVelocityViscosity)
  let q = quat(mm_trajectory__currRotation) * inverse(quat(mm_trajectory__prevRotation))
  var angles : float3
  quat_to_euler(q, angles.x, angles.y, angles.z)
  let currAngVelocity = float3(0f, safediv(angles.x, info.dt), 0f)
  mm_trajectory__angularVelocity = approach(mm_trajectory__angularVelocity, currAngVelocity, info.dt, mm_trajectory__angularVelocityViscosity)

[es(before=mm_trajectory_prediction)]
def mm_trajectory_track_history(info : ParallelUpdateFrameDelayed;
                                var mm_trajectory__historyPositions : Point3List;
                                var mm_trajectory__historyRotations : Point4List;
                                mm_trajectory__historyLength : int;
                                var mm_trajectory__lastHistoryIdx : int&;
                                var mm_trajectory__lastHistoryTime : float&;
                                mm_trajectory__historyDt : float;
                                mm_trajectory__prevPosition : float3;
                                mm_trajectory__prevRotation : float4;
                                mm_trajectory__currPosition : float3;
                                mm_trajectory__currRotation : float4)
  if mm_trajectory__historyLength <= 0 || mm_trajectory__historyDt <= 0f
    return
  mm_trajectory__lastHistoryTime += info.dt
  if mm_trajectory__lastHistoryTime >= mm_trajectory__historyDt
    var newPoints = int(mm_trajectory__lastHistoryTime / mm_trajectory__historyDt)
    mm_trajectory__lastHistoryTime -= float(newPoints) * mm_trajectory__historyDt
    newPoints = min(newPoints, mm_trajectory__historyLength)
    let historyLength = length(mm_trajectory__historyPositions)
    if historyLength < mm_trajectory__historyLength
      let newSize = min(historyLength + newPoints, mm_trajectory__historyLength)
      mm_trajectory__historyPositions |> resize(newSize)
      mm_trajectory__historyRotations |> resize(newSize)
    var interpK = 1f - mm_trajectory__lastHistoryTime / info.dt
    mm_trajectory__lastHistoryIdx = (mm_trajectory__lastHistoryIdx + newPoints) % mm_trajectory__historyLength
    //fill from most recent to older
    for i in range(newPoints)
      var historyIdx = mm_trajectory__lastHistoryIdx - i
      if historyIdx < 0
        historyIdx += mm_trajectory__historyLength
      assert(interpK >= 0f && interpK <= 1f)
      let historyPos = lerp(mm_trajectory__prevPosition, mm_trajectory__currPosition, float3(interpK))
      let historyRot = slerp(quat(mm_trajectory__prevRotation), quat(mm_trajectory__currRotation), interpK)
      mm_trajectory__historyPositions[historyIdx] = historyPos
      mm_trajectory__historyRotations[historyIdx] = float4(historyRot)
      interpK -= mm_trajectory__historyDt / info.dt

def sample_history_trajectory(sample_time : float;
                              historyPositions : Point3List;
                              historyRotations : Point4List;
                              historyLength, lastHistoryIdx : int;
                              lastHistoryTime, historyDt : float;
                              currPosition : float3;
                              currRotation : float4;
                              var out_world_pos : float3&;
                              var out_world_rot : quat&)
  let historyOffset = -(sample_time + lastHistoryTime) / historyDt
  assert(int(ceil(historyOffset)) < historyLength)
  if int(ceil(historyOffset)) >= length(historyPositions)
    //when history is not full
    out_world_pos = length(historyPositions) > 0 ? historyPositions[0] : currPosition
    out_world_rot = length(historyRotations) > 0 ? quat(historyRotations[0]) : quat(currRotation)
    return
  if historyOffset < 0f
    let interpK = 1f - sample_time / lastHistoryTime
    out_world_pos = lerp(historyPositions[lastHistoryIdx], currPosition, float3(interpK))
    out_world_rot = slerp(quat(historyRotations[lastHistoryIdx]), quat(currRotation), interpK)
  else
    var idx1 = lastHistoryIdx - int(ceil(historyOffset))
    var idx2 = lastHistoryIdx - int(floor(historyOffset))
    if idx1 < 0
      idx1 += historyLength
    if idx2 < 0
      idx2 += historyLength
    let interpK = 1f - fract(historyOffset)
    assert(idx1 >= 0 && idx1 < historyLength && idx2 >= 0 && idx2 < historyLength)
    out_world_pos = lerp(historyPositions[idx1], historyPositions[idx2], float3(interpK))
    out_world_rot = slerp(quat(historyRotations[idx1]), quat(historyRotations[idx2]), interpK)

def predict_future_trajectory(predict_time : float;
                              currPosition : float3;
                              currRotation : float4;
                              linearVelocity, angularVelocity : float3;
                              wishLinearVelocity, wishAngularVelocity : float3;
                              wishRotation : float4 const?;
                              linearVelocityViscosity, angularVelocityViscosity : float;
                              var out_world_pos : float3&;
                              var out_world_rot : quat&)
  if wishRotation != null
    var worldRotation = currRotation
    var angVelocity = angularVelocity
    simple_spring_damper_exact_q(worldRotation, angVelocity, *wishRotation, angularVelocityViscosity, predict_time)
    out_world_rot = quat(worldRotation)
  else
    // Angular velocity: w(t) = approach(w0, w_wish, t, w_visc) = w_wish + (w0 - w_wish) * exp(-t / w_visc)
    // Rotation: r(t) = r0 + integral w(t) dt = r0 + w_wish * t + (w0 - w_wish) * w_visc * (1 - exp(-t / w_visc))
    var rotAngle = wishAngularVelocity.y * predict_time
    if angularVelocityViscosity > 1e-5
      rotAngle += (angularVelocity.y - wishAngularVelocity.y) * angularVelocityViscosity * (1f - exp(-predict_time / angularVelocityViscosity))
    let deltaRot = quat(float3(0, 1, 0), rotAngle)
    out_world_rot = deltaRot * quat(currRotation)

  // Velocity: v(t) = approach(v0, v_wish, t, v_visc) = v_wish + (v0 - v_wish) * exp(-t / v_visc)
  // Position: x(t) = x0 + integral v(t) dt = x0 + v_wish * t + (v0 - v_wish) * v_visc * (1 - exp(-t / v_visc))
  var deltaPos = wishLinearVelocity * predict_time
  if linearVelocityViscosity > 1e-5
    deltaPos += (linearVelocity - wishLinearVelocity) * linearVelocityViscosity * (1f - exp(-predict_time / linearVelocityViscosity))
  out_world_pos = currPosition + deltaPos

[es(before=mm_update_goal_features_es)]
def mm_trajectory_prediction(info : ParallelUpdateFrameDelayed;
                             motion_matching__controller : MotionMatchingController;
                             mm_trajectory__historyPositions : Point3List;
                             mm_trajectory__historyRotations : Point4List;
                             mm_trajectory__historyLength : int;
                             mm_trajectory__lastHistoryIdx : int;
                             mm_trajectory__lastHistoryTime : float;
                             mm_trajectory__historyDt : float;
                             mm_trajectory__currPosition : float3;
                             mm_trajectory__currRotation : float4;
                             mm_trajectory__linearVelocity : float3;
                             mm_trajectory__angularVelocity : float3;
                             mm_trajectory__linearVelocityViscosity : float;
                             mm_trajectory__angularVelocityViscosity : float;
                             var mm_trajectory__featurePositions : Point3List;
                             var mm_trajectory__featureDirections : Point3List;
                             mm_trajectory__wishLinearVelocity : float3 const?;
                             mm_trajectory__wishAngularVelocity : float3 const?;
                             mm_trajectory__wishRotation : float4 const?)
  assume dataBase = motion_matching__controller.dataBase
  if dataBase == null
    return
  let invRotation = inverse(quat(motion_matching__controller.rootRotation))

  for i, t in range(length(dataBase.predictionTimes)), dataBase.predictionTimes
    var worldPos : float3
    var worldRot : quat
    if t < 0f
      sample_history_trajectory(t, mm_trajectory__historyPositions, mm_trajectory__historyRotations,
        mm_trajectory__historyLength, mm_trajectory__lastHistoryIdx, mm_trajectory__lastHistoryTime,
        mm_trajectory__historyDt, mm_trajectory__currPosition, mm_trajectory__currRotation, worldPos, worldRot)
    else
      predict_future_trajectory(t, mm_trajectory__currPosition, mm_trajectory__currRotation,
        mm_trajectory__linearVelocity, mm_trajectory__angularVelocity,
        mm_trajectory__wishLinearVelocity ?? mm_trajectory__linearVelocity,
        mm_trajectory__wishAngularVelocity ?? mm_trajectory__angularVelocity, mm_trajectory__wishRotation,
        mm_trajectory__linearVelocityViscosity, mm_trajectory__angularVelocityViscosity,
        worldPos, worldRot)
    let localPos = worldPos - motion_matching__controller.rootPosition
    mm_trajectory__featurePositions[i] = invRotation * localPos
    mm_trajectory__featureDirections[i] = invRotation * worldRot * MotionMatching::FORWARD_DIRECTION.xyz
