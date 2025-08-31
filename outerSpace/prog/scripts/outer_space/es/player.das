require ecs
require net
require DagorDataBlock
require DagorRandom
require DagorSystem
require DngActor
require app
require player
require DngNet
require GameTimers
require outer_space.es.net_utils_common
require outer_space.events_outer_space
require DagorTime

def create_player(connid : int; name : das_string; uid : uint64; group_id : int64; platform_uid : das_string; platform : das_string;
                  app_id : int) : EntityId
  let playerTemplateName : string = dgs_get_settings() |> datablock_getStr("playerTemplateName", "common_player")
  let eid : EntityId = createEntitySync(playerTemplateName) <| $(init)
    init |> set("connid", connid)
    init |> set("name", name)
    init |> set("userid", uid)
    init |> set("possessed", INVALID_ENTITY_ID)
    init |> set("groupId", group_id)
    init |> set("platformUid", platform_uid)
    init |> set("platform", platform)
    init |> set("appId", app_id)
  return eid

def has_players() : bool
  return find_query() <| $ [es(REQUIRE=player)] ()
    return true

def get_session_duration()
  return dgs_get_settings() |> datablock_get_block_by_name("sessionLaunchParams") |> datablock_getReal("sessionMaxTimeLimit", 10.0) * 60.0

[es(tag=server, on_appear)]
def create_timer_to_exit_on_session_created(evt : Event; exit_on_no_players_connected_time_sec : float; var server_no_players_joined_timer__handle : uint64&)
  let hasPlayers = has_players()
  if !hasPlayers
    let timeoutCb <- @ <| ()
      exit_game("no players joined timer")
    game_timer_clear(server_no_players_joined_timer__handle)
    server_no_players_joined_timer__handle = game_timer_set(timeoutCb, exit_on_no_players_connected_time_sec)
    print("setting timer to exit on no players connected in {exit_on_no_players_connected_time_sec}sec = {server_no_players_joined_timer__handle}")

[es(tag=server)]
def on_client_connected_clear_exit_time_server(evt : EventOnClientConnected; var server_no_players_joined_timer__handle : uint64&)
  print("removing timer to exit on no players connected = {server_no_players_joined_timer__handle}")
  game_timer_clear(server_no_players_joined_timer__handle)

[es(tag=server)]
def on_client_connected_server(evt : EventOnClientConnected)
  let plrEid = create_player(evt.connid, evt.userName, evt.matching, evt.groupId, evt.platformUserId, evt.platform, evt.appId)
  var conn : net::IConnection? = get_client_connection(evt.connid)
  if conn != null && plrEid != INVALID_ENTITY_ID
    verify(*conn |> setEntityInScopeAlways(plrEid))
    send_net_event(plrEid, SetLocalPlayerMsg(), target_entity_conn(plrEid))
    broadcastEvent(EventStartCountDown())
    broadcastEvent(PlayerStartQuitGameTimer())

def has_connected_players() : bool
  return find_query() <| $ [es(REQUIRE=player)] (connid : int; is_local : bool)
    return connid != INVALID_CONNECTION_ID || is_local


[es(tag=server, on_appear)]
def set_session_duration_on_start(evt : Event; var session_duration_sec : float&)
  session_duration_sec = get_session_duration()

let def_countdown_time_to_start = 15 * 1000

[es(tag=server)]
def set_session_times(evt : EventStartCountDown; session_duration_sec : float; var countdown_duration_sec : float&;
                      var time_to_start_session_msec : int&; var time_to_end_session_msec : int&;
                      var time_to_start_countdown_msec : int&)
  if time_to_start_session_msec <= 0
    print("EventStartCountDown")
    var delay = def_countdown_time_to_start
    if (countdown_duration_sec >= 0.0)
      delay = int(countdown_duration_sec * 1000.0)
    else
      countdown_duration_sec = float(delay) / 1000.0
    time_to_start_session_msec = int(get_sync_time() * 1000.0) + delay
    time_to_start_countdown_msec = int(get_sync_time() * 1000.0)
  if session_duration_sec > 0.0 && time_to_end_session_msec < 0
    time_to_end_session_msec = time_to_start_session_msec + int(session_duration_sec * 1000.0)
  broadcastEvent(EventSetMissionTimer(mission_end_time_sec = float(time_to_end_session_msec) / 1000.0))

def set_input_enabled(enabled : bool)
  query() <| $ [es(REQUIRE=hero)] (var input__enabled : bool&)
    input__enabled = enabled

[es(tag=(gameClient))]
def update_countdown_time(info : UpdateStageInfoAct; time_to_start_session_msec : int; var countdown_to_start_sec : int&; countdown_duration_sec : float)
  if time_to_start_session_msec <= 0 || countdown_duration_sec <= 0.0
    return
  let secs_left = (time_to_start_session_msec - int(get_sync_time() * 1000.0)) / 1000
  if secs_left > 0
    set_input_enabled(false)
  if secs_left == 0
    set_input_enabled(true)
  if secs_left > -2
    countdown_to_start_sec = secs_left

let timeToFinishSessionSec = 15.0

[es(tag=(server, net))]
def set_session_quit_timer(evt : EventSetMissionTimer)
  let time_to_end_session = evt.mission_end_time_sec
  if time_to_end_session > 0.0
    let endSessionCb <- @ <| ()
      exit_game("session finished")
    game_timer_set(endSessionCb, time_to_end_session + timeToFinishSessionSec)

