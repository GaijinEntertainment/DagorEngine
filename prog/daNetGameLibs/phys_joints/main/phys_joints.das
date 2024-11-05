require ecs
require DagorConsole
require GamePhys
require DagorDebug3D
require vehicle
require DngPlane
require DngHuman
require PhysObj
require DngPhysObj
require DngShip
require Ship
require math.base
require DagorRandom
require RendInst
require RiDestr

struct PhysBodyData
  transform : float3x4
  cog : float3
  velocity : float3
  omega : float3
  mass : float
  momentOfInertia : float3
  lambdaLimit : float

struct JointResult
  contactPt : float3
  addVel : float3
  addOmega : float3
  deltaLen : float
  lambdaSum : float

[es(before=phys_joint_update, after=after_net_phys_sync)]
def reset_winching(info : UpdateStageInfoAct; var vehicle_net_phys : VehiclePhysActor)
  vehicle_net_phys.phys.currentState.damageState.winching = false

def query_phys_body_data(eid : EntityId; ri_extra__handle : riex_handle_t; b : block<(arg1 : PhysBodyData) : JointResult>)
  if eid == INVALID_ENTITY_ID
    let res = invoke(b, [[PhysBodyData transform=IDENT_TM, cog=float3(0, 0, 0),
                             velocity=float3(0, 0, 0), omega=float3(0, 0, 0),
                             mass=0f, momentOfInertia=float3(0, 0, 0),
                             lambdaLimit=FLT_MAX]])
    if res.lambdaSum > 0f && riex_isRiGenExtraValid(ri_extra__handle)
      let dir = normalize(res.addVel)
      apply_impulse_to_riextra(ri_extra__handle, res.lambdaSum, dir, res.contactPt)
    return

  query(eid) <| $ [es] (var vehicle_net_phys : VehiclePhysActor; joint__lambdaLimit : float = FLT_MAX)
    assume phys = vehicle_net_phys.phys
    assume state = phys.currentState

    var transform : float3x4
    location_toTM(state.location, transform)

    let res = invoke(b, [[PhysBodyData transform=transform, cog=phys.mass.centerOfGravity,
                                       velocity=float3(state.velocity), omega=float3(state.omega),
                                       mass=phys.mass.mass, momentOfInertia=float3(phys.mass.momentOfInertia),
                                       lambdaLimit=joint__lambdaLimit]])

    if length(res.addVel) > 0f
      state.velocity += DPoint3(res.addVel)
      state.omega += DPoint3(res.addOmega)
      state.damageState.winching = true
      ground_control_state_setAxisValue(phys.producedCT, EGroundControlAxis GCA_BRAKE_LEFT, 0.f)
      ground_control_state_setAxisValue(phys.producedCT, EGroundControlAxis GCA_BRAKE_RIGHT, 0.f)
      wakeUp(phys)

    query(eid) <| $ [es] (var joint__lambdaSum : float&)
      joint__lambdaSum = res.lambdaSum

  query(eid) <| $ [es] (var ship_net_phys : ShipActor; joint__lambdaLimit : float = FLT_MAX)
    assume phys = ship_net_phys.phys
    assume state = phys.currentState

    var transform : float3x4
    location_toTM(state.location, transform)

    let res = invoke(b, [[PhysBodyData transform=transform, cog=phys.mass.centerOfGravity,
                                       velocity=float3(state.velocity), omega=float3(state.omega),
                                       mass=phys.mass.mass, momentOfInertia=float3(phys.mass.momentOfInertia),
                                       lambdaLimit=joint__lambdaLimit]])

    if length(res.addVel) > 0f
      state.velocity += DPoint3(res.addVel)
      state.omega += DPoint3(res.addOmega)
      ship_phys_set_anchor(phys, false)
      ship_phys_wake_up(phys)

  query(eid) <| $ [es] (var plane_net_phys : PlaneActor; joint__lambdaLimit : float = FLT_MAX)
    assume phys = plane_net_phys.phys
    assume state = phys.currentState

    var transform : float3x4
    location_toTM(state.location, transform)

    let res = invoke(b, [[PhysBodyData transform=transform, cog=phys.M.centerOfGravity,
                                       velocity=float3(state.velocity), omega=float3(state.omega),
                                       mass=phys.M.mass, momentOfInertia=float3(phys.M.momentOfInertia),
                                       lambdaLimit=joint__lambdaLimit]])

    if length(res.addVel) > 0f
      state.velocity += DPoint3(res.addVel)
      state.omega += DPoint3(res.addOmega)

  query(eid) <| $ [es] (var human_net_phys : HumanActor; joint__lambdaLimit : float = FLT_MAX)
    assume phys = human_net_phys.phys
    assume state = phys.currentState

    var transform : float3x4
    location_toTM(state.location, transform)

    let res = invoke(b, [[PhysBodyData transform=transform, cog=float3(0f, 1f, 0f),
                                       velocity=state.velocity, omega=float3(0),
                                       mass=80f, momentOfInertia=float3(0f),
                                       lambdaLimit=joint__lambdaLimit]])

    if length(res.addVel) > 0f
      state.velocity += res.addVel

  query(eid) <| $ [es] (var phys_obj_net_phys : PhysObjActor; joint__lambdaLimit : float = FLT_MAX; joint__minSpeed : float = 0f)
    assume phys = phys_obj_net_phys.phys
    assume state = phys.currentState

    var transform : float3x4
    location_toTM(state.location, transform)

    let res = invoke(b, [[PhysBodyData transform=transform, cog=phys.centerOfMass,
                                       velocity=state.velocity, omega=state.omega,
                                       mass=phys.mass, momentOfInertia=phys.momentOfInertia,
                                       lambdaLimit=joint__lambdaLimit]])

    if length(res.addVel) > joint__minSpeed
      state.velocity += res.addVel
      state.omega += res.addOmega
      wakeUp(phys)

