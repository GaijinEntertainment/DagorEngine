require ecs
require DagorSystem
require math.base
require DagorMath

[es(tag=tools)]
def tool_input(info : UpdateStageInfoAct;
               motion_matching__walkSpeed : float;
               motion_matching__runSpeed : float;
               var mm_trajectory__linearVelocity : float3&;
               var mm_trajectory__angularVelocity : float3&;
               mm_trajectory__linearVelocityViscosity : float;
               mm_trajectory__angularVelocityViscosity : float)
  query() <| $ [es] (input__verticalAxis : float;
                     input__horizontalAxis : float;
                     input__mouseDelta : int2;
                     input__mouseLeftButtonPressed : bool;
                     input__keyAccelerationPressed : bool)
    var vel = -float3(input__horizontalAxis, 0.f, input__verticalAxis)
    let len = length(vel)
    if len > 1f
      vel /= len
    vel *= input__keyAccelerationPressed ? motion_matching__runSpeed : motion_matching__walkSpeed;
    mm_trajectory__linearVelocity = approach(mm_trajectory__linearVelocity, vel, info.dt, mm_trajectory__linearVelocityViscosity)
    let PIXEL_TO_DEG = 40.f;
    var angVel = float3(0)
    if input__mouseLeftButtonPressed
      angVel = float3(0.f, deg_to_rad(float(input__mouseDelta.x) * PIXEL_TO_DEG), 0.f)
    mm_trajectory__angularVelocity = approach(mm_trajectory__angularVelocity, angVel, info.dt, mm_trajectory__angularVelocityViscosity)