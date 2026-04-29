require ecs
require soundEvent


[es(tag=sound, on_appear)]
def limit_max_instances_under_sound_control(evt : Event;
                                            limit_max_instances_under_sound_control__maxInstances : int;
                                            limit_max_instances_under_sound_control__path : string;
                                            var sound_control__allowSound : bool&)

  let curPath = limit_max_instances_under_sound_control__path
  var numInstances = 0
  query() <| $ [es] (limit_max_instances_under_sound_control__path : string; sound_control__allowSound : bool)
    if sound_control__allowSound && curPath == limit_max_instances_under_sound_control__path
      ++numInstances
  sound_control__allowSound = numInstances <= limit_max_instances_under_sound_control__maxInstances
