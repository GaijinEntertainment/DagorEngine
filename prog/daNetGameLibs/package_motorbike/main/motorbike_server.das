require ecs
require math.base
require DngActor
require DngPhysKind
require DagorMath
require danetlibs.phys_kind.es.phys_kind_events
require danetlibs.package_motorbike.main.motorbike_common

[es(tag=server, on_appear, REQUIRE=jolt_vehicle)]
def motorbike_spawn_es(evt : Event; transform : float3x4; var motorbike__spawnTm : float3x4&)
  motorbike__spawnTm = transform


// a stale speed would read as a crash in the next frame
def motorbike_stand_up(eid : EntityId; tm : float3x4; var net_phys : PhysKindActor&; var prev_velocity : float3&)
  net_phys.phys.currentState.velocity = float3(0.0)
  net_phys.phys.currentState.omega = float3(0.0)
  prev_velocity = float3(0.0)
  teleport_phys_actor(eid, tm)


// a spawn point can float, as the ones for hovering ships do, and the level ground can load after the spawn
[es(tag=server, after=after_net_phys_sync, before=motorbike_crash_es, REQUIRE=jolt_vehicle)]
def motorbike_place_on_ground_es(info : UpdatePhysEvent; eid : EntityId; motorbike__spawnTm : float3x4; motorbike__wheelX : float2;
                                 var phys_kind_net_phys : PhysKindActor&; var motorbike__prevVelocity : float3&;
                                 var motorbike__placeOnGround : bool&)
  if !motorbike__placeOnGround
    return
  var tm = motorbike__spawnTm
  if motorbike_place_on_ground(tm, motorbike__wheelX, -phys_kind_net_phys.phys.currentState.gravDirection)
    motorbike__placeOnGround = false
    motorbike_stand_up(eid, tm, phys_kind_net_phys, motorbike__prevVelocity)


// The rider loses the bike on a hard hit or landing, with throttle in a deep lean (lowside) or braking in a lean (washout);
// a negative lean or speed-change threshold turns its check off. A crash is a fall, though right after a hit the bike is upright.
[es(tag=server, after=after_net_phys_sync, before=motorbike_fall_es, REQUIRE=jolt_vehicle)]
def motorbike_crash_es(info : UpdatePhysEvent; eid : EntityId; transform : float3x4; var phys_kind_net_phys : PhysKindActor&;
                       motorbike__crashSpeedChange : float2; motorbike__lowside : float4; motorbike__washout : float3;
                       var motorbike__prevVelocity : float3&; var motorbike__lowsideTime : float&; var motorbike__fallen : bool&)
  assume state = phys_kind_net_phys.phys.currentState
  let dv = state.velocity - motorbike__prevVelocity
  motorbike__prevVelocity = state.velocity
  let up = -state.gravDirection
  let fwd = motorbike_flat_forward(transform, up)
  if motorbike__fallen || length_sq(fwd) == 0.0
    return
  let speed = dot(state.velocity, fwd)
  let sinLean = motorbike_abs_sin_lean(transform, up)
  let ct = phys_kind_ext(state, type<MotorbikeControls>)

  let impact = motorbike__crashSpeedChange.x
  // an obstacle changes the speed, a ramp or a slope only turns the velocity
  let hit = impact >= 0.0 && abs(length(state.velocity) - length(state.velocity - dv)) > impact
  let landing = motorbike__crashSpeedChange.y >= 0.0 && dot(dv, up) > motorbike__crashSpeedChange.y
  let throttleInLean = (sinLean > sin(deg_to_rad(motorbike__lowside.x)) && speed > motorbike__lowside.y &&
                        float(int(ct.forward)) > motorbike__lowside.z * 127.0)
  motorbike__lowsideTime = throttleInLean ? motorbike__lowsideTime + info.dt : 0.0
  let lowside = motorbike__lowside.x >= 0.0 && motorbike__lowsideTime > motorbike__lowside.w
  let washout = (motorbike__washout.x >= 0.0 && sinLean > sin(deg_to_rad(motorbike__washout.x)) && speed > motorbike__washout.y &&
                 float(int(ct.brake)) > motorbike__washout.z * 255.0)
  if hit || landing || lowside || washout
    motorbike__fallen = true
    print("motorbike: {eid} crashes: hit {hit} landing {landing} lowside {lowside} washout {washout}")


// On its side the wheels still touch the ground, so the lean controller would fling a lying bike; jolt_vehicle__upright
// turns it off. At speed a deep lean is a turn and a high pitch a wheelie, so a tilt is a fall only once the bike nearly stops.
[es(tag=server, after=after_net_phys_sync, REQUIRE=jolt_vehicle)]
def motorbike_fall_es(info : UpdatePhysEvent; transform : float3x4; phys_kind_net_phys : PhysKindActor; motorbike__fallTiltDeg : float;
                      var motorbike__fallen : bool&; var jolt_vehicle__upright : bool&)
  let slow = length_sq(phys_kind_net_phys.phys.currentState.velocity) < 1.0
  let up = -phys_kind_net_phys.phys.currentState.gravDirection
  if !motorbike__fallen && slow && dot(transform[1], up) < cos(deg_to_rad(motorbike__fallTiltDeg))
    motorbike__fallen = true
  jolt_vehicle__upright = !motorbike__fallen


// the game decides when a fallen bike gets up, so a still rolling bike can get up too: its motion is dropped
[es(tag=server, REQUIRE=jolt_vehicle)]
def motorbike_get_up_es(evt : CmdMotorbikeGetUp; eid : EntityId; transform : float3x4; motorbike__wheelX : float2;
                        var phys_kind_net_phys : PhysKindActor&; var motorbike__prevVelocity : float3&; var motorbike__fallen : bool&;
                        var jolt_vehicle__upright : bool&)
  var tm = transform
  if !motorbike_place_on_ground(tm, motorbike__wheelX, -phys_kind_net_phys.phys.currentState.gravDirection)
    return
  motorbike__fallen = false
  jolt_vehicle__upright = true
  motorbike_stand_up(eid, tm, phys_kind_net_phys, motorbike__prevVelocity)
