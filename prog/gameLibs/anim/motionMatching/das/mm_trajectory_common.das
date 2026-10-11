module mm_trajectory_common shared

require ecs
require math.base
require DagorMath
require MotionMatching


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
