require ecs
require soundSystem

[es(tag=sound, on_appear, track=sound_override_time_speed)]
def sound_override_time_speed_appear(evt : Event; sound_override_time_speed : float)
  sound_override_time_speed(sound_override_time_speed)

[es(tag=sound, on_disappear, REQUIRE=sound_override_time_speed)]
def sound_override_time_speed_disappear(evt : Event)
  sound_override_time_speed(0.)
