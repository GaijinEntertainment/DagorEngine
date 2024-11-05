module net_events shared

require ecs

[cpp_event(broadcast)]
struct EventOnConnectedToServer {}

[event(broadcast)]
struct MatchingRoomExtraParams
  routeEvaluationChance : float
  ddosSimulationChance : float
  ddosSimulationAddRtt : int

[cpp_event(broadcast)]
struct NetEchoReponse
  routeId : uint
  result : int
  rttOrTimeout : uint

[cpp_event(broadcast)]
struct ChangeServerRoute
  currentIsUnresponsive : bool
