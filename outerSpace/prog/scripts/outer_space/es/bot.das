require ecs
require DagorDataBlock
require DagorRandom
require DagorSystem
require app
require outer_space.events_outer_space
require DngPhysObj
require outer_space.es.space_ship_common
require math
require math.base
require DagorMath

enum TargetChoosing
  CLOSEST = 0
  RANDOM = 1

enum ThrustPatterns
  AGGRESSIVE = 0
  SLOW = 1
  STATIC = 2

// var bots_spawned = false

def get_bots_count() : int
  return dgs_get_settings() |> datablock_get_block_by_name("bots") |> datablock_getInt("botsCount", 0)

def get_targeting_weights() : float2
  let weights : float2 = dgs_get_settings() |> datablock_get_block_by_name("bots") |> datablock_getPoint2("targetingWeights", float2(0.5, 0.5))
  print("targeting weights {weights}")
  return weights / (weights[0] + weights[1])

def get_thrusting_weights() : float2
  let weights : float2 = dgs_get_settings() |> datablock_get_block_by_name("bots") |> datablock_getPoint2("thrustingWeights", float2(0.5, 0.5))
  print("thrusting weights {weights}")
  return weights / (weights[0] + weights[1])

def choose_random_from_weights(weights : float2) : int
  let rnd = float(grnd() % 1000) / 1000.0
  if rnd < weights[0]
    return 0
  return 1

[es(tag=server)]
def spawn_bots_on_first_countdown(evt : EventStartCountDown; var server_bot_spawned : bool&)
  if server_bot_spawned
    return

  var spawnpoint_count = 0
  query() <| $ [es(REQUIRE=respbase)] ()
    spawnpoint_count += 1

  if spawnpoint_count < get_bots_count() + 1 // all the bots and at least a single player
    return

  for i in range(get_bots_count())
    createEntity("ai_bot") <| $(var init)
      init |> set("name", "Bot_{i}")

  server_bot_spawned = true

def bot_choose_next_point(bot_eid : EntityId; pos : float3, targeting : int)
  var target_eid = INVALID_ENTITY_ID

  var checkpoints : array<EntityId>
  query() <| $ [es(REQUIRE=check_pt)] (eid : EntityId; checkpoint__visited_by_player : EidList)
    for b_eid in checkpoint__visited_by_player
      if b_eid == bot_eid
        return

    checkpoints |> push(eid)

  if length(checkpoints) > 0
    if targeting == 0 // closest
      var min_dist = 999999.0

      for cpt_eid in checkpoints
        query(cpt_eid) <| $ [es(REQUIRE=check_pt)] (eid : EntityId; transform : float3x4)
          let distance = length(transform[3] - pos)
          if min_dist > distance
            min_dist = distance
            target_eid = eid
    elif targeting == 1 // random
      target_eid = checkpoints[grnd() % length(checkpoints)]
  return target_eid

[es(tag=not_inside_tools, tag=server, on_appear, REQUIRE=is_bot)]
def on_bot_created_server(evt : Event; eid : EntityId)
  var spawnpoints : array<EntityId>
  var obstacles : array<float4> // other players/dynamic obstacles: .xyz=pos, .w=rad
  query() <| $ [es] (transform : float3x4; force_shield : float4)
    // print("bot pos={transform[3]} rad={force_shield[0]}")
    obstacles |> push(float4(transform[3], force_shield.x))

  query() <| $ [es(REQUIRE=respbase)] (eid : EntityId)
    spawnpoints |> push(eid)

  var spawnTm : float3x4
  assert(length(spawnpoints) > 0, "No spawn bases to spawn onto")
  var idx : int = grnd() % length(spawnpoints)
  let stop_idx = (idx + length(spawnpoints) - 1) % length(spawnpoints)
  var good_idx : bool = false
  while true
    good_idx = true
    query(spawnpoints[idx]) <| $ [es] (transform : float3x4; var checkpoint__occupiedBy : EntityId&)
      if checkpoint__occupiedBy != INVALID_ENTITY_ID
        good_idx = false
        return
      for o in obstacles // find spawn position not occupied by other players/obstacles
        if length(transform[3] - o.xyz) < 2.0f * o.w
          good_idx = false
          return

      //print("idx={idx}  {good_idx} stop_idx={stop_idx}");
      spawnTm = transform
      checkpoint__occupiedBy = eid
    if good_idx || idx == stop_idx
      break
    idx = (idx + 1) % length(spawnpoints)
  assert(good_idx, "No vacant spawn bases to spawn onto")

  let targeting = choose_random_from_weights(get_targeting_weights())
  let thrusting = choose_random_from_weights(get_thrusting_weights())

  var checkpoints <- array<EntityId>()
  query() <| $ [es(REQUIRE=check_pt)] (eid : EntityId)
    checkpoints |> push(eid)
  let checkpointIdx = grnd() % length(checkpoints)

  var has_gravity : bool = false
  query() <| $ [es] (phys_props__gravity : float)
    has_gravity = abs(phys_props__gravity) > 0.0f

  createEntity("ai_bot_combat_space_unit") <| $(var init)
    init |> set("transform", spawnTm)
    init |> set("possessedByPlr", eid)
    init |> set("bot__current_target_checkpoint", bot_choose_next_point(eid, spawnTm[3], targeting))
    init |> set("targeting", targeting)
    init |> set("thrusting", thrusting)

