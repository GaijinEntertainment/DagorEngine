require ecs
require soundEvent
require soundSystem
require math.base
require strings


def sound_enable(enable : bool;
                 var sound_control__soundEnabled : bool&;
                 sound_control__subtemplate : string;
                 eid : EntityId)
  if sound_control__soundEnabled != enable
    sound_control__soundEnabled = enable
    if !empty(sound_control__subtemplate)
      if enable
        addSubTemplate(eid, sound_control__subtemplate)
      else
        removeSubTemplate(eid, sound_control__subtemplate)


// sound_control is used to enable/disable all sound for specific entity using its distance to listener
// sound_control__allowSound is for external control, may be modified by any other es


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=(animchar_before_render_es, sound_begin_update_es), before=sound_end_update_es, REQUIRE=msg_sink)]
def sound_control_update(evt : Event)

  let listener = get_listener_pos()

  query() <| $ [es(REQUIRE_NOT=disableSoundControl)] (eid : EntityId;
                                                      sound_banks_state__isPresetLoaded : bool;
                                                      sound_control__allowSound : bool;
                                                      sound_control__range : float;
                                                      sound_control__subtemplate : string;
                                                      var sound_control__inRange : bool&;
                                                      var sound_control__soundEnabled : bool&;
                                                      is_watched_sound : bool = false;
                                                      transform : float3x4)

    sound_control__inRange = is_watched_sound || (sound_control__inRange ? (distance_sq(listener, transform[3]) < square(sound_control__range)) :
                                                                           (distance_sq(listener, transform[3]) < square(sound_control__range * 0.9)))

    sound_enable(sound_control__inRange && sound_banks_state__isPresetLoaded && sound_control__allowSound,
                 sound_control__soundEnabled,
                 sound_control__subtemplate,
                 eid)


[es(tag=sound, on_appear, REQUIRE=disableSoundControl)]
def sound_control_disable(evt : Event; eid : EntityId;
                          sound_control__subtemplate : string;
                          var sound_control__soundEnabled : bool&)

  sound_enable(false,
               sound_control__soundEnabled,
               sound_control__subtemplate,
               eid)


[es(tag=sound, on_appear, track=sound_banks_state__isPresetLoaded)]
def sound_control_init_range(evt : Event;
                             sound_banks_state__isPresetLoaded : bool;
                             sound_control__initRange : string;
                             var sound_control__range : float&)

  if sound_banks_state__isPresetLoaded
    sound_control__range = get_max_distance(sound_control__initRange)
