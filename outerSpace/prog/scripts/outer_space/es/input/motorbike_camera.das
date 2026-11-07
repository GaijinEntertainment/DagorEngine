require ecs
require math.base
require outer_space.events_outer_space
require outer_space.es.input.input_events
require DagorInput
require DagorMath

// The ship TPS camera follows spaceship__rotateView_* from the bike heading; Motorbike.Look turns it, and a second after the look
// it eases back behind the bike, or to look backward while Motorbike.Reverse is held.

[es(tag=input, on_appear)]
def motorbike_camera_appear_es(evt : Event; motorbike_camera__restPitch : float; var motorbike_camera__aLook : uint16&;
                               var spaceship__rotateView_rad : float2&)
  motorbike_camera__aLook = get_action_handle("Motorbike.Look", TYPEGRP_STICK)
  spaceship__rotateView_rad.y = motorbike_camera__restPitch


[es(tag=input)]
def motorbike_camera_init_es(evt : EventDaInputInit; var motorbike_camera__aLook : uint16&)
  motorbike_camera__aLook = evt.init ? get_action_handle("Motorbike.Look", TYPEGRP_STICK) : BAD_ACTION_HANDLE


[es(tag=input)]
def motorbike_camera_es(info : UpdateStageUpdateInput;
                        motorbike__headingQuat : float4;
                        motorbike_camera__aLook : uint16;
                        motorbike_input__aReverse : uint16;
                        motorbike_camera__restPitch : float;
                        var motorbike_camera__lookIdleTime : float&;
                        var spaceship__rotateView_base_quat : float4&;
                        var spaceship__rotateView_rad : float2&)
  spaceship__rotateView_base_quat = motorbike__headingQuat

  let look = get_analog_stick_action_state(motorbike_camera__aLook)
  let lookDelta = look.bActive ? abs(look.x) + abs(look.y) : 0.f
  if lookDelta >= 0.1f
    motorbike_camera__lookIdleTime = 0.f
    spaceship__rotateView_rad.x = norm_s_ang(spaceship__rotateView_rad.x + look.x / 512.f)
    let pitchLimit = 70.f * PI / 180.f
    spaceship__rotateView_rad.y = clamp(norm_s_ang(spaceship__rotateView_rad.y + look.y / 512.f), -pitchLimit, pitchLimit)
  else
    motorbike_camera__lookIdleTime += info.dt
    if motorbike_camera__lookIdleTime > 1.f
      let reverse = get_digital_action_state(motorbike_input__aReverse)
      let restYaw = reverse.bActive && reverse.bState ? PI : 0.f
      let keep = exp(-info.dt / 0.4f)
      // the shortest way round, so the view never swings through the long side
      spaceship__rotateView_rad.x = norm_s_ang(restYaw + norm_s_ang(spaceship__rotateView_rad.x - restYaw) * keep)
      spaceship__rotateView_rad.y = motorbike_camera__restPitch + (spaceship__rotateView_rad.y - motorbike_camera__restPitch) * keep
