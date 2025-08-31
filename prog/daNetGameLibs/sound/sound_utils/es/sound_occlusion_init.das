require ecs
require ecs.common
require ecs.safe
require soundSystem
require level


[es(no_order, tag=sound, on_appear, on_event=EventLevelLoaded)]
def sound_occlusion_init(evt : Event;
                         sound_occlusion__occluderTypes : Object;
                         sound_occlusion__ri_name_to_occluder_type : Object)
  if is_level_loaded()

    var occluderTypes : array<string>
    var occluderValues : array<float>

    occluderTypes |> reserve(length(sound_occlusion__occluderTypes))
    occluderValues |> reserve(length(sound_occlusion__occluderTypes))

    for it in sound_occlusion__occluderTypes
      occluderTypes |> push(it.key)
      occluderValues |> push(it.value as float ?? 0.)

    var riNames : array<string>
    var riTypes : array<string>

    riNames |> reserve(length(sound_occlusion__ri_name_to_occluder_type))
    riTypes |> reserve(length(sound_occlusion__ri_name_to_occluder_type))

    for it in sound_occlusion__ri_name_to_occluder_type
      riNames |> push(it.key)
      riTypes |> push(it.value as string ?? "")

    sound_init_ri_occluders(riNames, riTypes, occluderTypes, occluderValues)
