require ecs
require ecs.common
require PhysMat
require DagorSystem
require daslib/assert_once


def sound_material_to_smid(name : string; sound_physmat__soundMaterials : StringList; def_smid : float) : float
  let idx = find_index(sound_physmat__soundMaterials, name)
  return idx >= 0 ? float(idx) : def_smid


[es(tag=sound, on_appear)]
def sound_physmat_on_appear(evt : Event;
                            var sound_physmat__PMIDToSMID : FloatList&;
                            sound_physmat__defPhysMatId : float;
                            sound_physmat__defSoundMatId : float;
                            sound_physmat__soundMaterials : StringList;
                            sound_physmat__physmatToSoundmat : Object)
  var maxCount = 0
  for it in sound_physmat__physmatToSoundmat
    let mid = get_material_id(it.key)
    if mid != PHYSMAT_INVALID
      maxCount = max(maxCount, int(mid) + 1)

  sound_physmat__PMIDToSMID |> resize(maxCount)

  for idx in range(maxCount)
    sound_physmat__PMIDToSMID[idx] = sound_physmat__defPhysMatId

  for it in sound_physmat__physmatToSoundmat
    let mid = get_material_id(it.key) // will return PHYSMAT_DEFAULT when material is missing
    if mid != PHYSMAT_DEFAULT && mid != PHYSMAT_INVALID
      assert_once(int(mid) >= 0)
      if int(mid) >= 0
        let soundMat = get_ecs_string(it.value)
        if soundMat != null
          sound_physmat__PMIDToSMID[int(mid)] = sound_material_to_smid(string(*soundMat), sound_physmat__soundMaterials, sound_physmat__defSoundMatId)
