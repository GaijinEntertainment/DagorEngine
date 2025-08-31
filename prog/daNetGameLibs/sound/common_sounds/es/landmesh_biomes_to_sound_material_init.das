require ecs
require ecs.safe
require ecs.common
require landMesh
require level
require DagorSystem


[es(tag=sound, on_appear, on_event=EventLevelLoaded)]
def landmesh_biomes_to_sound_material_appear(evt : Event;
                                             landmesh_biomes_to_sound_material__biomes : Object;
                                             var landmesh_biomes_to_sound_material__biomeIdToSound : IPoint2List&;
                                             sound_physmat__soundMaterials : StringList)

  if !is_level_loaded()
    return

  landmesh_biomes_to_sound_material__biomeIdToSound |> resize(max(0, get_num_biome_groups()))
  for biomeId in iter_range(landmesh_biomes_to_sound_material__biomeIdToSound)
    let group = get_biome_group_name(biomeId)
    let obj = landmesh_biomes_to_sound_material__biomes[group] ?as Object
    if obj != null
      let soundmat = obj?.soundmat ?? ""
      let smid = sound_physmat__soundMaterials |> find_index(soundmat)
      //logerr("lbtsma group={group}   soundmat={soundmat}  smid={smid}")
      if smid != -1
        landmesh_biomes_to_sound_material__biomeIdToSound[biomeId].x = smid
        landmesh_biomes_to_sound_material__biomeIdToSound[biomeId].y = (obj?.overrideWater ?? false) ? 1 : 0
      else
        logerr("smid for biome '{group}' is not valid: there is no sound mat '{soundmat}'")
    else
      landmesh_biomes_to_sound_material__biomeIdToSound[biomeId] = int2(-1, 0)
