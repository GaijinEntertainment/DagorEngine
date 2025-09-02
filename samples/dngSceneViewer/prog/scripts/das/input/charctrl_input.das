require app
require ecs
require math.base
require das.input.input_events
require DagorInput
require DagorSystem
require DagorMath
require DagorMathUtils
require das.character_controller.charctrl_state

let VIEW_HT_STAND  = 1.75f
let VIEW_HT_SPRINT = VIEW_HT_STAND - 0.2f
let VIEW_HT_CROUCH = VIEW_HT_STAND * 0.55f
let VIEW_HT_CRAWL  = VIEW_HT_STAND

let CAP_OFS_Y_STAND  = 0.5f
let CAP_OFS_Y_SPRINT = 0.55f
let CAP_OFS_Y_CROUCH = 0.3f
let CAP_OFS_Y_CRAWL  = 0.50f

let MOVE_VEL_STAND  = 13.f / 3.6f
let MOVE_VEL_SPRINT = MOVE_VEL_STAND * 2.0f
let MOVE_VEL_CROUCH = MOVE_VEL_STAND * 0.4f
let MOVE_VEL_CRAWL  = MOVE_VEL_STAND * 0.2f

let JUMP_HT_STAND  = 0.7f
let JUMP_HT_SPRINT = 0.5f
let JUMP_HT_CROUCH = 0.3f

def vel0_for_jump_ht(ht : float)
  return sqrt(2.f * 9.81f * ht)

def init(var move : uint16&;
         var look : uint16&;
         var jump : uint16&;
         var sprint : uint16&;
         var crouch : uint16&;
         var crawl : uint16&)
  move    = get_action_handle("CharCtrl.Move", TYPEGRP_STICK)
  look    = get_action_handle("CharCtrl.Look", TYPEGRP_STICK)
  jump    = get_action_handle("CharCtrl.Jump", TYPEGRP_DIGITAL)
  sprint  = get_action_handle("CharCtrl.Sprint", TYPEGRP_DIGITAL)
  crouch  = get_action_handle("CharCtrl.Crouch", TYPEGRP_DIGITAL)
  crawl   = get_action_handle("CharCtrl.Crawl", TYPEGRP_DIGITAL)
  let charctrlSetHandle = get_action_set_handle("CharCtrl")
  if charctrlSetHandle != BAD_ACTION_SET_HANDLE
    activate_action_set(charctrlSetHandle, true)


def reset(var move : uint16&;
          var look : uint16&;
          var jump : uint16&;
          var sprint : uint16&;
          var crouch : uint16&;
          var crawl : uint16&)
  move = BAD_ACTION_HANDLE
  look = BAD_ACTION_HANDLE
  jump = BAD_ACTION_HANDLE
  sprint = BAD_ACTION_HANDLE
  crouch = BAD_ACTION_HANDLE
  crawl = BAD_ACTION_HANDLE
  let charctrlSetHandle = get_action_set_handle("CharCtrl")
  if charctrlSetHandle != BAD_ACTION_SET_HANDLE
    activate_action_set(charctrlSetHandle, false)


[es(tag=input, on_appear, REQUIRE=charctrl_input)]
def charctrl_input_appear_es(evt : Event; eid : EntityId;
                             transform : float3x4 const?;
                             var charctrl__look_ang : float2&;
                             var charctrl_input__aMove : uint16&;
                             var charctrl_input__aLook : uint16&;
                             var charctrl_input__aJump : uint16&;
                             var charctrl_input__aSprint : uint16&;
                             var charctrl_input__aCrouch : uint16&;
                             var charctrl_input__aCrawl : uint16&)
  let thisEid = eid
  let found = ecs::find_query() <| $ [es(REQUIRE=charctrl_input, REQUIRE_NOT=deadEntity)] (eid : EntityId)
    return eid != thisEid
  if found
    logerr("Attempt to create > 1 charctrl_input component")

  charctrl__look_ang = float2(euler_from_quat(float4(math::quat(transform ?? IDENT_TM))).xz)
  print("quat={float4(math::quat(transform ?? IDENT_TM))} charctrl__look_ang={charctrl__look_ang}")

  init(charctrl_input__aMove, charctrl_input__aLook, charctrl_input__aJump,
    charctrl_input__aSprint, charctrl_input__aCrouch, charctrl_input__aCrawl)



