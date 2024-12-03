require app
require ecs
require soundEvent
require outer_space.events_outer_space

[es(tag=sound, before=mark_checkpoint_visited_es)]
def on_checkpoint_visited_es(evt : EventOnCheckpointVisited; transform : float3x4)
  var handle = soundEvent::play("", "ui/checkpoint", transform[3])
  soundEvent::abandon(handle)
