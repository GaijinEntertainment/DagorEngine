options no_aot
require ecs
require DagorMath
require DagorSystem
require PhysMat


[es(tag=sound, on_appear, after=reverb_sound_appear, REQUIRE=reverbSoundDebug)]
def reverb_sound_debug_appear(evt : Event;
                              reverb_sound__PMIDToAbsorption : FloatList)
  let absCount = length(reverb_sound__PMIDToAbsorption)
  let pmatCount = physMatCount()
  for pmid in range(pmatCount)
    if pmid >= absCount || reverb_sound__PMIDToAbsorption[pmid] < 0.
      logerr("missing physmat {pmid} '{get_material_name(get_material(pmid))}' in reverb_sound__physmatAbsorption")
