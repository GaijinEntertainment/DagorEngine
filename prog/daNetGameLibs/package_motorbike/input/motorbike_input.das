require ecs
require math.base
require DagorInput
require DngPhysKind
require danetlibs.phys_kind.es.phys_kind_events
require danetlibs.package_motorbike.main.motorbike_common


[cpp_event(broadcast)]
struct UpdateStageUpdateInput
  curTime : float
  dt : float


[cpp_event(broadcast)]
struct EventDaInputInit
  init : bool


// the game declares the Motorbike action set: Move a joystick_move, Y throttle and brake, X steering; Reverse a digital_button
def init(var move : uint16&; var reverse : uint16&; activate : bool)
  move = activate ? get_action_handle("Motorbike.Move", TYPEGRP_STICK) : BAD_ACTION_HANDLE
  reverse = activate ? get_action_handle("Motorbike.Reverse", TYPEGRP_DIGITAL) : BAD_ACTION_HANDLE
  activate_action_set(get_action_set_handle("Motorbike"), activate)


[es(tag=input, on_appear, REQUIRE=(motorbike_input, jolt_vehicle))]
def motorbike_input_appear_es(evt : Event; var motorbike_input__aMove : uint16&; var motorbike_input__aReverse : uint16&)
  init(motorbike_input__aMove, motorbike_input__aReverse, true)


[es(tag=input, on_disappear, REQUIRE=(motorbike_input, jolt_vehicle))]
def motorbike_input_disappear_es(evt : Event; var motorbike_input__aMove : uint16&;
                                 var motorbike_input__aReverse : uint16&;
                                 var phys_kind_net_phys : PhysKindActor&)
  init(motorbike_input__aMove, motorbike_input__aReverse, false)
  memzero(*phys_kind_ext(phys_kind_net_phys.phys.producedCT, type<MotorbikeControls>))


[es(tag=input, REQUIRE=(motorbike_input, jolt_vehicle))]
def motorbike_input_init_es(evt : EventDaInputInit; var motorbike_input__aMove : uint16&; var motorbike_input__aReverse : uint16&)
  init(motorbike_input__aMove, motorbike_input__aReverse, evt.init)


[es(tag=input, REQUIRE=(motorbike_input, jolt_vehicle))]
def motorbike_input_es(info : UpdateStageUpdateInput; input__enabled : bool;
                       transform : float3x4;
                       motorbike_input__aMove : uint16;
                       motorbike_input__aReverse : uint16;
                       var motorbike_input__steer : float&;
                       var phys_kind_net_phys : PhysKindActor&)
  let move = get_analog_stick_action_state(motorbike_input__aMove)
  let reverse = get_digital_action_state(motorbike_input__aReverse)
  let moveX = input__enabled && move.bActive ? move.x : 0.f
  let moveY = input__enabled && move.bActive ? move.y : 0.f
  let forward = max(moveY, 0.f) * (reverse.bActive && reverse.bState ? -1.f : 1.f)
  // a disabled input holds the brakes, so the bike does not roll during the race countdown; full brakes in a lean make it spin out
  let brakeInput = input__enabled ? max(-moveY, 0.f) : 1.f
  let brake = brakeInput * square(1.f - motorbike_abs_sin_lean(transform, -phys_kind_net_phys.phys.currentState.gravDirection))

  // the bars reach full lock in about 1 s from any input and return 3 times faster; x > 0 steers right
  let rate = moveX != 0.f && moveX * motorbike_input__steer >= 0.f ? 1.f : 3.f
  motorbike_input__steer = move_to(motorbike_input__steer, moveX, info.dt, rate)

  var ct = phys_kind_ext(phys_kind_net_phys.phys.producedCT, type<MotorbikeControls>)
  ct.forward = int8(int(forward * 127.f))
  ct.right = int8(int(motorbike_input__steer * 127.f))
  ct.brake = uint8(int(brake * 255.f))
  ct.handBrake = uint8(0)
