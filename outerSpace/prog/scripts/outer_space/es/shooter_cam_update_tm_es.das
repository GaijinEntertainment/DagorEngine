require ecs
require DagorMath
require math
require math.base
require level
require Dacoll

[es(tag=render, after=shooter_cam_act_es, before=(after_camera_sync, fx_magnification_es))]
def shooter_cam_update_tm_es(info : UpdateStageInfoAct;
                             var fov : float&;
                             var transform : float3x4;
                             var shooter_cam__lastPos : float3&;
                             var shooter_cam__lastAnimTmOffset : float3&;
                             var camera__accuratePos : DPoint3&;
                             var isHeroCockpitCam : bool&;
                             var camera__totalMagnification : float&;
                             var shooter_cam__wasAlternative : bool&;
                             var shooter_cam__isLastPosValid : bool&;
                             var shooter_cam__tauInc : float&;
                             var shooter_cam__velMain : float3&;
                             var shooter_cam__velAnim : float3&;
                             var shooter_cam__lastOffsDistMult : float&;
                             fovSettings : float;
                             camera__target : EntityId;
                             camera__offset : float3;
                             camera__pivotPos : float3;
                             camera__velTau : float;
                             camera__velFactor : float;
                             camera__tau : float;
                             camera__tauPos : float;
                             camera__tmToAnimRatio : float;
                             camera__animTau : float;
                             shooter_cam__tauOnChange : float;
                             shooter_cam__rayMatId : int;
                             shooter_cam__alternative_settings = false;
                             shooter_cam__moveInSpd = 2.f;
                             shooter_cam__moveOutSpd = 2.f;
                             shooter_cam__offset = float3(0.f, 0.f, 0.f);
                             shooter_cam__additionalOffset = float3(0.f, 0.f, 0.f);
                             shooter_cam__tauMult = 1.f;
                             shooter_cam__velTauMult = 1.f;
                             shooter_cam__magnificationMult = 1.f;
                             shooter_cam__offset_multiplier = 1.f)
  if is_level_loading()
    return

  let offset = camera__offset + shooter_cam__offset + shooter_cam__additionalOffset
  let pivotPos = camera__pivotPos

  let velTau = camera__velTau * shooter_cam__velTauMult
  let velFactor = camera__velFactor
  let tau = camera__tau * shooter_cam__tauMult
  let tauPos = camera__tauPos
  let tmToAnimRatio = camera__tmToAnimRatio
  let animTau = camera__animTau

  if shooter_cam__wasAlternative != shooter_cam__alternative_settings
    shooter_cam__tauInc = max(tauPos, max(shooter_cam__tauInc, shooter_cam__tauOnChange))
  shooter_cam__wasAlternative = shooter_cam__alternative_settings

  var cameraTransform = transform
  var magnification = 1.f

  query(camera__target) <| $ [es] (transform : float3x4;
                                   camera__look_at : DPoint3;
                                   spaceship__rotateView_rad : float2;
                                   spaceship__rotateView_base_quat : float4;
                                   camera__magnification = 1.f;
                                   camera__tauMult = 1.f)
    assert(camera__magnification > 0.0f)

    var q : quat
    euler_to_quat(spaceship__rotateView_rad.x, spaceship__rotateView_rad.y, 0.f, q)

    var rotTm : float3x4
    make_tm(quat(quat_mul(spaceship__rotateView_base_quat, float4(q))), rotTm)
    let wishX = -rotTm[2]
    let wishY = rotTm[1]

    cameraTransform[0] = quat_get_forward(normalize(approach(dir_to_quat(cameraTransform[0]), dir_to_quat(wishX), info.dt, shooter_cam__tauInc)))
    cameraTransform[1] = quat_get_forward(normalize(approach(dir_to_quat(cameraTransform[1]), dir_to_quat(wishY), info.dt, shooter_cam__tauInc)))
    cameraTransform[2] = normalize(cross(cameraTransform[0], cameraTransform[1]))
    cameraTransform[1] = normalize(cross(cameraTransform[2], cameraTransform[0]))

    magnification = camera__magnification * shooter_cam__magnificationMult
    fov = rad_to_deg(atan(tan(deg_to_rad(fovSettings * 0.5f)) * safeinv(magnification))) * 2.f

    camera__totalMagnification = magnification

    let lookAtPos = length_sq(camera__look_at) != 0. ? camera__look_at : DPoint3(transform[3])
    var velMain = DPoint3(shooter_cam__velMain)
    var wishPos = lookAtPos
    if shooter_cam__isLastPosValid
      wishPos = approach_vel(DPoint3(shooter_cam__lastPos), lookAtPos, info.dt, (tau + shooter_cam__tauInc) * camera__tauMult, velMain, velTau, velFactor)

    shooter_cam__lastPos = float3(wishPos)
    shooter_cam__velMain = float3(velMain)

    var velAnim = DPoint3(shooter_cam__velAnim)
    let animTmOffset = lookAtPos - DPoint3(transform[3])
    var wishPosAnim = animTmOffset
    if shooter_cam__isLastPosValid
      wishPosAnim = approach_vel(DPoint3(shooter_cam__lastAnimTmOffset), animTmOffset, info.dt, (animTau + shooter_cam__tauInc) * camera__tauMult, velAnim, velTau, velFactor)

    shooter_cam__velAnim = float3(velAnim)
    shooter_cam__lastAnimTmOffset = float3(wishPosAnim)

    shooter_cam__isLastPosValid = true

    wishPos = wishPos * (1.f - tmToAnimRatio) + (wishPosAnim + DPoint3(transform[3])) * tmToAnimRatio

    var cameraOffs = rotate(cameraTransform, offset * shooter_cam__offset_multiplier)
    let cameraOffslen = length(cameraOffs)

    isHeroCockpitCam = length_sq(offset) == 0.f // exactly zero, we switch to a cockpit cam in that case

    var pivotOffs = rotate(cameraTransform, pivotPos)
    let sphereRad = 0.1f
    if length_sq(pivotOffs) > 1e-4f && cameraOffslen > 0.01f
      // do additional check to original pivot from this one
      var queryOut = ShapeQueryOutput()
      if sphere_cast(float3(wishPos), float3(wishPos + DPoint3(pivotOffs)), sphereRad, queryOut, shooter_cam__rayMatId)
        pivotOffs = rotate(cameraTransform, (pivotPos * queryOut.t))

    var queryOut = ShapeQueryOutput()
    var wishDistMult = 1.f
    if cameraOffslen > 1e-2f && sphere_cast(float3(wishPos + DPoint3(pivotOffs)), float3(wishPos + DPoint3(cameraOffs)), sphereRad, queryOut, shooter_cam__rayMatId)
      //This will result in camera position approaching the same location (save for sphereRad) as the collision point returned by sphere_cast
      let wishOffs = DPoint3(queryOut.res) - wishPos - DPoint3(pivotOffs)
      wishDistMult = safediv(max(length(wishOffs) - sphereRad, 0.f), length(cameraOffs - pivotOffs))

    var multSpd = shooter_cam__moveInSpd
    if wishDistMult > shooter_cam__lastOffsDistMult
      multSpd = shooter_cam__moveOutSpd
    shooter_cam__lastOffsDistMult = move_to_scalar(shooter_cam__lastOffsDistMult, wishDistMult, info.dt, multSpd)
    cameraOffs = lerp(pivotOffs, cameraOffs, float3(shooter_cam__lastOffsDistMult))
    camera__accuratePos = approach(camera__accuratePos, wishPos + DPoint3(cameraOffs), info.dt, shooter_cam__tauInc)
    query(camera__target) <| $ [es] (isAlive = true;
                                     isDowned = false;
                                     camera__fallenGroundDistance = 0.3f)
      if (!isAlive || isDowned) && camera__fallenGroundDistance > 0.f
        let tracePoint = float2(float(camera__accuratePos.x), float(camera__accuratePos.z))
        let lMeshHeight = traceht_lmesh(tracePoint)
        let minHeight = camera__fallenGroundDistance + lMeshHeight
        if camera__accuratePos.y < double(minHeight)
          camera__accuratePos.y = double(minHeight)

    cameraTransform[3].x = float(camera__accuratePos.x)
    cameraTransform[3].y = float(camera__accuratePos.y)
    cameraTransform[3].z = float(camera__accuratePos.z)

  transform = cameraTransform
  shooter_cam__tauInc = move_to_scalar(shooter_cam__tauInc, 0.f, info.dt, 0.4f)
