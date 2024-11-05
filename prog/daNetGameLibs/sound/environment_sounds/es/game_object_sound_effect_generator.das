require ecs
require ecs.safe
require soundSystem
require gameObject
require math.base


[es(tag=sound, on_appear, on_event=EventGameObjectsCreated)]
def game_object_sound_effect_generator(evt : Event;
                                       game_object_sound_effect_generator__entityTemplate : string;
                                       game_object_sound_effect_generator__pathComponentName : string;
                                       game_object_sound_effect_generator__gameObjectToEventPath : Object)

  query() <| $ [es(REQUIRE=generatedSoundEffect)] (eid : EntityId)
    destroyEntity(eid)

  query() <| $ [es] (game_objects : GameObjects)
    var totalObjects = 0
    var itm = IDENT_TM

    for it in game_object_sound_effect_generator__gameObjectToEventPath
      let gameObjectName = string(it.key)
      let path = it.value ?? ""
      let scene = game_objects |> get_scene_game_objects_by_name(gameObjectName)
      if scene != null
        scene |> find_scene_game_objects() <| $ [unused_argument(ni)] (ni : uint; tm : float3x4#)
          ++totalObjects

          createEntity(game_object_sound_effect_generator__entityTemplate) <| $(var init)
            itm[3] = tm[3]
            init |> set("transform", itm)
            init |> set(game_object_sound_effect_generator__pathComponentName, path)

          return false

    sound_debug("{totalObjects} effects total for game_object_sound_effect_generator")