def vec_safeinv(input : float3)
  return float3(safeinv(input.x), safeinv(input.y), safeinv(input.z))

[es(after=after_net_phys_sync, before=after_collision_sync)]
def phys_joint_update(info : UpdateStageInfoAct;
                      phys_joint__firstBody : EntityId;
                      phys_joint__secondBody : EntityId;
                      phys_joint__firstBodyAnchor : float3;
                      phys_joint__secondBodyAnchor : float3;
                      phys_joint__jointLength : float;
                      phys_joint__extensionMax : float = FLT_EPSILON;
                      phys_joint__erpRatio : float2 = float2(1.1f, 1.5f);
                      phys_joint__erpRange : float2 = float2(0f, 8f);
                      var joint__lambdaSum : float&;
                      ri_extra__handle : riex_handle_t)
  if phys_joint__jointLength < phys_joint__extensionMax
    return
  // query first body
  query_phys_body_data(phys_joint__firstBody, ri_extra__handle) <| $(body1 : PhysBodyData)
    let firstAnchorWorld = body1.transform * phys_joint__firstBodyAnchor
    // query second body
    var addVel1 = float3(0)
    var addOmega1 = float3(0)
    var deltaLen = phys_joint__jointLength
    var lambdaSum = 0f
    query_phys_body_data(phys_joint__secondBody, ri_extra__handle) <| $(body2 : PhysBodyData)
      // evaluate constraint
      var addVel2 = float3(0)
      var addOmega2 = float3(0)
      let secondAnchorWorld = body2.transform * phys_joint__secondBodyAnchor
      let worldDir = secondAnchorWorld - firstAnchorWorld
      let curJointLength = length(worldDir)
      deltaLen -= curJointLength
      if curJointLength > phys_joint__jointLength
        // apply impulses
        let erpFactor = cvt(curJointLength / phys_joint__jointLength, phys_joint__erpRatio.x, phys_joint__erpRatio.y, phys_joint__erpRange.x, phys_joint__erpRange.y)
        let invMass1 = safeinv(body1.mass)
        let invMass2 = safeinv(body2.mass)
        let invMoi1 = vec_safeinv(body1.momentOfInertia)
        let invMoi2 = vec_safeinv(body2.momentOfInertia)
        let invTm1 = inverse(body1.transform)
        let invTm2 = inverse(body2.transform)

        let axis1 = worldDir / curJointLength // from first to second
        let axis2 = -axis1
        let w1 = cross(phys_joint__firstBodyAnchor - body1.cog, rotate(invTm1, axis1))
        let w2 = cross(phys_joint__secondBodyAnchor - body2.cog, rotate(invTm2, axis2))
        let b = invMass1 + invMass2 + dot(w1, w1 * invMoi1) + dot(w2, w2 * invMoi2)
        let lambdaLimit = min(body1.lambdaLimit, body2.lambdaLimit)
        for _ in range(5)
          let vel1 = body1.velocity + addVel1
          let vel2 = body2.velocity + addVel2
          let omega1 = body1.omega + addOmega1
          let omega2 = body2.omega + addOmega2

          let a = dot(axis1, vel1) + dot(w1, omega1) + dot(axis2, vel2) + dot(w2, omega2)
          let l = min(-safediv(a - erpFactor, b), lambdaLimit - lambdaSum)
          lambdaSum += l
          if abs(l) < FLT_EPSILON
            break
          addVel1 += axis1 * l * invMass1
          addOmega1 += w1 * invMoi1 * l
          addVel2 += axis2 * l * invMass2
          addOmega2 += w2 * invMoi2 * l
        joint__lambdaSum = lambdaSum
      return [[JointResult contactPt=secondAnchorWorld, addVel=addVel2, addOmega=addOmega2, deltaLen=deltaLen, lambdaSum=lambdaSum]]
    return [[JointResult contactPt=firstAnchorWorld, addVel=addVel1, addOmega=addOmega1, deltaLen=deltaLen, lambdaSum=lambdaSum]]

