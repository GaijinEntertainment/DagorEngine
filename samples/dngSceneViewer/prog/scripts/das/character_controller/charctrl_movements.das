require app
require ecs
require math.base
require das.main_events
require DagorSystem
require DagorMath
require DagorMathUtils
require Dacoll
require Daphys
require das.character_controller.charctrl_state
//require DagorDebug3D

[event(unicast)]
struct SimulateCharCtrlDynamics
  tick : int
  dt : float

[event(unicast)]
struct IntegrateCharCtrlPose
  tick : int
  dt : float

let phys_step_dt = 0.01;
let phys_back_tick_delta = 20;

def get_phys_tick_at(t : float) : int
  return int(t / phys_step_dt + 0.5f)

def get_phys_tick_at_rel(dt : float) : int
  return get_phys_tick_at(get_sync_time() + dt)


def collide_capcule_against_phys_world(var phys__pos : float3&;
                                       var phys__vel : float3&;
                                       var phys__wvel : float3&;
                                       heading : float;
                                       charctrl__capsule_dir : float3;
                                       charctrl__capsule_ofs : float2;
                                       charctrl__view_ht : float)
  let tm_orient = quat_mul(quat_from_euler(heading, 0.f, 0.f), quat_from_unit_arc(float3(0.f, 1.f, 0.), charctrl__capsule_dir))
  let invMass = 1.lf / 60.lf
  let invMoi = DPoint3(0.lf, 0.lf, 0.lf)
  let cap_rad = 0.45f
  let cap_half_ht = (charctrl__view_ht - charctrl__capsule_ofs.y - cap_rad) * 0.5f
  let cap_c = quat_mul_vec(tm_orient, float3(0.f, (cap_half_ht + cap_rad) * (1.f - charctrl__capsule_ofs.x), 0.f)) + float3(0.f, charctrl__capsule_ofs.y, 0)
  var tm : float3x4;
  make_tm(DagorMath::quat(tm_orient), phys__pos + cap_c, tm)
  //draw_debug_capsule_buffered(tm[3] - cap_half_ht * tm[1], tm[3] + cap_half_ht * tm[1], cap_rad, E3DCOLOR(0xFF00FF00), 16, 2000)

  dacoll_use_capsule_collision() <| $(var coll : CollisionObject)
    dacoll_set_vert_capsule_shape_size(coll, cap_rad, cap_half_ht * 2.f)
    dacoll_set_collision_object_tm(coll, IDENT_TM)

    var totalPenetration : float
    var dest_pos  = DPoint3(tm[3])
    var dest_vel  = DPoint3(phys__vel)
    var dest_wvel = DPoint3(phys__wvel)
    let orient    = DagorMath::quat(tm)

    test_collision_world_ex(coll, tm, charctrl__view_ht) <| $(contacts : array<CollisionContactData>#)
      for contact in contacts
        if contact.depth < 0.0
          totalPenetration += -contact.depth
      using() <| $(var contactParams : ResolveContactParams#)
        contactParams.friction = 0.42lf
        contactParams.minFriction = -1.lf
        contactParams.maxFriction = 1.lf
        contactParams.bounce = 0.01lf
        contactParams.minSpeedForBounce = 0.lf
        contactParams.energyConservation = 0.95lf
        resolve_contacts(dest_pos, orient, dest_vel, dest_wvel, contacts, invMass, invMoi, contactParams, 5);
      resolve_penetration(dest_pos, orient, contacts, invMass, invMoi, 1.lf, true, 1)

    phys__pos += float3(dest_pos) - tm[3]
    phys__vel  = float3(dest_vel)
    phys__wvel = float3(dest_wvel)


[es]
def charctrl_update_es(evt : UpdateStageInfoAct;
                       var phys_global__tick_residual : float&;
                       var phys_global__tick : int&)
  let num_ticks = int((evt.dt + phys_global__tick_residual) / phys_step_dt)
  phys_global__tick_residual = (evt.dt + phys_global__tick_residual) - float(num_ticks) * phys_step_dt
  if phys_global__tick == -1
    phys_global__tick = get_phys_tick_at_rel(0.)
  if abs(phys_global__tick - get_phys_tick_at_rel(0.)) > phys_back_tick_delta / 2
    phys_global__tick = get_phys_tick_at_rel(0.)

  for t in range(0, num_ticks)
    broadcastEventImmediate([[PhysUpdateEvent tick=phys_global__tick + t, dt=phys_step_dt]])
  phys_global__tick += num_ticks


[es(REQUIRE=hero)]
def charctrl_phys_updated_es(evt : PhysUpdateEvent; eid : EntityId)
  sendEventImmediate(eid, [[SimulateCharCtrlDynamics tick=evt.tick, dt=evt.dt]])
  sendEventImmediate(eid, [[IntegrateCharCtrlPose tick=evt.tick, dt=evt.dt]])

[es]
def charctrl_phys_simulate_es(evt : SimulateCharCtrlDynamics;
                              phys__at_tick : int;
                              var phys__vel : float3&;
                              var phys__wvel : float3&;
                              var phys__pos : float3&;
                              var charctrl__state : int3&; // CCStandType, CCMoveType, CCStateFlag
                              phys__rot : float4;
                              charctrl__move_vel : float2;
                              charctrl__look_ang : float2;
                              charctrl__jump_vel : float;
                              charctrl__capsule_dir : float3;
                              charctrl__capsule_ofs : float2;
                              charctrl__view_ht : float)
  let tick = evt.tick
  if tick == phys__at_tick
    return
  let dt = evt.dt

  var t = charctrl__capsule_ofs.y + 0.5
  var norm : float3
  var p0 : float3 = phys__pos;
  p0.y += charctrl__capsule_ofs.y
  let on_ground = traceray_normalized(p0, float3(0.f, -1.f, 0.f), t, norm)
  if on_ground && t < charctrl__capsule_ofs.y + 0.02f
    let heading = euler_from_quat(phys__rot).x
    let move_vel = quat_mul_vec(quat_from_euler(heading, 0.f, 0.f), float3(charctrl__move_vel.x, 0.f, charctrl__move_vel.y))
    let move_up_vel = min((charctrl__capsule_ofs.y - t) / dt, 3.f)

    phys__vel = float3(move_vel.x, move_up_vel, move_vel.z)
    if charctrl__jump_vel > 0.f
      phys__vel.y += charctrl__jump_vel
    if length_sq(phys__vel) < 1e-8f
      phys__vel = float3(0.f, 0.f, 0.f)
    charctrl__state.z |= int(CCStateFlag ST_ON_GROUND)

  elif on_ground && t < charctrl__capsule_ofs.y + 0.05f && charctrl__jump_vel > 0.f
    phys__vel.y = charctrl__jump_vel
    charctrl__state.z |= int(CCStateFlag ST_JUMP)

  else
    phys__vel.y -= 9.81 * dt
    if t > charctrl__capsule_ofs.y + 0.2f
      charctrl__state.z &= ~int(CCStateFlag ST_ON_GROUND)

  let dq = quat_mul(quat_conjugate(phys__rot), quat_from_euler(charctrl__look_ang.x, 0.f, charctrl__look_ang.y))
  phys__wvel = float3(normalize(dq).xyz) / dt * 0.5f

  if length_sq(phys__wvel) < 1e-8f
    phys__wvel = float3(0.f, 0.f, 0.f)

  collide_capcule_against_phys_world(phys__pos, phys__vel, phys__wvel,
    euler_from_quat(phys__rot).x, charctrl__capsule_dir, charctrl__capsule_ofs, charctrl__view_ht)

[es]
def charctrl_phys_integrate_pose_es(evt : IntegrateCharCtrlPose;
                                    var phys__at_tick : int&;
                                    var phys__pos : float3&;
                                    var phys__rot : float4&;
                                    phys__vel : float3;
                                    phys__wvel : float3)
  let tick = evt.tick
  if tick == phys__at_tick
    return
  let dt = evt.dt
  let orientationInc = phys__wvel * dt
  let quatInc = quat(orientationInc, length(orientationInc))
  let orient = DagorMath::quat(phys__rot)

  phys__rot = float4(normalize(orient * quatInc))
  phys__pos += phys__vel * dt
  phys__at_tick = tick
  //print("pos={phys__pos}")
