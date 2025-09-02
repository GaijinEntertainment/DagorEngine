options no_aot
require ecs
require ecs.ecs_template
require math.color
require math.base
require math
require DagorMath
require DagorSystem
require PhysMat
require DagorDebug3D
require DagorConsole
require AnimV20
require GeomNodeTree
require soundSystem
require Dacoll


[es(tag=sound, on_appear, after=reverb_sound_appear, REQUIRE=reverbSoundDebug)]
def reverb_sound_debug_appear(evt : Event;
                              reverb_sound__PMIDToAbsorption : FloatList)
  let absCount = length(reverb_sound__PMIDToAbsorption)
  let pmatCount = physMatCount()
  for pmid in range(pmatCount)
    if pmid >= absCount || reverb_sound__PMIDToAbsorption[pmid] < 0.
      logerr("missing physmat {pmid} '{get_material_name(get_material(pmid))}' in reverb_sound__physmatAbsorption")


[ecs_template]
struct debug_reverb_template
  debugReverb : bool


[console_cmd(name="snd.debug_reverb")]
def sound_debug_reverb_console_cmd()
  var entEid : EntityId
  find_query() <| $ [es(REQUIRE=debugReverb)] (eid : EntityId)
    entEid = eid
    return true
  if !entEid
    createEntity("debug_reverb_template")
    sound_debug("snd.debug_reverb=1")
  else
    destroyEntity(entEid)
    sound_debug("snd.debug_reverb=0")


def get_reverb_3d_pos()
  var pos : float3
  query() <| $ [es(REQUIRE=watchedByPlr)] (attach_reverb_sound_to_watchable__nodeId : int; animchar : AnimcharBaseComponent)
    if attach_reverb_sound_to_watchable__nodeId >= 0
      pos = geomtree_getNodeWpos(*animchar.nodeTree, attach_reverb_sound_to_watchable__nodeId) + float3(0., 0.1, 0.)
  return pos


[es(no_order, tag=sound, on_event=EventSoundDrawDebug, REQUIRE=debugReverb)]
def sound_debug_reverb_draw_debug(evt : Event)
  query() <| $ [es] (reverb_sound__rayLength : float;
                     reverb_sound__rays : Point3List)
    begin_draw_cached_debug_lines(false, false, false)
    let a = get_reverb_3d_pos()
    let color = E3DCOLOR(0xffffffff)

    for dir in reverb_sound__rays
      var pmid = PHYSMAT_INVALID
      var norm : float3
      var t = reverb_sound__rayLength
      var riDesc = RendInstDesc()
      if traceray_normalized(a, dir, t, pmid, norm, ETF_LMESH | ETF_FRT | ETF_RI | ETF_STRUCTURES, riDesc, -1)
        var aa = a + dir * t - float3(1., 0., 0.) * 0.1
        var bb = a + dir * t + float3(1., 0., 0.) * 0.1
        draw_cached_debug_line(aa, bb, color)
        aa = a + dir * t - float3(0., 1., 0.) * 0.1
        bb = a + dir * t + float3(0., 1., 0.) * 0.1
        draw_cached_debug_line(aa, bb, color)
        aa = a + dir * t - float3(0., 0., 1.) * 0.1
        bb = a + dir * t + float3(0., 0., 1.) * 0.1
        draw_cached_debug_line(aa, bb, color)
      else
        t = reverb_sound__rayLength
      let b = a + dir * t
      draw_cached_debug_line(a, b, color)
    end_draw_cached_debug_lines()
