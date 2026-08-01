options no_aot

require DagorConsole
require app // physmat_init
require ecs
require PhysMat

[console_cmd(name="physmat.reload")]
def physmat_reload()
  physmat_init()


[console_cmd(name="physmat.roughness_vis")]
def physmat_roughness_vis()
  let found = find_query() <| $ [es(REQUIRE=terrain_roughness_vis__baseStep)] (eid : EntityId)
    destroyEntity(eid)
    return true
  if !found
    createEntity("terrain_roughness_vis")


[console_cmd(name="physmat.list")]
def physmat_list_cmd()
  let pmatCount = physMatCount()
  for pmid in range(pmatCount)
    console_print("{pmid} : {get_material_name(get_material(pmid))}")


[console_cmd(name="physmat.info")]
def physmat_info_cmd(name : string = "")
  let pmid = get_material_id(name)
  let mat & = get_material(pmid)
  // default material is returned when no material with name found so check if default was returned and if we asked for non default
  if pmid == 0 && get_material_name(mat) != name
    console_print("No material with name '{name}' found.")
    return

  console_print("name: {get_material_name(mat)}:")
  console_print("id: {mat.id}")
  console_print("imp_absorb_k: {mat.imp_absorb_k}")
  console_print("imp_weak_k: {mat.imp_weak_k}")
  console_print("r_bounce_k: {mat.r_bounce_k}")
  console_print("shake_factor: {mat.shake_factor}")
  console_print("mk_dmg: {mat.mk_dmg}")
  console_print("dont_trace: {mat.dont_trace}")
  console_print("clippable: {mat.clippable}")
  console_print("autoReset: {mat.autoReset}")
  console_print("disable_control: {mat.disable_control}")
  console_print("invisible_clipping: {mat.invisible_clipping}")
  console_print("phobj_only: {mat.phobj_only}")
  console_print("damage_k: {mat.damage_k}")
  console_print("deformableWidth: {mat.deformableWidth}")
  console_print("resistanceK: {mat.resistanceK}")
  console_print("completelyTransparent: {mat.completelyTransparent}")
  console_print("lightTransparent: {mat.lightTransparent}")
  console_print("noTransparentThickness: {mat.noTransparentThickness}")
  console_print("fly_through_clip: {mat.fly_through_clip}")
  console_print("stick_k: {mat.stick_k}")
  console_print("lifeTime: {mat.lifeTime}")
  console_print("physBodyMaterial: {mat.physBodyMaterial}")
  console_print("physStaticFriction: {mat.physStaticFriction}")
  console_print("physRestitution: {mat.physRestitution}")
  console_print("camera_collision: {mat.camera_collision}")
  console_print("physics_collision: {mat.physics_collision}")
  console_print("bullets_collision: {mat.bullets_collision}")
  console_print("characters_collision: {mat.characters_collision}")
  console_print("characters_collision2: {mat.characters_collision2}")
  console_print("characters_collision3: {mat.characters_collision3}")
  console_print("directocclusion: {mat.directocclusion}")
  console_print("reverbocclusion: {mat.reverbocclusion}")
  console_print("soundMaterial: {mat.soundMaterial}")
  console_print("isSolid: {mat.isSolid}")
  console_print("tankTracksTexId: {mat.tankTracksTexId}")
  console_print("vehicleHeightmapDeformation: {mat.vehicleHeightmapDeformation}")
  console_print("humanHeightmapDeformation: {mat.humanHeightmapDeformation}")
  console_print("trailDetailStrength: {mat.trailDetailStrength}")

