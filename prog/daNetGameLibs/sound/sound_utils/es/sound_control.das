require ecs
require soundEvent
require soundSystem
require math.base
require strings
require sound_utils.modules.sound_control_common


def toggle_subtemplate(have_subtemplate : bool;
                       var sound_control__haveSubTemplate : bool&;
                       sound_control__subtemplate : string;
                       eid : EntityId)

  if sound_control__haveSubTemplate != have_subtemplate
    sound_control__haveSubTemplate = have_subtemplate
    if !empty(sound_control__subtemplate)
      if sound_control__haveSubTemplate
        addSubTemplate(eid, sound_control__subtemplate)
      else
        removeSubTemplate(eid, sound_control__subtemplate)


// sound_control__allowSound is for external control, may be modified by any other es
// not using track=(__isPresetLoaded, __allowSound, &c...), because HAVE to avoid double sound creation on_appear+on_track(sound_control__state)
// high chance of FMOD problems when start event instance, then immediately abandon it and then start again.

[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=(animchar_before_render_es, sound_begin_update_es), before=sound_end_update_es, REQUIRE=msg_sink)]
def sound_control_update(evt : Event)

  let listener = get_listener_pos()

  query() <| $ [es] (eid : EntityId;
                     sound_preset__isLoaded : bool;

                     sound_control__range : float;
                     sound_control__subtemplate : string;
                     var sound_control__state : int&;
                     var sound_control__haveSubTemplate : bool&;
                     sound_control__allowSound : bool;
                     is_watched_sound : bool = false;

                     transform : float3x4;
                     disableSoundControl : Tag const?)

    let isHearable = is_watched_sound || (is_hearable_sound_state(sound_control__state) ? (distance_sq(listener, transform[3]) < square(sound_control__range * 0.99)) :
                                                                                          (distance_sq(listener, transform[3]) < square(sound_control__range * 0.92)))

    sound_control__state = isHearable ? int(SoundControlState.SOUND_CONTROL_IS_HEARABLE) : 0
    if is_watched_sound
      sound_control__state |= int(SoundControlState.SOUND_CONTROL_IS_WATCHED)
    if sound_preset__isLoaded && sound_control__allowSound && disableSoundControl == null
      sound_control__state |= int(SoundControlState.SOUND_CONTROL_IS_PLAYABLE)

    toggle_subtemplate(have_sound(sound_control__state), sound_control__haveSubTemplate, sound_control__subtemplate, eid)


[es(tag=sound, on_appear, track=sound_preset__isLoaded)]
def sound_control_init_range(evt : Event;
                             sound_preset__isLoaded : bool;
                             sound_control__initRange : string;
                             var sound_control__range : float&)
  if sound_preset__isLoaded
    sound_control__range = get_max_distance(sound_control__initRange)