def update_winch(dt : float; eid : EntityId; var phys_joint__jointLength : float&)
  query(eid) <| $ [es] (winch__pulls, winch__extends : bool;
                        winch__rad, winch__omega : float;
                        winch__torque, winch__brakeTorque, joint__lambdaSum : float;
                        var joint__lambdaLimit : float&)
    let isOnBrake = winch__pulls == winch__extends
    let winchTorque = isOnBrake ? winch__brakeTorque : winch__torque
    joint__lambdaLimit = winchTorque / winch__rad * dt

    let pulls = winch__pulls && !isOnBrake
    let extends = winch__extends && !isOnBrake

    let appliedLambda = max(0f, joint__lambdaSum)
    let pullOmega = pulls ? -(winch__omega * (1f - safediv(appliedLambda, joint__lambdaLimit))) : 0f
    let brakeOmega = isOnBrake ? safediv(max(0f, appliedLambda - joint__lambdaLimit), joint__lambdaLimit) * winch__omega : 0f
    let extendsOmega = extends ? winch__omega : 0f
    let omega = pullOmega + extendsOmega + brakeOmega

    // pull/push winch connection
    let diff = dt * winch__rad * omega
    phys_joint__jointLength = max(phys_joint__jointLength + diff, 0.5f)


[es(before=phys_joint_update)]
def phys_winch_update(info : UpdateStageInfoAct;
                      phys_joint__firstBody : EntityId;
                      phys_joint__firstAnchor : EntityId;
                      phys_joint__secondBody : EntityId;
                      phys_joint__secondAnchor : EntityId;
                      var phys_joint__jointLength : float&)
  query(phys_joint__firstAnchor) <| $ [es(REQUIRE=anchor__winch)] ()
    update_winch(info.dt, phys_joint__firstBody, phys_joint__jointLength)
  query(phys_joint__secondAnchor) <| $ [es(REQUIRE=anchor__winch)] ()
    update_winch(info.dt, phys_joint__secondBody, phys_joint__jointLength)

