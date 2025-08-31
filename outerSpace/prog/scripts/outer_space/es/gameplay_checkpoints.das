require app
require ecs
require math.base
require outer_space.events_outer_space
require DagorMath
require DngPhysObj
require DngNet
require GameTimers

def get_total_checkpoints()
  var total_checkpoints = 0
  query() <| $ [es(REQUIRE=check_pt)] ()
    total_checkpoints += 1
  return total_checkpoints

[es(tag=server, after=spaceship_thrust_es_event_handler)]
def spaceship_visit_checkpoints_es(evt : CmdPostPhysUpdate; phys_obj_net_phys : PhysObjActor; possessedByPlr : EntityId)
  let ship_pos = float3(phys_obj_net_phys.phys.currentState.location.P)
  let total_checkpoints = get_total_checkpoints()
  var player_winner_eid = INVALID_ENTITY_ID
  query() <| $ [es(REQUIRE=check_pt)] (eid : EntityId;
                                       transform : float3x4;
                                       var checkpoint__visited_by_player : EidList&;
                                       checkpoint__score : float)
    if length_sq(transform[3] - ship_pos) < square(7.f)
      for v in checkpoint__visited_by_player
        if v == possessedByPlr
          return
      checkpoint__visited_by_player |> push(possessedByPlr)
      query(possessedByPlr) <| $ [es] (var player__score : float&; var player__last_checkpoint_time : float&; var player__checkpoints : int&)
        player__checkpoints += 1
        player__score += checkpoint__score
        player__last_checkpoint_time = get_sync_time()
        if total_checkpoints == player__checkpoints
          player_winner_eid = possessedByPlr
        //print("score {player__score} at {player__last_checkpoint_time}")
      net::send_net_event(eid, EventOnCheckpointVisited(byPlayer = possessedByPlr))
  if player_winner_eid != INVALID_ENTITY_ID
    using() <| $(var scores : Object)
      query() <| $ [es(REQUIRE=player)] (eid : EntityId;
                                         name : string;
                                         player__checkpoints : int;
                                         player__last_checkpoint_time : float;
                                         player__score : float)
        using() <| $(var plinfo : Object)
          set(plinfo, "name", name)
          set(plinfo, "checkpoints", player__checkpoints)
          set(plinfo, "player_score", player__score)
          set(plinfo, "player_last_checkpoint_time", player__last_checkpoint_time)
          set(scores, "{eid}", plinfo)
      net::broadcast_net_event(EventSessionFinishedOnWin(byPlayer = player_winner_eid, scores = ecs_addr(scores)))
    broadcastEvent(EventSetExitTimerOnWin(exit_in_sec = 15.0))


[es(tag=server, tag=net)]
def on_checkpoint_visited(evt : EventSetExitTimerOnWin; var server_player_win_timer__handle : uint64&)
  if server_player_win_timer__handle == uint64(0)
    let endSessionCb <- @ <| ()
      exit_game("session finished by player win")
    server_player_win_timer__handle = game_timer_set(endSessionCb, evt.exit_in_sec)

[es(tag=render, tag=gameClient, on_appear)]
def add_visual_fx_for_checkpoint(evt : Event; eid : EntityId; checkpoint__visited_by_player : EidList)
  var not_visited_by_local_player : bool = true
  // add visible FX for checkpoint unless if it was visited by our local player already
  query() <| $ [es(REQUIRE=player)] (eid : EntityId; is_local : bool)
    if !is_local
      return
    for v in checkpoint__visited_by_player
      if v == eid
        not_visited_by_local_player = false

  if not_visited_by_local_player
    addSubTemplate(eid, "checkpoint_visible_fx")

[es(tag=render, tag=gameClient)]
def mark_checkpoint_visited_es(evt : EventOnCheckpointVisited; eid : EntityId)
  let cp_eid = eid
  // remove visible FX for checkpoint if it was visited by our local player
  query() <| $ [es(REQUIRE=player)] (eid : EntityId; is_local : bool)
    if is_local && eid == evt.byPlayer
      removeSubTemplate(cp_eid, "checkpoint_visible_fx")
