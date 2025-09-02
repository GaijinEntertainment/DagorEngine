require ecs
require soundEvent
require danetlibs.light_flicker.render.light_flicker_common
require sound_utils.modules.sound_control_common


[es(tag=sound, on_appear, track=light_flicker__state)]
def light_flicker_sound(evt : Event;
                        light_flicker__state : int;
                        light_flicker__lightEids : EidList;
                        var light_flicker_sound__inited : bool&;
                        var light_flicker_sound__enabled : bool&)
  let enabled = light_flicker__state != int(LightFlickerState.Off)
  if light_flicker_sound__enabled != enabled
    light_flicker_sound__enabled = enabled
    for lightEid in light_flicker__lightEids
      query(lightEid) <| $ [es(REQUIRE_NOT=disableSoundControl)] (var sound_control__allowSound : bool&;
                                                                  light_flicker_updater_sound__onPath : string;
                                                                  light_flicker_updater_sound__offPath : string;
                                                                  transform : float3x4)
        sound_control__allowSound = enabled
        if light_flicker_sound__inited
          oneshot(enabled ? light_flicker_updater_sound__onPath : light_flicker_updater_sound__offPath, transform[3])
  light_flicker_sound__inited = true


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es)]
def light_flicker_sound_update(evt : Event;
                               light_flicker_sound__enabled : bool;
                               light_flicker__brightness : float;
                               light_flicker__lightEids : EidList)
  if light_flicker_sound__enabled
    for lightEid in light_flicker__lightEids
      query(lightEid) <| $ [es(REQUIRE_NOT=disableSoundControl)] (light_flicker_updater_sound__event : SoundEvent)
        if is_valid_handle_value(light_flicker_updater_sound__event)
          set_var(light_flicker_updater_sound__event, "brightness", light_flicker__brightness)


[es(tag=sound, track=sound_control__state)]
def light_flicker_updater_sound_track(evt : Event;
                                      sound_control__state : int;
                                      light_flicker_updater_sound__path : string;
                                      var light_flicker_updater_sound__event : SoundEvent&;
                                      transform : float3x4)
  abandon(light_flicker_updater_sound__event)
  if have_sound(sound_control__state)
    light_flicker_updater_sound__event |> play(light_flicker_updater_sound__path, transform[3])
