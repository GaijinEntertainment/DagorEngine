require app
require ecs
require math.base
require outer_space.events_outer_space
require outer_space.es.input.input_events
require outer_space.es.space_ship_common
require DagorInput
require DagorSystem
require DagorMath
require DagorMathUtils
require DngNet
require DngPhysObj
require TouchInput


def init(var accel_xz : uint16&;
         var accel_ry_rz : uint16&;
         var accel_y : uint16&;
         var accel_rx : uint16&;
         var rotate : uint16&;
         var look_around : uint16&;
         var fwd_move_stab : uint16&;
         var change_stab_ht : uint16&)
  accel_xz    = get_action_handle("SpaceShip.AccelXZ", TYPEGRP_STICK)
  accel_ry_rz = get_action_handle("SpaceShip.AccelRyRz", TYPEGRP_STICK)
  accel_y     = get_action_handle("SpaceShip.AccelY", TYPEGRP_AXIS)
  accel_rx    = get_action_handle("SpaceShip.AccelRx", TYPEGRP_AXIS)
  rotate      = get_action_handle("SpaceShip.RotateView", TYPEGRP_STICK)
  look_around = get_action_handle("SpaceShip.LookAround", TYPEGRP_DIGITAL)
  fwd_move_stab = get_action_handle("SpaceShip.FwdMoveStab", TYPEGRP_DIGITAL)
  change_stab_ht = get_action_handle("SpaceShip.ChangeStabHt", TYPEGRP_AXIS)
  let spaceshipSetHandle = get_action_set_handle("SpaceShip")
  if spaceshipSetHandle != BAD_ACTION_SET_HANDLE
    activate_action_set(spaceshipSetHandle, true)


def reset(var accel_xz : uint16&;
          var accel_ry_rz : uint16&;
          var accel_y : uint16&;
          var accel_rx : uint16&;
          var rotate : uint16&;
          var look_around : uint16&;
          var fwd_move_stab : uint16&;
          var change_stab_ht : uint16&)
  accel_xz = BAD_ACTION_HANDLE
  accel_ry_rz = BAD_ACTION_HANDLE
  accel_y = BAD_ACTION_HANDLE
  accel_rx = BAD_ACTION_HANDLE
  rotate = BAD_ACTION_HANDLE
  look_around = BAD_ACTION_HANDLE
  fwd_move_stab = BAD_ACTION_HANDLE
  change_stab_ht = BAD_ACTION_HANDLE
  let spaceshipSetHandle = get_action_set_handle("SpaceShip")
  if spaceshipSetHandle != BAD_ACTION_SET_HANDLE
    activate_action_set(spaceshipSetHandle, false)


[es(tag=input, on_appear, REQUIRE=spaceship_input)]
def spaceship_input_appear_es(evt : Event; eid : EntityId; var phys_obj_net_phys : PhysObjActor&;
                              transform : float3x4;
                              var spaceship_input__aAccelXZ : uint16&;
                              var spaceship_input__aAccelRyRz : uint16&;
                              var spaceship_input__aAccelY : uint16&;
                              var spaceship_input__aAccelRx : uint16&;
                              var spaceship_input__aRotateView : uint16&;
                              var spaceship_input__aLookAround : uint16&;
                              var spaceship_input__aFwdMoveStab : uint16&;
                              var spaceship_input__aChangeStabHt : uint16&;
                              spaceship__use_stabilization : bool = false)
  let thisEid = eid
  let found = ecs::find_query() <| $ [es(REQUIRE=spaceship_input, REQUIRE_NOT=deadEntity)] (eid : EntityId)
    return eid != thisEid
  if found
    logerr("Attempt to create > 1 spaceship_input component")

  var producedCT = get_phys_obj_produced_controls(phys_obj_net_phys)
  producedCT.rotateView_base_quat = math::quat(transform)
  producedCT.use_stabilization = spaceship__use_stabilization

  init(spaceship_input__aAccelXZ, spaceship_input__aAccelRyRz, spaceship_input__aAccelY, spaceship_input__aAccelRx,
    spaceship_input__aRotateView, spaceship_input__aLookAround, spaceship_input__aFwdMoveStab, spaceship_input__aChangeStabHt)


[es(tag=input, on_disappear, REQUIRE=spaceship_input)]
def spaceship_input_destroyed_es(evt : Event;
                                 var spaceship_input__aAccelXZ : uint16&;
                                 var spaceship_input__aAccelRyRz : uint16&;
                                 var spaceship_input__aAccelY : uint16&;
                                 var spaceship_input__aAccelRx : uint16&;
                                 var spaceship_input__aRotateView : uint16&;
                                 var spaceship_input__aLookAround : uint16&;
                                 var spaceship_input__aFwdMoveStab : uint16&;
                                 var spaceship_input__aChangeStabHt : uint16&)
  reset(spaceship_input__aAccelXZ, spaceship_input__aAccelRyRz, spaceship_input__aAccelY, spaceship_input__aAccelRx,
    spaceship_input__aRotateView, spaceship_input__aLookAround, spaceship_input__aFwdMoveStab, spaceship_input__aChangeStabHt)


