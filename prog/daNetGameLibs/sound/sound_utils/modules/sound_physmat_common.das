module sound_physmat_common shared

require ecs
require soundSystem


def get_sound_step_material_id(pmid : int) : int
  var smid = -1
  let ret = find_query() <| $ [es(REQUIRE=soundStepSettings)] (sound_physmat__PMIDToSMID : IntList; sound_physmat__defPhysMatId : int)
    smid = (pmid >= 0 && pmid < length(sound_physmat__PMIDToSMID)) ? sound_physmat__PMIDToSMID[pmid] : sound_physmat__defPhysMatId
    return true
  if !ret
    sound_debug("missing singleton entity with soundStepSettings+sound_physmat")
  return smid


def get_projectile_sound_material_id(pmid : int) : int
  var smid = -1
  let ret = find_query() <| $ [es(REQUIRE=projectileSoundPhysmat)] (sound_physmat__PMIDToSMID : IntList; sound_physmat__defPhysMatId : int)
    smid = (pmid >= 0 && pmid < length(sound_physmat__PMIDToSMID)) ? sound_physmat__PMIDToSMID[pmid] : sound_physmat__defPhysMatId
    return true
  if !ret
    sound_debug("missing singleton entity with projectileSoundPhysmat+sound_physmat")
  return smid


def get_human_melee_impact_sound_material_id(pmid : int) : int
  var smid = -1
  let ret = find_query() <| $ [es(REQUIRE=humanMeleeImpactSoundPhysmat)] (sound_physmat__PMIDToSMID : IntList; sound_physmat__defPhysMatId : int)
    smid = (pmid >= 0 && pmid < length(sound_physmat__PMIDToSMID)) ? sound_physmat__PMIDToSMID[pmid] : sound_physmat__defPhysMatId
    return true
  if !ret
    sound_debug("missing singleton entity with humanMeleeImpactSoundPhysmat+sound_physmat")
  return smid
