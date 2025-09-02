require ecs
require soundEvent
require soundSystem
require strings


[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def game_effect_sound_appear(evt : Event;
                             game_effect_sound__hero : string;
                             game_effect_sound__enemy : string;
                             var game_effect_sound__event : SoundEvent&;
                             game_effect__attachedTo : EntityId)
  abandon(game_effect_sound__event)
  let sound = has(game_effect__attachedTo, "hero") ? game_effect_sound__hero : game_effect_sound__enemy
  if !empty(sound) && sound_banks_is_master_preset_loaded()
    ecs::query(game_effect__attachedTo) <| $ [es] (transform : float3x4)
      game_effect_sound__event |> play(sound, transform[3])
      game_effect_sound__event.abandonOnReset = true
      if is_oneshot(game_effect_sound__event)
        abandon(game_effect_sound__event)


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=sound_begin_update_es, before=sound_end_update_es)]
def game_effect_sound_update(evt : Event;
                             game_effect_sound__event : SoundEvent&;
                             game_effect__attachedTo : EntityId)
  if is_valid_handle_value(game_effect_sound__event)
    ecs::query(game_effect__attachedTo) <| $ [es] (transform : float3x4)
      set_pos(game_effect_sound__event, transform[3])
