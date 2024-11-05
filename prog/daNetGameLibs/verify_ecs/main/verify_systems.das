options no_global_variables = false
require ecs
require DagorSystem
require %danetlibs.verify_ecs.main.verify_events


var
  levelWasLoaded = false
  systemsWasVerified = false
  templatesWasVerified = false


[es(tag=(dev, gameClient), on_event=EventOnGameAppStarted)]
def client_broadcast_systems_validation(evt : Event)
  broadcastEvent([[VerifyEcsSystems]])


[es(tag=(dev, netClient), on_event=EventOnConnectedToServer)]
def net_client_broadcast_systems_validation(evt : Event)
  broadcastEvent([[VerifyEcsSystems]])

[es(tag=(dev, server), on_event=EventLevelLoaded)]
def server_broadcast_systems_validation(evt : Event)
  broadcastEvent([[VerifyEcsSystems]])

[es(tag=(dev, playingReplay), on_event=EventLevelLoaded)]
def replay_broadcast_systems_validation(evt : Event)
  broadcastEvent([[VerifyEcsSystems]])


[es(tag=dev, on_event=EventLevelLoaded)]
def broadcast_templates_validation(evt : Event)
  levelWasLoaded = true
  broadcastEvent([[VerifyEcsTemplates]])


[es(tag=dev)]
def check_systems_validation(evt : VerifyEcsSystems)
  systemsWasVerified = true


[es(tag=dev)]
def check_templates_validation(evt : VerifyEcsTemplates)
  templatesWasVerified = true


[es(tag=dev, on_event=EventEntityManagerBeforeClear)]
def post_verify_validation(evt : Event)
  // does we need support it in replay mode?
  // Yes: We can validate replay only by version, but also by templates
  if levelWasLoaded // ignore loading scene (without level)
    if systemsWasVerified && templatesWasVerified
      print("ecs_verify: validation succeed")
    if !systemsWasVerified
      logerr("ecs_verify: event VerifyEcsSystems wasn't sent")
    if !templatesWasVerified
      logerr("ecs_verify: event VerifyEcsTemplates wasn't sent")
  levelWasLoaded = false
  systemsWasVerified = false
  templatesWasVerified = false
