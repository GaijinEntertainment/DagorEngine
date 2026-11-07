require ecs
require math.base
require DngPhysKind
require DagorMath
require Dacoll
require danetlibs.phys_kind.es.phys_kind_events
require danetlibs.package_motorbike.main.motorbike_common

[es(tag=gameClient, after=after_net_phys_sync, REQUIRE=jolt_vehicle)]
def motorbike_heading_es(info : UpdatePhysEvent; transform : float3x4; phys_kind_net_phys : PhysKindActor;
                         var motorbike__headingQuat : float4&)
  let up = -phys_kind_net_phys.phys.currentState.gravDirection
  let fwd = motorbike_flat_forward(transform, up)
  if length_sq(fwd) == 0.0
    return
  motorbike__headingQuat = float4(dir_and_up_to_quat(fwd, up))


// 10 Hz state log of a bike with motorbike__trace on, for tuning
[es(tag=gameClient, after=after_net_phys_sync, REQUIRE=jolt_vehicle)]
def motorbike_trace_es(info : UpdatePhysEvent; transform : float3x4; var phys_kind_net_phys : PhysKindActor&; motorbike__trace : bool)
  if !motorbike__trace || int(info.curTime * 10.0) == int((info.curTime - info.dt) * 10.0)
    return
  let pos = transform[3]
  var t = 100.0
  var normal = float3(0.0, 1.0, 0.0)
  let up = -phys_kind_net_phys.phys.currentState.gravDirection
  let hasGround = traceray_normalized(pos + up, -up, t, normal, ETF_DEFAULT)
  let ground = hasGround ? "{t - 1.0} above ground, slope normal {normal}" : "no ground"
  assume state = phys_kind_net_phys.phys.currentState
  let v = state.velocity
  let w = state.omega
  let ct = phys_kind_ext(state, type<MotorbikeControls>)
  print("motorbike: t={info.curTime} pos={pos} {ground}; v={length(v)} (fwd {dot(v, transform[0])}, up {dot(v, up)}) w=(pitch {w.z} yaw {w.y} roll {w.x}); upright {dot(transform[1], up)} sinLean {motorbike_abs_sin_lean(transform, up)}; in fwd {int(ct.forward)} right {int(ct.right)} brake {int(ct.brake)}")
