module events_outer_space shared
require ecs
require DngNet
require danetlibs.native_dasevents.main.native_events public

[cpp_event(unicast)]
struct EventItemAttached
  attach_to : EntityId

[cpp_event(unicast)]
struct EventItemDetached
  detach_from : EntityId

[event(unicast)]
struct EventOnPlayerConnected
  wishTeam : int
  reconnected : bool
  canSpawnEntity : bool

[event(unicast)]
struct CmdSpawnEntityForPlayer
  team : int
  possessed : ecs::EntityId

[cpp_event(broadcast)]
struct UpdateStageUpdateInput
  curTime : float
  dt : float

[cpp_event(unicast)]
struct CmdPostPhysUpdate
  tick : int
  dt : float
  isForReal : bool

[cpp_event(broadcast)]
struct CmdAddInitialEntitiesInNetScope
  connid : int

[cpp_event(broadcast)]
struct CmdAddDefaultEntitiesInNetScope
  connid : int
  viewPos : float3
  viewFwd : float3

[cpp_event(broadcast)]
struct EventOnClientConnected
  connid : int
  matching : uint64
  userName : das_string
  groupId : int64
  clientFlags : uint16
  platformUserId : das_string
  platform : das_string
  matchingTeam : int
  appId : int

[cpp_event(broadcast)]
struct EventOnClientDisconnected
  connid : int
  cause : DisconnectionCause

[event(unicast)]
struct CmdSetCameraTarget
  target : EntityId

[event(unicast, routing=ROUTING_CLIENT_CONTROLLED_ENTITY_TO_SERVER)]
struct RequestCamSwitch
  isTpsView : bool

[event(unicast, routing=ROUTING_CLIENT_TO_SERVER)]
struct PassControlInputs
  lin : float3
  rot : float3
  rotateView_rad : float2
  rotateView_base_quat : float4
  min_target_ht : float
  free_look_around : bool
  use_stabilization : bool
  use_fwd_stabilization : bool

[event(broadcast)]
struct PlayerStartQuitGameTimer {}

[event(broadcast)]
struct EventStartCountDown {}

[event(broadcast)]
struct EventSetMissionTimer
  mission_end_time_sec : float

[event(broadcast)]
struct EventSetExitTimerOnWin
  exit_in_sec : float

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT)]
struct EventSessionFinishedOnTimeout {}

[event(broadcast, routing=ROUTING_SERVER_TO_CLIENT)]
struct EventSessionFinishedOnWin
  byPlayer : EntityId
  scores : ecs::Object const?

[event(unicast, routing=ROUTING_SERVER_TO_CLIENT)]
struct EventOnCheckpointVisited
  byPlayer : EntityId

[event(broadcast)]
struct EventSetMusic
  music : string

[cpp_event(broadcast)]
struct EventOnSoundPresetLoaded
  hash : uint
  isLoaded : bool

[event(unicast)]
struct CmdPlaySound
  hash : uint
  pos : float3
  param : float

[event(unicast)]
struct CmdPlaySoundSimple
  hash : uint

[cpp_event(unicast)]
struct EventOnCollision
  vel : float3
  pvel : float3
  pos : float3
  offender : EntityId
  cvel : float3
  dt : float
  damage : float