[es(REQUIRE=ai_possessed)]
def spaceship_ai_controls(evt : CmdPrePhysUpdate;
                          eid : EntityId;
                          var phys_obj_net_phys : PhysObjActor&;
                          transform : float3x4,
                          possessedByPlr : EntityId;
                          bot__current_target_checkpoint : EntityId,
                          thrusting : int,
                          var can_move : bool&)
  if !can_move
    query() <| $[es] (time_to_start_session_msec : int)
      can_move = float(time_to_start_session_msec) < get_sync_time() * 1000.0

  let shEid = eid
  let pos = transform[3]
  var target = float3(0, 0, 0)

  query(bot__current_target_checkpoint) <| $ [es(REQUIRE=check_pt)] (eid : EntityId; checkpoint__visited_by_player : EidList; transform : float3x4)
    target = transform[3]
    for b_eid in checkpoint__visited_by_player
      if b_eid == possessedByPlr
        broadcastEventImmediate(EventOnBotVisitedCheckpoint(byBot = shEid, checkpoint = eid))

  var ct = get_phys_obj_produced_controls(phys_obj_net_phys)
  if ct != null
    let canThrust = can_move

    let dL = rotate(inverse(transform), phys_obj_net_phys.phys.currentState.velocity)
    let toTarget = target - pos
    let dW = normalize(toTarget)

    let cur_tm_q = math::quat(transform)
    let cur_tm_quat = quat(cur_tm_q)
    var curHeading, curPitch, curRoll : float
    quat_to_euler(cur_tm_quat, curHeading, curPitch, curRoll)

    // bearing to target is only meaningful with real horizontal separation - flying almost
    // directly over/under a checkpoint makes the required heading sweep through the full
    // circle in a fraction of a second (correct, but not something the ship can track), so
    // hold the current heading instead of chasing an unfollowable target while this close
    var heading = curHeading
    if length_sq(toTarget.xz) > 25.f // more than 5m of horizontal separation
      heading = atan2(dW.z, dW.x)
    var pitch = atan2(-dW.y, length(dW.xz))
    // never request a fully vertical attitude: the dynamics' heading/pitch/roll
    // decomposition of the ship's own orientation (matrix_to_euler) gimbal-locks at
    // pitch=+-90deg, which is exactly what a checkpoint straight above/below causes
    pitch = clamp(pitch, -80.f * PI / 180.f, 80.f * PI / 180.f)

    // if the ship is already spinning fast, chasing a new target heading on top of that
    // just fights the existing angular momentum - let go of the heading lock (free_look)
    // so the plain self-leveling/damping in the dynamics can arrest the spin first, then
    // resume aiming at the target once it settles
    let isSpinningFast = length_sq(phys_obj_net_phys.phys.currentState.omega) > 4.f // ~2 rad/s
    // fwd stabilization cancels local Y/Z velocity with a dt-scaled gain - strong relative
    // to a small actual velocity, so only engage it once actually moving, not while slow/hovering
    let isMovingFast = length_sq(phys_obj_net_phys.phys.currentState.velocity) > 25.f // 5 m/s

    // don't accelerate while still far from facing the target - cruise thrust at a large
    // attitude error just launches the ship past the target before it can finish turning,
    // producing a divergent overshoot-and-reverse oscillation instead of converging on it
    let fwdWorld = rotate(transform, float3(1, 0, 0))
    let alignment = dot(fwdWorld, dW)
    let isAligned = alignment > 0.7f // roughly within 45deg of facing the target

    // use_stabilization's altitude control is a ground-hover spring-damper relative to
    // terrain trace - only meaningful on a gravity mission (matches spawn_player_entity's
    // has_gravity check in player.das). It also disables the forward stabilization's
    // vertical velocity damping, so in zero-g open space (no ground to hover over) leave
    // it off and let fwd_stabilization damp all three axes instead
    var has_gravity : bool = false
    query() <| $ [es] (phys_props__gravity : float)
      has_gravity = abs(phys_props__gravity) > 0.0f

    (*ct).use_fwd_stabilization = isMovingFast
    (*ct).use_stabilization = has_gravity
    (*ct).free_look_around = !canThrust || isSpinningFast

    // target in ship-relative coordspace, same as the human input path (spaceship_input_es):
    // express the desired absolute attitude as an offset from the ship's current orientation
    // instead of from world identity - required by the use_stabilization=false attitude
    // controller, which uses rotateView_rad directly as the error term with no
    // reconstruction against current orientation of its own
    var q : quat
    euler_heading_attitude_to_quat(-heading, -pitch, q)
    q = quat(quat_mul(float4(inverse(quat(cur_tm_q))), float4(q)))
    var newRad : float2
    var roll_ : float
    quat_to_euler(q, newRad.x, newRad.y, roll_)
    (*ct).rotateView_rad = newRad
    (*ct).rotateView_base_quat = cur_tm_q

    (*ct).rot = float3(0, 0, 0)
    if !canThrust || !isAligned
      (*ct).lin = float3(0, 0, 0)
    elif thrusting == 0  // fast
      if dL.x < 80.0
        (*ct).lin = float3(100, 0, 0)
      elif dL.x > 100.0
        (*ct).lin = float3(-50, 0, 0)
      else
        (*ct).lin = float3(0, 0, 0)
    elif thrusting == 1 // slow
      if dL.x < 40.0
        (*ct).lin = float3(100, 0, 0)
      elif dL.x > 50.0
        (*ct).lin = float3(-50, 0, 0)
      else
        (*ct).lin = float3(0, 0, 0)

[es(tag=server)]
def bot_on_checkpoint_visit(evt : EventOnBotVisitedCheckpoint;
                            eid : EntityId;
                            transform : float3x4;
                            possessedByPlr : EntityId;
                            var bot__current_target_checkpoint : EntityId&;
                            targeting : int)
  if eid != evt.byBot
    return

  bot__current_target_checkpoint = bot_choose_next_point(possessedByPlr, transform[3], targeting)