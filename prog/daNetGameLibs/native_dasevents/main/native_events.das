module native_events shared
require ecs
require RendInst


[event(unicast, with_scheme, routing=ROUTING_SERVER_TO_CLIENT)]
struct SetLocalPlayerMsg {}

[cpp_event(unicast, with_scheme)]
struct PossessTargetByPlayer
  target : EntityId

// this is like UpdateStageInfoAct, but is also sent in level loading as well
[cpp_event(broadcast, with_scheme)]
struct UpdateStageGameLogic
  dt : float
  curTime : float

[cpp_event(broadcast, with_scheme)]
struct EventGameSessionFinished
  disconnected : bool

[cpp_event(broadcast, with_scheme)]
struct EventGameSessionStarted {}

[cpp_event(broadcast, with_scheme)]
struct ChangeServerRoute
  currentIsUnresponsive : bool

[cpp_event(broadcast, with_scheme)]
struct ServerCreatePlayersEntities {}

[cpp_event(broadcast, with_scheme)]
struct EventAnyEntityResurrected
  eid : EntityId

[cpp_event(broadcast, with_scheme, cpp_gen=false)]
struct EventRendinstsLoaded {}

[cpp_event(unicast, with_scheme, cpp_gen=false)]
struct CmdDestroyRendinst
  userData : int
  destroyEntity : bool

[cpp_event(broadcast, with_scheme)]
struct EventTickrateChanged
  oldTickrate : int
  newTickrate : int

[cpp_event(broadcast, with_scheme)]
struct EventKeyFrameSaved
  time : int

[cpp_event(broadcast, with_scheme)]
struct RequestSaveKeyFrame {}

[cpp_event(unicast, with_scheme)]
struct CmdUpdateGridScale {}

[cpp_event(unicast, with_scheme)]
struct CmdShowHitcamera {}

[cpp_event(unicast, with_scheme)]
struct EventOnHitCameraControlEvent
  target : EntityId
  xrayEventType : int
  hitResult : int