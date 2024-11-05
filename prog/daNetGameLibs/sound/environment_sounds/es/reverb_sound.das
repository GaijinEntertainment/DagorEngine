require ecs
require ecs.safe
require soundEvent
require soundSystem
require math.base
require DagorMath
require Dacoll
require AnimV20
require GeomNodeTree
require DagorSystem
require strings
require PhysMat


[es(tag=sound, on_appear)]
def reverb_sound_appear(evt : Event;
                        reverb_sound__physmatAbsorption : Array;
                        reverb_sound__defAbsorption : float;
                        var reverb_sound__PMIDToAbsorption : FloatList&)
  var maxCount = 0
  for it in reverb_sound__physmatAbsorption
    let physmats = get_ecs_StringList(it as Object?.physmats)
    if physmats != null
      for physmat in *physmats
        let pmid = get_material_id(string(physmat))
        maxCount = max(maxCount, int(pmid) + 1)

  resize(reverb_sound__PMIDToAbsorption, maxCount)
  for idx in range(maxCount)
    reverb_sound__PMIDToAbsorption[idx] = -1.

  for it in reverb_sound__physmatAbsorption
    let grp = it as Object
    let valueMinMax = grp?.valueMinMax ?as float2 ?? float2(0., 0.)
    let physmats = get_ecs_StringList(grp?.physmats)
    if physmats != null
      var idx = 0.
      let ab = float2(0., float(length(*physmats)) - 1.)
      for physmat in *physmats
        let val = saturate(cvt(idx++, ab.x, ab.y, valueMinMax.x, valueMinMax.y))
        let pmid = get_material_id(string(physmat))
        if pmid == PHYSMAT_INVALID || pmid < 0 || pmid >= maxCount
          logerr("pmid is PHYSMAT_INVALID for '{physmat}' specified in reverb_sound__physmatAbsorption")
        elif pmid == PHYSMAT_DEFAULT
          reverb_sound__PMIDToAbsorption[pmid] = reverb_sound__defAbsorption
          if physmat != get_material_name(get_material(PHYSMAT_DEFAULT))
            logerr("physmat '{physmat}' specified in reverb_sound__physmatAbsorption treated as 'default'")
        else
          if reverb_sound__PMIDToAbsorption[pmid] != -1.
            logerr("duplicate physmat '{physmat}' in reverb_sound__physmatAbsorption")
          reverb_sound__PMIDToAbsorption[pmid] = val


def make_param(value : float; max_value : float; num_rays : int)
  return saturate(safediv(value, float(num_rays) * max_value)) * 100.


