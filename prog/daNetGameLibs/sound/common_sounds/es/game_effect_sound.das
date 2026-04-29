require ecs
require soundEvent
require soundSystem
require strings


def toggle(game_effect_sound__hero : string;
           game_effect_sound__enemy : string;
           var game_effect_sound__event : SoundEvent&;
           game_effect__attachedTo : EntityId)
  abandon(game_effect_sound__event)
  if sound_banks_is_master_preset_loaded()
    query(game_effect__attachedTo) <| $ [es] (transform : float3x4; is_watched_sound = false)
      let path = is_watched_sound ? game_effect_sound__hero : game_effect_sound__enemy
      if !empty(path)
        game_effect_sound__event |> play(path, transform[3])
        game_effect_sound__event.abandonOnReset = true
        if is_oneshot(game_effect_sound__event)
          abandon(game_effect_sound__event)


[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def game_effect_sound_appear(evt : Event;
                             game_effect_sound__hero : string;
                             game_effect_sound__enemy : string;
                             var game_effect_sound__event : SoundEvent&;
                             game_effect__attachedTo : EntityId)
  toggle(game_effect_sound__hero, game_effect_sound__enemy, game_effect_sound__event, game_effect__attachedTo)


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es)]
def game_effect_sound_update(evt : Event;
                             game_effect_sound__event : SoundEvent&;
                             game_effect__attachedTo : EntityId)
  if is_valid_handle_value(game_effect_sound__event)
    query(game_effect__attachedTo) <| $ [es] (transform : float3x4)
      set_pos(game_effect_sound__event, transform[3])


[es(tag=sound, track=is_watched_sound, REQUIRE=is_watched_sound)]
def entity_with_attached_game_effect__track_watched_sound(evt : Event; eid : EntityId)
  query() <| $ [es] (game_effect_sound__hero : string;
                     game_effect_sound__enemy : string;
                     var game_effect_sound__event : SoundEvent&;
                     game_effect__attachedTo : EntityId)
    if eid == game_effect__attachedTo && is_playing(game_effect_sound__event) && !is_oneshot(game_effect_sound__event)
      toggle(game_effect_sound__hero, game_effect_sound__enemy, game_effect_sound__event, game_effect__attachedTo)
