require ecs
require DagorDataBlock
require DagorRandom
require DagorSystem
require app
require outer_space.events_outer_space
require DngPhysObj
require outer_space.es.space_ship_common
require math

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
        broadcastEvent(EventOnBotVisitedCheckpoint(byBot = shEid, checkpoint = eid))

  var ct = get_phys_obj_produced_controls(phys_obj_net_phys)
  if ct != null
    let dL = rotate(inverse(transform), phys_obj_net_phys.phys.currentState.velocity)
    let dW = normalize(target - pos)
    let heading = atan2(dW.z, dW.x)
    let pitch = atan2(-dW.y, length(dW.xz))

    (*ct).use_fwd_stabilization = true
    (*ct).use_stabilization = true
    (*ct).free_look_around = !can_move
    (*ct).rotateView_base_quat = float4(0, 0, 0, 1)
    (*ct).rotateView_rad = float2(-heading, -pitch)
    (*ct).rot = float3(0, 0, 0)
    if !can_move
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