def get_reverb_3d_pos()
  var pos = get_listener_pos()
  query() <| $ [es] (is_watched_sound : bool;
                     human_reverb_sound__nodeId : int;
                     animchar : AnimcharBaseComponent)
    if is_watched_sound && human_reverb_sound__nodeId >= 0
      pos = geomtree_getNodeWpos(*animchar.nodeTree, human_reverb_sound__nodeId) + float3(0., 0.1, 0.)
  return pos


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=(animchar_before_render_es, sound_begin_update_es), before=sound_end_update_es)]
def reverb_sound_update(evt : Event;
                        var reverb_sound__rayId : int&;
                        var reverb_sound__intensityValue : float&;
                        var reverb_sound__roomSizeValue : float&;
                        var reverb_sound__absorptionValue : float&;
                        var reverb_sound__absorptionRays : int&;
                        var reverb_sound__intensity_roomSize_absorption_weight : float4&;
                        reverb_sound__PMIDToAbsorption : FloatList;
                        reverb_sound__defAbsorption : float;
                        reverb_sound__absorptionVar : string;
                        reverb_sound__absorptionNormalize : bool;
                        reverb_sound__intensityMax : float;
                        reverb_sound__intensityVar : string;
                        reverb_sound__roomSizeMax : float;
                        reverb_sound__roomSizeVar : string;
                        reverb_sound__rays : Point3List;
                        reverb_sound__rayLength : float;
                        reverb_sound__fadeOutPower : float;
                        reverb_sound__event : SoundEvent&)
  if !is_valid_handle_value(reverb_sound__event)
    return

  let numRays = length(reverb_sound__rays)
  assert(uint(reverb_sound__rayId) < uint(numRays))
  let pos = get_reverb_3d_pos()
  let dir = reverb_sound__rays[reverb_sound__rayId]
  set_pos(reverb_sound__event, pos)

  if reverb_sound__intensity_roomSize_absorption_weight.w >= 1.
    set_var_global(reverb_sound__intensityVar, reverb_sound__intensity_roomSize_absorption_weight.x)
    set_var(reverb_sound__event, reverb_sound__roomSizeVar, reverb_sound__intensity_roomSize_absorption_weight.y)
    set_var(reverb_sound__event, reverb_sound__absorptionVar, reverb_sound__intensity_roomSize_absorption_weight.z)
    reverb_sound__rayId = 0
    reverb_sound__intensityValue = 0.
    reverb_sound__roomSizeValue = 0.
    reverb_sound__absorptionValue = 0.
    reverb_sound__absorptionRays = 0
  else
    var pmid = PHYSMAT_INVALID
    var norm : float3
    var t = reverb_sound__rayLength
    var riDesc = RendInstDesc()
    if traceray_normalized(pos, dir, t, pmid, norm, ETF_FRT | ETF_RI | ETF_STRUCTURES, riDesc, -1)
      let distT = saturate(safediv(t, reverb_sound__rayLength))
      reverb_sound__intensityValue += saturate(-dot(norm, dir)) * (1. - pow(distT, reverb_sound__fadeOutPower))
      reverb_sound__roomSizeValue += t
      if pmid >= 0 && pmid < length(reverb_sound__PMIDToAbsorption)
        reverb_sound__absorptionValue += reverb_sound__PMIDToAbsorption[pmid]
      else
        reverb_sound__absorptionValue += reverb_sound__defAbsorption
      ++reverb_sound__absorptionRays
    else
      reverb_sound__roomSizeValue += reverb_sound__rayLength
      if !reverb_sound__absorptionNormalize
        ++reverb_sound__absorptionRays

    ++reverb_sound__rayId
    if reverb_sound__rayId >= numRays
      let valueFlipped = 1. - reverb_sound__intensity_roomSize_absorption_weight.w
      let intensity = make_param(reverb_sound__intensityValue, reverb_sound__intensityMax, numRays) * valueFlipped + reverb_sound__intensity_roomSize_absorption_weight.x * reverb_sound__intensity_roomSize_absorption_weight.w
      let roomSize = make_param(reverb_sound__roomSizeValue, reverb_sound__roomSizeMax, numRays) * valueFlipped + reverb_sound__intensity_roomSize_absorption_weight.y * reverb_sound__intensity_roomSize_absorption_weight.w
      let absorption = reverb_sound__absorptionValue / float(max(1, reverb_sound__absorptionRays)) * valueFlipped + reverb_sound__intensity_roomSize_absorption_weight.z * reverb_sound__intensity_roomSize_absorption_weight.w

      set_var_global(reverb_sound__intensityVar, intensity)
      set_var(reverb_sound__event, reverb_sound__roomSizeVar, roomSize)
      set_var(reverb_sound__event, reverb_sound__absorptionVar, absorption)

      reverb_sound__rayId = 0
      reverb_sound__intensityValue = 0.
      reverb_sound__roomSizeValue = 0.
      reverb_sound__absorptionValue = 0.
      reverb_sound__absorptionRays = 0

  reverb_sound__intensity_roomSize_absorption_weight.w = 0.


[es(tag=sound, track=sound_banks_state__isPresetLoaded)]
def reverb_sound_toggle(evt : Event;
                        sound_banks_state__isPresetLoaded : bool;
                        reverb_sound__path : string;
                        reverb_sound__intensityVar : string;
                        var reverb_sound__event : SoundEvent&)
  release_immediate(reverb_sound__event)
  if sound_banks_state__isPresetLoaded
    reverb_sound__event |> play(reverb_sound__path, get_reverb_3d_pos())
    set_var_global(reverb_sound__intensityVar, 0.)


[es(tag=sound, on_disappear)]
def reverb_sound_destroyed(evt : Event;
                           reverb_sound__intensityVar : string;
                           var reverb_sound__event : SoundEvent&)
  release(reverb_sound__event)
  set_var_global(reverb_sound__intensityVar, 0.)


[es(tag=sound, on_appear)]
def human_reverb_sound_init_node_id(evt : Event;
                                    animchar : AnimcharBaseComponent;
                                    human_animchar_sound__reverbNode : string;
                                    var human_reverb_sound__nodeId : int&)
  if !empty(human_animchar_sound__reverbNode)
    human_reverb_sound__nodeId = geomtree_findNodeIndex(*animchar.nodeTree, human_animchar_sound__reverbNode)
    if human_reverb_sound__nodeId == -1
      logerr("Reverb node '{human_animchar_sound__reverbNode}' not found")
