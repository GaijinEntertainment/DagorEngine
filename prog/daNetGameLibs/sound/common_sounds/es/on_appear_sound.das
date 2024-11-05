require ecs
require soundEvent


[es(tag=sound, on_appear)]
def on_appear_sound(evt : Event;
                    on_appear_sound__path : string;
                    transform : float3x4)
  oneshot(on_appear_sound__path, transform[3])
