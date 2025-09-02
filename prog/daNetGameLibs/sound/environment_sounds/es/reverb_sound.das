require ecs
require ecs.safe
require soundEvent
require soundSystem
require math.random
require math.base
require math
require DagorMath
require Dacoll
require AnimV20
require GeomNodeTree
require DagorSystem
require strings
require PhysMat


[es(tag=sound, on_appear)]
def reverb_sound_init_absorption(evt : Event;
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


[es(tag=sound, on_appear)]
def human_reverb_sound_init_node_id(evt : Event;
                                    animchar : AnimcharBaseComponent;
                                    attach_reverb_sound_to_watchable__nodeName : string;
                                    var attach_reverb_sound_to_watchable__nodeId : int&)
  if !empty(attach_reverb_sound_to_watchable__nodeName)
    attach_reverb_sound_to_watchable__nodeId = geomtree_findNodeIndex(*animchar.nodeTree, attach_reverb_sound_to_watchable__nodeName)
    if attach_reverb_sound_to_watchable__nodeId == -1
      logerr("Reverb node '{attach_reverb_sound_to_watchable__nodeName}' not found")


[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def reverb_sound_appear(evt : Event;
                        reverb_sound__intensityVar : string;
                        reverb_sound__path : string;
                        var reverb_sound__rayId : int&;
                        var reverb_sound__offsetId : int&;
                        var reverb_sound__intensity_roomSize_absorption_weight : float4&;
                        var reverb_sound__intensity_roomSize_absorption_blend : float4&;
                        var reverb_sound__event : SoundEvent&)
  reverb_sound__event |> release_immediate()
  if sound_banks_is_master_preset_loaded()
    reverb_sound__event |> play(reverb_sound__path, get_reverb_3d_pos())
    set_var_global(reverb_sound__intensityVar, 0.)
  reverb_sound__intensity_roomSize_absorption_blend = float4(0., 0., 0., 0.)
  reverb_sound__intensity_roomSize_absorption_weight = float4(0., 0., 0., 0.)
  reverb_sound__rayId = 0
  reverb_sound__offsetId = 0


[es(tag=sound, on_disappear)]
def reverb_sound_disappear(evt : Event;
                           reverb_sound__intensityVar : string)
  if sound_banks_is_master_preset_loaded()
    set_var_global(reverb_sound__intensityVar, 0.)


def make_param(value, max_value : float; max_param = 100.)
  return saturate(safediv(value, max_value)) * max_param


def get_reverb_3d_pos()
  var pos = get_listener_pos()
  query() <| $ [es(REQUIRE=watchedByPlr)] (attach_reverb_sound_to_watchable__nodeId : int; animchar : AnimcharBaseComponent)
    if attach_reverb_sound_to_watchable__nodeId >= 0
      pos = geomtree_getNodeWpos(*animchar.nodeTree, attach_reverb_sound_to_watchable__nodeId) + float3(0., 0.1, 0.)
  return pos


[es(tag=sound, on_event=ParallelUpdateFrameDelayed, after=(animchar_before_render_es, sound_begin_update_es), before=sound_end_update_es)]
def reverb_sound_update(evt : Event;
                        var reverb_sound__rayId : int&;
                        var reverb_sound__offsetId : int&;
                        var reverb_sound__intensity_roomSize_absorption_weight : float4&;
                        var reverb_sound__intensity_roomSize_absorption_blend : float4&;
                        reverb_sound__PMIDToAbsorption : FloatList;
                        reverb_sound__defAbsorption : float;
                        reverb_sound__absorptionVar : string;
                        reverb_sound__intensityMax : float;
                        reverb_sound__intensityVar : string;
                        reverb_sound__roomSizeMax : float;
                        reverb_sound__roomSizeVar : string;
                        reverb_sound__rayLength : float;
                        reverb_sound__fadeOutPower : float;
                        reverb_sound__decay : float;
                        reverb_sound__numTraces : int = 2;
                        reverb_sound__rays : Point3List;
                        reverb_sound__offsets : Point3List;
                        reverb_sound__event : SoundEvent&)

  if !sound_banks_is_master_preset_loaded()
    return

  assume _intensity = reverb_sound__intensity_roomSize_absorption_weight.x
  assume _roomSize = reverb_sound__intensity_roomSize_absorption_weight.y
  assume _absorption = reverb_sound__intensity_roomSize_absorption_weight.z
  assume _weight = reverb_sound__intensity_roomSize_absorption_weight.w
  assume blend = reverb_sound__intensity_roomSize_absorption_blend

  if !is_valid_handle_value(reverb_sound__event)
    return

  let maxRays = length(reverb_sound__rays)
  let maxOffsets = length(reverb_sound__offsets)
  if maxRays <= 0 || maxOffsets <= 0
    return

  let up = get_listener_up()
  let upDecayDot = float2(-0.5, 0.)
  let pos = get_reverb_3d_pos()
  set_pos(reverb_sound__event, pos)

  if blend.w >= 1.
    set_var_global(reverb_sound__intensityVar, blend.x)
    set_var(reverb_sound__event, reverb_sound__roomSizeVar, blend.y)
    set_var(reverb_sound__event, reverb_sound__absorptionVar, blend.z)
    reverb_sound__rayId = 0
    reverb_sound__offsetId = 0
    reverb_sound__intensity_roomSize_absorption_weight = float4(0., 0., 0., 0.)
  else
    var i = reverb_sound__numTraces
    while --i >= 0

      var pmid = PHYSMAT_INVALID
      var norm : float3
      var t = reverb_sound__rayLength
      var riDesc = RendInstDesc()
      let dir = reverb_sound__rays[reverb_sound__rayId] * reverb_sound__offsets[reverb_sound__offsetId]
      let u = cvt(dot(dir, up), upDecayDot.x, upDecayDot.y, 0., 1.)
      if u > 0. && traceray_normalized(pos, dir, t, pmid, norm, ETF_LMESH | ETF_FRT | ETF_RI | ETF_STRUCTURES, riDesc, -1)
        _roomSize += t * u
        let distT = saturate(safediv(t, reverb_sound__rayLength))
        _intensity += saturate(-dot(norm, dir)) * (1. - pow(distT, reverb_sound__fadeOutPower)) * u
        if pmid >= 0 && pmid < length(reverb_sound__PMIDToAbsorption)
          _absorption += reverb_sound__PMIDToAbsorption[pmid] * u
        else
          _absorption += reverb_sound__defAbsorption * u
      else
        _roomSize += reverb_sound__rayLength * u
      _weight += u

      if ++reverb_sound__rayId >= maxRays

        let intensity = lerp(make_param(_intensity, _weight * reverb_sound__intensityMax), blend.x, blend.w)
        let roomSize = lerp(make_param(_roomSize, _weight * reverb_sound__roomSizeMax), blend.y, blend.w)
        let absorption = lerp(make_param(_absorption, _weight, 1.), blend.z, blend.w)

        set_var_global(reverb_sound__intensityVar, intensity)
        set_var(reverb_sound__event, reverb_sound__roomSizeVar, roomSize)
        set_var(reverb_sound__event, reverb_sound__absorptionVar, absorption)

        //logerr("intensity={intensity}  roomSize={roomSize}   absorption={absorption}     weight={_weight}  offset={reverb_sound__offsets[reverb_sound__offsetId]}")

        // new cycle
        reverb_sound__rayId = 0
        reverb_sound__offsetId = (reverb_sound__offsetId + 1) % maxOffsets
        reverb_sound__intensity_roomSize_absorption_weight *= reverb_sound__decay

  blend.w = 0.
