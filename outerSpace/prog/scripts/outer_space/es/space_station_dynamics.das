require app
require ecs
require math.base
require outer_space.events_outer_space
require DagorSystem
require DagorMath
require DagorMathUtils
require DngPhysObj
require PhysObj
require Dacoll

let MAX_LIN_FORCE = 9000.f
let MAX_ROT_FORCE = MAX_LIN_FORCE

[es(tag=server)]
def recv_control_inputs_es(evt : PassControlInputs;
                           //eid : EntityId;
                           var spaceship_thrust__lin : float3&;
                           var spaceship_thrust__rot : float3&;
                           var spaceship__rotateView_rad : float2&;
                           var spaceship__rotateView_base_quat : float4&;
                           var spaceship__min_target_ht : float&;
                           var spaceship__use_stabilization : bool&;
                           var spaceship__use_fwd_stabilization : bool&;
                           var spaceship__free_look_around : bool&)
  //print("recv PassControlInputs lin={evt.lin} rot={evt.rot} eid={eid}")
  spaceship_thrust__lin = evt.lin
  spaceship_thrust__rot = evt.rot
  spaceship__rotateView_rad = evt.rotateView_rad
  spaceship__rotateView_base_quat = evt.rotateView_base_quat
  spaceship__min_target_ht = evt.min_target_ht
  spaceship__free_look_around = evt.free_look_around
  spaceship__use_stabilization = evt.use_stabilization
  spaceship__use_fwd_stabilization = evt.use_fwd_stabilization

def push_by_other_force_fields(var obj_net_phys : PhysObjActor&; obj_eid : EntityId;
                               obj_transform : float3x4; obj_center_of_mass : float3; obj_bound_rad : float)
  query() <| $ [es] (eid : EntityId; transform : float3x4; force_shield : float4; var force_shield_applied : float?)
    if eid != obj_eid
      let dist_sq = length_sq(obj_transform[3] - transform[3])
      let field_outer_rad = force_shield.x
      let field_width = force_shield.z
      let field_min_force = force_shield.y
      let field_max_force = force_shield.w
      // find other players with force shields which our obj entered into
      if dist_sq <= square(field_outer_rad + obj_bound_rad)
        let vec_len = sqrt(dist_sq)
        let dist = vec_len - obj_bound_rad

        // compute push force of other's force field (our local coordinates)
        let force_abs = min(MAX_LIN_FORCE,
          field_width > 0.f ? lerp(field_min_force, field_max_force, min((field_outer_rad - dist) / field_width, 1.f)) : field_max_force)
        if force_shield_applied != null
          *force_shield_applied += force_abs

        // apply push force to our center of mass
        let force_vec = rotate(inverse(obj_transform), (force_abs / vec_len) * (obj_transform[3] - transform[3]))
        phys_obj_addForce(obj_net_phys.phys, obj_center_of_mass, force_vec)


[es(before=spaceship_thrust_es_event_handler, REQUIRE=force_shield)]
def force_field_clear_applied_force_es_event_handler(evt : CmdPostPhysUpdate; var force_shield_applied : float&)
  force_shield_applied = 0.0f

