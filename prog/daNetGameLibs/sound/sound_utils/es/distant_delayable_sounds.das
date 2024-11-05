require ecs
require soundSystem

[es(tag=sound, on_appear, REQUIRE=distantDelayableSounds)]
def distant_delayable_sounds_created(evt : Event)
  sound_enable_distant_delay(true)

[es(tag=sound, on_disappear, REQUIRE=distantDelayableSounds)]
def distant_delayable_sounds_destroyed(evt : Event)
  sound_enable_distant_delay(false)
  sound_release_delayed_events()
