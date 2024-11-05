require ecs
require sound_utils.modules.sound_player_common


[es(tag=sound, on_appear)]
def game_effect_sound_tag_set(evt : Event;
                              game_effect_sound__tag : string;
                              game_effect__attachedTo : EntityId)
  ecs::query(game_effect__attachedTo) <| $ [es] (var sound_tags : Object&)
    raise_sound_tag(game_effect_sound__tag, sound_tags)


[es(tag=sound, on_disappear)]
def game_effect_sound_tag_unset(evt : Event;
                                game_effect_sound__tag : string;
                                game_effect__attachedTo : EntityId)
  ecs::query(game_effect__attachedTo) <| $ [es] (var sound_tags : Object&)
    clear_sound_tag(game_effect_sound__tag, sound_tags)