[es(tag=server)]
def end_session_on_time(info : UpdateStageInfoAct; time_to_end_session_msec : int)
  if time_to_end_session_msec <= int(get_sync_time() * 1000.0) && time_to_end_session_msec > 0
    broadcast_net_event(EventSessionFinishedOnTimeout())

[es(tag=server)]
def check_to_exit_on_no_players(evt : PlayerStartQuitGameTimer; timeout_on_all_players_left_sec : float; var server_no_players_timer__handle : uint64&)
  let hasConnectedPlayers = has_connected_players()
  print("hasConnectedPlayers {hasConnectedPlayers}")
  if !hasConnectedPlayers
    let timeoutCb <- @ <| ()
      exit_game("no players timer")
    game_timer_clear(server_no_players_timer__handle)
    server_no_players_timer__handle = game_timer_set(timeoutCb, timeout_on_all_players_left_sec)
  else
    game_timer_clear(server_no_players_timer__handle)

[es(tag=server)]
def on_client_disconnected_server(evt : EventOnClientDisconnected)
  print("player disconnected")
  find_query() <| $ [es] (eid : EntityId; connid : int)
    if connid == evt.connid
      query(eid) <| $ [es] (possessed : EntityId)
        print("request destroy possesed {possessed}")
        destroyEntity(possessed)
      print("request destroy player {eid}")
      destroyEntity(eid)
      return true
    return false
  broadcastEvent(PlayerStartQuitGameTimer())

def spawn_player_entity(possessed_by_player : EntityId)
  var spawnTms : array<float3x4>
  var obstacles : array<float4> // other players/dynamic obstacles: .xyz=pos, .w=rad
  var has_gravity : bool = false
  query() <| $ [es] (transform : float3x4; force_shield : float4)
    //print("player pos={transform[3]} rad={force_shield_rad}")
    obstacles |> push(float4(transform[3], force_shield.x))

  query() <| $ [es(REQUIRE=respbase)] (transform : float3x4)
    //print("spawn {transform[3]}")
    spawnTms |> push(transform)
  query() <| $ [es] (phys_props__gravity : float)
    has_gravity = abs(phys_props__gravity) > 0.0f

  var spawnTm : float3x4
  assert(length(spawnTms) > 0, "No spawn bases to spawn onto")
  var idx : int = grnd() % length(spawnTms)
  let stop_idx = (idx + length(spawnTms) - 1) % length(spawnTms)
  while idx != stop_idx
    var good_idx : bool = true
    for o in obstacles // find spawn position not occupied by other players/obstacles
      if length(spawnTms[idx][3] - o.xyz) < 2.0f * o.w
        good_idx = false
        break
    //print("idx={idx}  {good_idx} stop_idx={stop_idx}");
    if good_idx
      break
    idx = (idx + 1) % length(spawnTms)

  spawnTm = spawnTms[idx]

  createEntity("combat_space_unit") <| $(var init)
    init |> set("transform", spawnTm)
    init |> set("possessedByPlr", possessed_by_player)
    init |> set("spaceship__use_stabilization", has_gravity)

[es(tag=not_inside_tools, tag=server, on_appear, REQUIRE=player)]
def on_player_created_server(evt : Event; eid : EntityId; connid : int = -1; canBeLocal : bool = true)
  var conn : IConnection? = get_client_connection(connid)
  if conn != null
    *conn |> connection_setUserEid(eid)
    base_phys_actor_resizeSyncStates(connid)
    query(eid) <| $ [es] (var replication : NetObject&)
      replication |> net_object_setControlledBy(connid)
  else // local ?
    if canBeLocal && get_local_player_eid() == INVALID_ENTITY_ID
      set_local_player_eid(eid)
  spawn_player_entity(eid)
  broadcastEvent(EventStartCountDown())
  broadcastEvent(PlayerStartQuitGameTimer())

[es(tag=gameClient, on_appear, REQUIRE=player)]
def on_player_created_client(evt : Event; var replication : NetObject&)
  let sconn : IConnection const? = get_server_conn()
  if sconn != null
    replication |> net_object_setControlledBy(connection_getId(*sconn))

[es(on_disappear, REQUIRE=player)]
def on_player_destroyed(evt : Event; eid : EntityId; connid : int)
  if get_local_player_eid() == eid
    set_local_player_eid(INVALID_ENTITY_ID)
  var conn : IConnection? = get_client_connection(connid)
  if conn != null
    *conn |> connection_setUserEid(INVALID_ENTITY_ID)
  broadcastEvent(PlayerStartQuitGameTimer())

[es(on_event=SetLocalPlayerMsg, tag=gameClient, REQUIRE=player)]
def set_local_player(evt : Event; eid : EntityId)
  set_local_player_eid(eid)

[es(tag=(server), on_appear)]
def finalize_possesion_server(evt : Event; eid, possessedByPlr : EntityId; var replication : NetObject&)
  let possessedEid = eid
  query(possessedByPlr) <| $ [es] (connid : int; var possessed : EntityId&; var clientNetFlags : int&)
    print("player_possess_server {int(uint(possessedByPlr))}: conn=#{connid} {int(uint(possessedEid))}")

    possessed = possessedEid

    var clientConn : net::IConnection? = get_client_connection(connid)
    if clientConn != null
      replication |> net_object_setControlledBy(connid)
      clientNetFlags |= int(ClientNetFlags.CNF_REPLICATE_PHYS_ACTORS)
      var actor : BasePhysActor? = get_phys_actor(possessedEid)
      if actor != null
        *actor |> base_phys_actor_setRoleAndTickrateType(NetRole.ROLE_REMOTELY_CONTROLLED_AUTHORITY, PhysTickRateType.Normal)