[es(tag=input, on_disappear, REQUIRE=charctrl_input)]
def charctrl_input_destroyed_es(evt : Event;
                                var charctrl_input__aMove : uint16&;
                                var charctrl_input__aLook : uint16&;
                                var charctrl_input__aJump : uint16&;
                                var charctrl_input__aSprint : uint16&;
                                var charctrl_input__aCrouch : uint16&;
                                var charctrl_input__aCrawl : uint16&)
  reset(charctrl_input__aMove, charctrl_input__aLook, charctrl_input__aJump,
    charctrl_input__aSprint, charctrl_input__aCrouch, charctrl_input__aCrawl)


[es(tag=input, REQUIRE=charctrl_input)]
def charctrl_input_init_es(evt : EventDaInputInit; input__enabled : bool;
                           var charctrl_input__aMove : uint16&;
                           var charctrl_input__aLook : uint16&;
                           var charctrl_input__aJump : uint16&;
                           var charctrl_input__aSprint : uint16&;
                           var charctrl_input__aCrouch : uint16&;
                           var charctrl_input__aCrawl : uint16&)
  if !input__enabled
    return
  if evt.init
    init(charctrl_input__aMove, charctrl_input__aLook, charctrl_input__aJump,
      charctrl_input__aSprint, charctrl_input__aCrouch, charctrl_input__aCrawl)
  else
    reset(charctrl_input__aMove, charctrl_input__aLook, charctrl_input__aJump,
      charctrl_input__aSprint, charctrl_input__aCrouch, charctrl_input__aCrawl)