def break_rope(eid : EntityId; curTime : float)
  query(eid) <| $ [es] (phys_joint__firstBody, phys_joint__secondBody : EntityId;
                        phys_joint__firstBodyAnchor, phys_joint__secondBodyAnchor : float3;
                        rope_phys__segmentLen : float;
                        spline_gen_geometry__points : Point3List;
                        rope_phys__prevPos : Point3List;
                        joint__detachAtEnding : bool = false;
                        joint__detachedDuration : float = 3f)
    var splitAtIdx = 1
    if !joint__detachAtEnding
      let clampedRnd = clamp(abs(gauss_rnd(2)), 0f, 1f)
      let numPoints = length(spline_gen_geometry__points)
      let rndIdx = clamp(int(clampedRnd * float(numPoints - 1)), 2, numPoints - 3)
      let fromEnd = grnd() % 2 == 0
      splitAtIdx = fromEnd ? numPoints - rndIdx : rndIdx
      createEntity("detached_rope") <| $(init)
        using() <| $(var points : Point3List)
          using() <| $(var prevPos : Point3List)
            for i in range(splitAtIdx)
              push(points, spline_gen_geometry__points[i])
              push(prevPos, rope_phys__prevPos[i])
            set(init, "rope_phys__segmentLen", rope_phys__segmentLen)
            set(init, "spline_gen_geometry__points", points)
            set(init, "rope_phys__prevPos", prevPos)
            set(init, "phys_joint__firstBody", phys_joint__firstBody)
            set(init, "phys_joint__firstBodyAnchor", phys_joint__firstBodyAnchor)
            set(init, "rope_phys__despawnAt", curTime + joint__detachedDuration)
            set(init, "detached_rope__isOrigin", true)
    createEntity("detached_rope") <| $(init)
      using() <| $(var points : Point3List)
        using() <| $(var prevPos : Point3List)
          let remainingLen = length(spline_gen_geometry__points) - splitAtIdx
          for i in range(remainingLen)
            let idx = splitAtIdx + remainingLen - i - 1
            push(points, spline_gen_geometry__points[idx])
            push(prevPos, rope_phys__prevPos[idx])
          set(init, "rope_phys__segmentLen", rope_phys__segmentLen)
          set(init, "spline_gen_geometry__points", points)
          set(init, "rope_phys__prevPos", prevPos)
          set(init, "phys_joint__firstBody", phys_joint__secondBody)
          set(init, "phys_joint__firstBodyAnchor", phys_joint__secondBodyAnchor)
          set(init, "rope_phys__despawnAt", curTime + joint__detachedDuration)
  destroyEntity(eid)

[es(after=phys_joint_update)]
def rope_stress_update(info : UpdateStageInfoAct;
                       eid : EntityId;
                       joint__lambdaSum : float;
                       joint__stressLimit : float;
                       joint__stressStart : float;
                       var joint__stress : float&)
  // translate from impulse to force
  let force = joint__lambdaSum * safeinv(info.dt)
  if force < joint__stressStart
    return
  let increment = (force - joint__stressStart) / (joint__stressLimit - joint__stressStart)
  joint__stress += increment * info.dt
  if joint__stress < 1f
    return
  break_rope(eid, info.curTime)

[es(after=phys_joint_update, REQUIRE=joint__stressLimit)]
def rope_detach_from_ri(info : UpdateStageInfoAct;
                        eid : EntityId;
                        ri_extra__handle : riex_handle_t)
  if ri_extra__handle == RIEX_HANDLE_NULL || riex_isRiGenExtraValid(ri_extra__handle)
    return
  break_rope(eid, info.curTime)

[es(after=phys_winch_update)]
def update_rope(info : UpdateStageInfoAct;
                phys_joint__jointLength : float;
                rope_phys__numPoints : int;
                var rope_phys__segmentLen : float&)
  rope_phys__segmentLen = phys_joint__jointLength / float(rope_phys__numPoints - 1)

[es(before=phys_winch_update)]
def phys_joint_extension_update(info : UpdateStageInfoAct; var phys_joint__jointLength : float&; phys_joint__extensionMax : float;
                                phys_joint__firstAnchor : EntityId; phys_joint__secondAnchor : EntityId)
  query(phys_joint__firstAnchor) <| $ [es] (transform aka first_transform : float3x4)
    query(phys_joint__secondAnchor) <| $ [es] (transform aka second_transform : float3x4)
      phys_joint__jointLength = min(distance(first_transform[3], second_transform[3]), phys_joint__extensionMax)

[es(after=update_rope)]
def detached_rope_despawn_update(info : UpdateStageInfoAct; eid : EntityId; rope_phys__despawnAt : float)
  if info.curTime > rope_phys__despawnAt
    destroyEntity(eid)
