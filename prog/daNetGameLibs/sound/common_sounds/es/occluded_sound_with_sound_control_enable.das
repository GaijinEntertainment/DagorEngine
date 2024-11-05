require ecs
require soundEvent

[es(tag=sound, track=sound_control__soundEnabled, REQUIRE=occludedSoundWithSoundControl)]
def occluded_sound_with_sound_control_enable(evt : Event;
                                             sound_control__soundEnabled : bool;
                                             var occluded_sound__enabled : bool&)
  occluded_sound__enabled = sound_control__soundEnabled
