require ecs
require soundEvent


[es(tag=sound, track=door_operations__curState)]
def door_sound(evt : Event;
               door_operations__curState : bool;
               door_sound__openPath : string;
               door_sound__closePath : string;
               transform : float3x4)
  oneshot(door_operations__curState ? door_sound__openPath : door_sound__closePath, transform[3])
