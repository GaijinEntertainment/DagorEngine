require ecs
require soundEvent


[es(tag=sound, on_disappear)]
def on_disappear_sound(evt : Event;
                       on_disappear_sound__path : string;
                       transform : float3x4)
  oneshot(on_disappear_sound__path, transform[3])
