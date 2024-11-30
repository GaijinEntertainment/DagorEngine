require ecs


[es(tag=sound, on_appear, track=(sound_flags, sound_mods))]
def sound_tags_apply(evt : Event;
                     sound_flags : Object;
                     sound_mods : EidList;
                     var sound_tags : Object&)

  sound_tags |> clear()

  sound_tags := sound_flags

  for eid in sound_mods
    query(eid) <| $ [es] (sound_mod_tags : StringList)
      for tag in sound_mod_tags
        sound_tags |> set(string(tag), true)