[es(tag=input, REQUIRE=spaceship_input)]
def spaceship_input_init_es(evt : EventDaInputInit; input__enabled : bool;
                            var spaceship_input__aAccelXZ : uint16&;
                            var spaceship_input__aAccelRyRz : uint16&;
                            var spaceship_input__aAccelY : uint16&;
                            var spaceship_input__aAccelRx : uint16&;
                            var spaceship_input__aRotateView : uint16&;
                            var spaceship_input__aLookAround : uint16&;
                            var spaceship_input__aFwdMoveStab : uint16&;
                            var spaceship_input__aChangeStabHt : uint16&)
  if !input__enabled
    return
  if evt.init
    init(spaceship_input__aAccelXZ, spaceship_input__aAccelRyRz, spaceship_input__aAccelY, spaceship_input__aAccelRx,
      spaceship_input__aRotateView, spaceship_input__aLookAround, spaceship_input__aFwdMoveStab, spaceship_input__aChangeStabHt)
  else
    reset(spaceship_input__aAccelXZ, spaceship_input__aAccelRyRz, spaceship_input__aAccelY, spaceship_input__aAccelRx,
      spaceship_input__aRotateView, spaceship_input__aLookAround, spaceship_input__aFwdMoveStab, spaceship_input__aChangeStabHt)


[es(tag=input, REQUIRE=spaceship_input)]
def spaceship_input_es(info : UpdateStageUpdateInput; var phys_obj_net_phys : PhysObjActor&; input__enabled : bool;
                       spaceship_input__aAccelXZ : uint16;
                       spaceship_input__aAccelRyRz : uint16;
                       spaceship_input__aAccelY : uint16;
                       spaceship_input__aAccelRx : uint16;
                       spaceship_input__aRotateView : uint16;
                       spaceship_input__aLookAround : uint16;
                       spaceship_input__aFwdMoveStab : uint16;
                       spaceship_input__aChangeStabHt : uint16;
                       transform : float3x4;
                       var spaceship__min_target_dHt : float&)

  var producedCT = get_phys_obj_produced_controls(phys_obj_net_phys)
  assume spaceship_thrust__lin = producedCT.lin
  assume spaceship_thrust__rot = producedCT.rot
  assume spaceship__rotateView_rad = producedCT.rotateView_rad
  assume spaceship__rotateView_base_quat = producedCT.rotateView_base_quat
  assume spaceship__free_look_around = producedCT.free_look_around
  assume spaceship__use_stabilization = producedCT.use_stabilization
  assume spaceship__use_fwd_stabilization = producedCT.use_fwd_stabilization

  if !input__enabled
    // when input is deactivated on final countdown (on game start) we allow rotating camera
    spaceship__free_look_around = true
    let aRotateView = get_analog_stick_action_state(spaceship_input__aRotateView)
    if aRotateView.bActive
      spaceship__rotateView_rad.x = norm_s_ang(spaceship__rotateView_rad.x + aRotateView.x / 512.f)
      spaceship__rotateView_rad.y = clamp(norm_s_ang(spaceship__rotateView_rad.y + aRotateView.y / 512.f), -70.f * PI / 180.f, +70.f * PI / 180.f)
    return

  var use_free_look_around : bool = false
  let aLookAround = get_digital_action_state(spaceship_input__aLookAround)
  if aLookAround.bActive && aLookAround.bState
    use_free_look_around = true

  let aAccelXZ = get_analog_stick_action_state(spaceship_input__aAccelXZ)
  if aAccelXZ.bActive
    spaceship_thrust__lin.x = 200.0f * aAccelXZ.y
    spaceship_thrust__lin.z = -200.0f * aAccelXZ.x

  let aAccelRyRz = get_analog_stick_action_state(spaceship_input__aAccelRyRz)
  if aAccelRyRz.bActive
    spaceship_thrust__rot.y = -5.0f * aAccelRyRz.x
    spaceship_thrust__rot.z =  5.0f * aAccelRyRz.y

  let aAccelY = get_analog_axis_action_state(spaceship_input__aAccelY)
  if aAccelY.bActive
    spaceship_thrust__lin.y = 200.0f * aAccelY.x

  let aAccelRx = get_analog_axis_action_state(spaceship_input__aAccelRx)
  if aAccelRx.bActive
    spaceship_thrust__rot.x = -5.0f * aAccelRx.x

  let aChangeStabHt = get_analog_axis_action_state(spaceship_input__aChangeStabHt)
  if aChangeStabHt.bActive
    spaceship__min_target_dHt = 4.f * aChangeStabHt.x
  else
    spaceship__min_target_dHt = 0.f

  let cur_tm_q = math::quat(transform)
  if !use_free_look_around
    // retarget spaceship__rotateView_rad to current transform leaving direction the same in world space
    var q : quat
    euler_heading_attitude_to_quat(spaceship__rotateView_rad.x, spaceship__rotateView_rad.y, q)
    var roll_ : float
    q = quat(quat_mul(float4(inverse(quat(cur_tm_q))), quat_mul(spaceship__rotateView_base_quat, float4(q))))
    quat_to_euler(q, spaceship__rotateView_rad.x, spaceship__rotateView_rad.y, roll_)
  // update used frame for spaceship__rotateView_rad
  spaceship__rotateView_base_quat = cur_tm_q

  let aRotateView = get_analog_stick_action_state(spaceship_input__aRotateView)
  if aRotateView.bActive
    spaceship__rotateView_rad.x = norm_s_ang(spaceship__rotateView_rad.x + aRotateView.x / 512.f)
    spaceship__rotateView_rad.y = clamp(norm_s_ang(spaceship__rotateView_rad.y + aRotateView.y / 512.f), -70.f * PI / 180.f, +70.f * PI / 180.f)

  spaceship__free_look_around = use_free_look_around
  let aFwdMoveStab = get_digital_action_state(spaceship_input__aFwdMoveStab)
  spaceship__use_fwd_stabilization = aFwdMoveStab.bActive && aFwdMoveStab.bState
