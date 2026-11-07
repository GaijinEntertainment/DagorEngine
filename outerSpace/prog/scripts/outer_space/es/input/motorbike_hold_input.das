require ecs
require math.base
require DagorInput
require DngPhysKind
require danetlibs.phys_kind.es.phys_kind_events
require outer_space.events_outer_space
require danetlibs.package_motorbike.main.motorbike_common


// a stopped bike holds its brakes, so it does not roll back on a slope;
// with Reverse held it rolls free to back up full brakes in a lean make it spin out, as in motorbike_input_es
[es(tag=input, REQUIRE=(motorbike_input, jolt_vehicle), after=motorbike_input_es)]
def motorbike_hold_at_stop_input_es(info : UpdateStageUpdateInput; transform : float3x4; motorbike_test__holdSpeed : float;
                                    motorbike_input__aReverse : uint16; var phys_kind_net_phys : PhysKindActor&)
  let reverse = get_digital_action_state(motorbike_input__aReverse)
  if reverse.bActive && reverse.bState
    return
  assume st = phys_kind_net_phys.phys.currentState
  var ct = phys_kind_ext(phys_kind_net_phys.phys.producedCT, type<MotorbikeControls>)
  if ct.forward != int8(0) || dot(st.velocity, transform[0]) >= motorbike_test__holdSpeed
    return
  let hold = 255.f * square(1.f - motorbike_abs_sin_lean(transform, -st.gravDirection))
  ct.brake = uint8(max(int(ct.brake), int(hold)))