[es(tag=input, REQUIRE=charctrl_input)]
def charctrl_input_es(info : UpdateStageUpdateInput; input__enabled : bool;
                      charctrl_input__aMove : uint16;
                      charctrl_input__aLook : uint16;
                      charctrl_input__aJump : uint16;
                      charctrl_input__aSprint : uint16;
                      charctrl_input__aCrouch : uint16;
                      charctrl_input__aCrawl : uint16;
                      var charctrl__state : int3&; // CCStandType, CCMoveType, CCStateFlag
                      var charctrl__move_vel : float2&;
                      var charctrl__look_ang : float2&;
                      var charctrl__jump_vel : float&;
                      var charctrl__capsule_dir : float3&;
                      var charctrl__capsule_ofs : float2&;
                      var charctrl__view_ht : float&)
  let morph_viscosity = 0.17f
  let move_viscosity = 0.08f
  let dt = info.dt

  if !input__enabled
    charctrl__move_vel.x = approach(charctrl__move_vel.x, 0.f, dt, move_viscosity)
    charctrl__move_vel.y = approach(charctrl__move_vel.y, 0.f, dt, move_viscosity)
    charctrl__state.xy = int2(int(CCStandType.ESS_STAND), int(CCMoveType.EMS_STAND))
    return

  let aLook = get_analog_stick_action_state(charctrl_input__aLook)
  if aLook.bActive
    charctrl__look_ang.x = charctrl__look_ang.x + aLook.x
    charctrl__look_ang.y = clamp(norm_s_ang(charctrl__look_ang.y + aLook.y), -70.f * PI / 180.f, +70.f * PI / 180.f)

  let aSprint = get_digital_action_state(charctrl_input__aSprint)
  let aCrouch = get_digital_action_state(charctrl_input__aCrouch)
  let aCrawl = get_digital_action_state(charctrl_input__aCrawl)
  let aJump = get_digital_action_state(charctrl_input__aJump)

  var move_vel : float = MOVE_VEL_STAND
  if aSprint.bActive && aSprint.bState
    move_vel = MOVE_VEL_SPRINT
    charctrl__view_ht = approach(charctrl__view_ht, VIEW_HT_SPRINT, dt, morph_viscosity)
    charctrl__capsule_dir = normalize(approach(charctrl__capsule_dir, normalize(float3(0.f, 1.f, 0.1f)), dt, morph_viscosity))
    charctrl__capsule_ofs = approach(charctrl__capsule_ofs, float2(0.f, CAP_OFS_Y_SPRINT), dt, morph_viscosity)
    charctrl__jump_vel = aJump.bActive && aJump.bState && !(aCrawl.bActive && aCrawl.bState) ? vel0_for_jump_ht(JUMP_HT_SPRINT) : 0.f
    charctrl__state.x = int(CCStandType.ESS_STAND)
    charctrl__state.y = int(CCMoveType.EMS_SPRINT)
  elif aCrouch.bActive && aCrouch.bState
    move_vel = MOVE_VEL_CROUCH
    charctrl__view_ht = approach(charctrl__view_ht, VIEW_HT_CROUCH, dt, morph_viscosity)
    charctrl__capsule_dir = normalize(approach(charctrl__capsule_dir, float3(0.f, 1.f, 0.f), dt, morph_viscosity))
    charctrl__capsule_ofs = approach(charctrl__capsule_ofs, float2(0.f, CAP_OFS_Y_CROUCH), dt, morph_viscosity)
    charctrl__jump_vel = aJump.bActive && aJump.bState && !(aCrawl.bActive && aCrawl.bState) ? vel0_for_jump_ht(JUMP_HT_CROUCH) : 0.f
    charctrl__state.x = int(CCStandType.ESS_CROUCH)
    charctrl__state.y = int(CCMoveType.EMS_WALK)
  elif aCrawl.bActive && aCrawl.bState
    move_vel = MOVE_VEL_CRAWL
    charctrl__view_ht = approach(charctrl__view_ht, VIEW_HT_CRAWL, dt, morph_viscosity)
    charctrl__capsule_dir = normalize(approach(charctrl__capsule_dir, float3(0.f, 0.f, 1.f), dt, morph_viscosity))
    charctrl__capsule_ofs = approach(charctrl__capsule_ofs, float2(0.5f, CAP_OFS_Y_CRAWL), dt, morph_viscosity)
    charctrl__jump_vel = 0.f
    charctrl__state.x = int(CCStandType.ESS_CRAWL)
    charctrl__state.y = int(CCMoveType.EMS_WALK)
  else
    charctrl__view_ht = approach(charctrl__view_ht, VIEW_HT_STAND, dt, morph_viscosity)
    charctrl__capsule_dir = normalize(approach(charctrl__capsule_dir, float3(0.f, 1.f, 0.f), dt, morph_viscosity))
    charctrl__capsule_ofs = approach(charctrl__capsule_ofs, float2(0.f, CAP_OFS_Y_STAND), dt, morph_viscosity)
    charctrl__jump_vel = aJump.bActive && aJump.bState && !(aCrawl.bActive && aCrawl.bState) ? vel0_for_jump_ht(JUMP_HT_STAND) : 0.f
    charctrl__state.x = int(CCStandType.ESS_STAND)
    charctrl__state.y = int(CCMoveType.EMS_RUN)

  let aMove = get_analog_stick_action_state(charctrl_input__aMove)
  if aMove.bActive
    charctrl__move_vel.x = approach(charctrl__move_vel.x, move_vel * aMove.x, dt, move_viscosity)
    charctrl__move_vel.y = approach(charctrl__move_vel.y, move_vel * aMove.y, dt, move_viscosity)
  else
    charctrl__move_vel.x = approach(charctrl__move_vel.x, 0.f, dt, move_viscosity)
    charctrl__move_vel.y = approach(charctrl__move_vel.y, 0.f, dt, move_viscosity)

  if charctrl__state.y != int(CCMoveType.EMS_STAND) && length(charctrl__move_vel) < 0.01
    charctrl__state.y = int(CCMoveType.EMS_STAND)

  if charctrl__state.y == int(CCStandType.ESS_CRAWL)
    charctrl__state.z |= int(CCStateFlag.ST_CRAWL)
  else
    charctrl__state.z &= ~int(CCStateFlag.ST_CRAWL)

  if charctrl__state.y == int(CCStandType.ESS_CROUCH)
    charctrl__state.z |= int(CCStateFlag.ST_CROUCH)
  else
    charctrl__state.z &= ~int(CCStateFlag.ST_CROUCH)