[es]
def spaceship_thrust_es_event_handler(evt : CmdPostPhysUpdate; var phys_obj_net_phys : PhysObjActor&;
                                      eid : EntityId;
                                      spaceship_thrust__lin : float3;
                                      spaceship_thrust__rot : float3;
                                      transform : float3x4;
                                      spaceship__free_look_around : bool;
                                      spaceship__rotateView_rad : float2;
                                      spaceship__rotateView_base_quat : float4;
                                      spaceship__use_stabilization : bool;
                                      spaceship__use_fwd_stabilization : bool;
                                      var spaceship__min_target_ht : float&;
                                      spaceship__min_target_dHt : float;
                                      spaceship_nozzle_pos__mainA : float3;
                                      spaceship_nozzle_pos__mainD : float3;
                                      spaceship_nozzle_pos__agileL : float3;
                                      spaceship_nozzle_pos__agileR : float3;
                                      spaceship_nozzle_pos__agileU : float3;
                                      spaceship_nozzle_pos__agileD : float3;
                                      spaceship__center_of_mass__bound_rad : float4;
                                      var csu_engine_mainA__thrust : float?;
                                      var csu_engine_mainD__thrust : float?;
                                      var csu_engine_agileU__thrust : float3?;
                                      var csu_engine_agileD__thrust : float3?;
                                      var csu_engine_agileL__thrust : float3?;
                                      var csu_engine_agileR__thrust : float3?)

  var thrust_lin, thrust_lin_world : float3
  thrust_lin = spaceship_thrust__lin
  thrust_lin.y *= 10.f
  thrust_lin_world = float3(0, 0, 0)

  var rotate_heading, rotate_pitch, rotate_roll : float
  rotate_heading = -spaceship_thrust__rot.y
  rotate_pitch = -spaceship_thrust__rot.z
  rotate_roll = spaceship_thrust__rot.x

  let local_wvel = phys_obj_net_phys.phys.currentState.omega
  if spaceship__use_fwd_stabilization
    let local_vel = rotate(inverse(transform), phys_obj_net_phys.phys.currentState.velocity)
    thrust_lin -= clamp(float3(0.f, spaceship__use_stabilization ? 0.f : local_vel.y, local_vel.z) * 5.f / evt.dt,
      float3(-MAX_LIN_FORCE, -MAX_LIN_FORCE, -MAX_LIN_FORCE), float3(MAX_LIN_FORCE, MAX_LIN_FORCE, MAX_LIN_FORCE))

  if spaceship__use_stabilization
    spaceship__min_target_ht = clamp(spaceship__min_target_ht + spaceship__min_target_dHt * evt.dt, 3.5f, 50.f)
    // stabilize in hover mode and too fast descent
    let posXZ = transform[3].xz
    var land_altitude = transform[3].y - traceht_lmesh(posXZ)
    var water_altitude : float = -1.f
    if traceht_water(transform[3], water_altitude)
      water_altitude = transform[3].y - water_altitude
      if land_altitude > water_altitude
        land_altitude = water_altitude

    let trace_above_ht = 3.0f
    var stp : float3 = transform[3] + float3(0.f, trace_above_ht, 0.f)
    var dir : float3 = float3(0.f, -1.f, 0.f)
    var ray_dist : float = trace_above_ht + spaceship__min_target_ht + 5.f
    var norm_ : float3
    if traceray_normalized(stp, dir, ray_dist, norm_)
      if land_altitude > ray_dist - trace_above_ht
        land_altitude = ray_dist - trace_above_ht

    let phys_vel = phys_obj_net_phys.phys.currentState.velocity
    if (land_altitude < spaceship__min_target_ht || phys_vel.y < -10.f) && abs(transform[1].y) > 0.5f && length_sq(local_wvel) < 1e2f
      thrust_lin_world.y += clamp((land_altitude < spaceship__min_target_ht ? (spaceship__min_target_ht - land_altitude - 6.f * phys_vel.y * evt.dt) : -9.f * phys_vel.y * evt.dt) * 600.0f, -MAX_LIN_FORCE, MAX_LIN_FORCE)

    // stabilize orientation
    if (length_sq(transform[1].xz) > 1e-6f || length_sq(local_wvel) > 1e-6f) || !spaceship__free_look_around
      var heading, pitch, roll : float;
      matrix_to_euler(transform, heading, pitch, roll)

      if !spaceship__free_look_around
        // compute euler angles in world frame for spaceship__rotateView_rad
        var q : quat;
        euler_heading_attitude_to_quat(spaceship__rotateView_rad.x, spaceship__rotateView_rad.y, q)
        q = quat(quat_mul(spaceship__rotateView_base_quat, float4(q)))
        var heading_to, pitch_to, roll_to : float;
        quat_to_euler(q, heading_to, pitch_to, roll_to)

        // orient ship to spaceship__rotateView_rad direction
        heading = norm_s_ang(heading - heading_to)
        pitch = norm_s_ang(pitch - pitch_to)
      else
        heading = 0.0f

      let stab_roll = -(roll + 20.f * local_wvel.x * evt.dt) * 20.f
      let stab_pitch = -(pitch + 20.f * local_wvel.z * evt.dt) * 30.f
      let stab_heading = -(heading + 20.f * local_wvel.y * evt.dt) * 30.f
      rotate_roll += stab_roll
      rotate_pitch += stab_pitch
      rotate_heading += stab_heading
      //print("localW={local_wvel} pitch={pitch} roll={roll} stab_pitch={stab_pitch} stab_roll={stab_roll}")

  elif length_sq(local_wvel) > 1e-6f || !spaceship__free_look_around
    var d_pitch : float = 0.0f;
    var d_heading : float = 0.0f;
    if !spaceship__free_look_around
      // orient ship to spaceship__rotateView_rad direction
      d_pitch = -spaceship__rotateView_rad.y
      d_heading = -spaceship__rotateView_rad.x

    let stab_roll = -(10.f * local_wvel.x * evt.dt) * 40.f
    let stab_pitch = -(d_pitch + 20.f * local_wvel.z * evt.dt) * 100.f
    let stab_heading = -(d_heading + 20.f * local_wvel.y * evt.dt) * 100.f
    rotate_roll += stab_roll
    rotate_pitch += stab_pitch
    rotate_heading += stab_heading


  if length_sq(thrust_lin_world) > 1e-6
    thrust_lin += rotate(inverse(transform), thrust_lin_world)
  thrust_lin = clamp(thrust_lin, float3(-MAX_LIN_FORCE, -MAX_LIN_FORCE, -MAX_LIN_FORCE), float3(MAX_LIN_FORCE, MAX_LIN_FORCE, MAX_LIN_FORCE))

  //print("dynamics: lin={thrust_lin} rotate_heading={rotate_heading} rotate_pitch={rotate_pitch} rotate_roll={rotate_roll} tm={transform} alt={land_altitude} thrust_lin_world={thrust_lin_world}")
  if thrust_lin.x > 0.f
    let force = float3(thrust_lin.x, 0.0f, 0.0f)
    phys_obj_addForce(phys_obj_net_phys.phys, spaceship_nozzle_pos__mainA, force)
  elif thrust_lin.x < 0.f
    let force = float3(thrust_lin.x, 0.0f, 0.0f)
    phys_obj_addForce(phys_obj_net_phys.phys, spaceship_nozzle_pos__mainD, force)

  if csu_engine_mainA__thrust != null
    *csu_engine_mainA__thrust = thrust_lin.x > 0.f ? thrust_lin.x : 0.f
  if csu_engine_mainD__thrust != null
    *csu_engine_mainD__thrust = thrust_lin.x < 0.f ? thrust_lin.x : 0.f

  var thrust_agileU : float3 = float3(0.f, 0.f, 0.f)
  var thrust_agileD : float3 = float3(0.f, 0.f, 0.f)
  var thrust_agileL : float3 = float3(0.f, 0.f, 0.f)
  var thrust_agileR : float3 = float3(0.f, 0.f, 0.f)

  if thrust_lin.y > 0.f
    thrust_agileD += float3(0.0f, thrust_lin.y, 0.0f)
  elif thrust_lin.y < 0.f
    thrust_agileU += float3(0.0f, thrust_lin.y, 0.0f)

  if thrust_lin.z > 0.f
    thrust_agileL += float3(0.0f, 0.0f, thrust_lin.z)
  elif thrust_lin.z < 0.f
    thrust_agileR += float3(0.0f, 0.0f, thrust_lin.z)

  rotate_roll = clamp(rotate_roll, -MAX_ROT_FORCE, MAX_ROT_FORCE)
  if rotate_roll > 0.f
    let force = float3(0.0f, rotate_roll, 0.0f)
    thrust_agileL += force
    thrust_agileR -= force
  elif rotate_roll < 0.f
    let force = float3(0.0f, -rotate_roll, 0.0f)
    thrust_agileL -= force
    thrust_agileR += force

  rotate_heading = clamp(rotate_heading, -MAX_ROT_FORCE, MAX_ROT_FORCE)
  if rotate_heading > 0.f
    let force = float3(-rotate_heading, 0.0f, 0.0f)
    thrust_agileL += force
    thrust_agileR -= force
  elif rotate_heading < 0.f
    let force = float3(rotate_heading, 0.0f, 0.0f)
    thrust_agileL -= force
    thrust_agileR += force

  rotate_pitch = clamp(rotate_pitch, -MAX_ROT_FORCE, MAX_ROT_FORCE)
  if rotate_pitch > 0.f
    let force = float3(-rotate_pitch, 0.0f, 0.0f)
    thrust_agileU += force
    thrust_agileD -= force
  elif rotate_pitch < 0.f
    let force = float3(rotate_pitch, 0.0f, 0.0f)
    thrust_agileU -= force
    thrust_agileD += force

  phys_obj_addForce(phys_obj_net_phys.phys, spaceship_nozzle_pos__agileU, thrust_agileU)
  phys_obj_addForce(phys_obj_net_phys.phys, spaceship_nozzle_pos__agileD, thrust_agileD)
  phys_obj_addForce(phys_obj_net_phys.phys, spaceship_nozzle_pos__agileL, thrust_agileL)
  phys_obj_addForce(phys_obj_net_phys.phys, spaceship_nozzle_pos__agileR, thrust_agileR)
  if csu_engine_agileU__thrust != null
    *csu_engine_agileU__thrust = thrust_agileU
  if csu_engine_agileD__thrust != null
    *csu_engine_agileD__thrust = thrust_agileD
  if csu_engine_agileL__thrust != null
    *csu_engine_agileL__thrust = thrust_agileL
  if csu_engine_agileR__thrust != null
    *csu_engine_agileR__thrust = thrust_agileR

  push_by_other_force_fields(phys_obj_net_phys, eid, transform,
    spaceship__center_of_mass__bound_rad.xyz, spaceship__center_of_mass__bound_rad.w)
