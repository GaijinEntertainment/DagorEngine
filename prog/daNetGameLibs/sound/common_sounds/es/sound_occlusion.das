require ecs
require app
require DagorSystem
require DagorRandom
require math.base
require DagorMath
require GeomNodeTree
require RendInst
require AnimV20
require Dacoll
require soundSystem
require sound_utils_net.modules.sound_utils_net_events
require gameObject
require level


def random_offset(radius : float; pos, from : float3)
  let offset = float3(rnd_float(-radius, radius),
                      rnd_float(-radius, radius),
                      rnd_float(-radius, radius))
  if dot(offset, from - pos) < 0.
    return -offset
  return offset


def make_pos(transform : float3x4 const?;
             animchar : AnimcharBaseComponent const?;
             local_offset : float3)
  if transform != null
    return *transform * local_offset
  var tm : float3x4
  geomtree_getNodeWtmScalar(*animchar.nodeTree, 0, tm)
  return tm * local_offset


def trace(pos, from : float3; ri_extra__handle : riex_handle_t)

  let dist = distance(pos, from)
  let dir = (pos - from) * safeinv(dist)
  var t = dist
  var norm : float3
  var pmid : int
  var riDesc = RendInstDesc()
  if !traceray_normalized(from, dir, t, pmid, norm, ETF_FRT | ETF_RI | ETF_STRUCTURES, riDesc, -1)
    return 0.
  if riDesc.isRiExtra && ri_extra__handle == riDesc.riExtraHandle // ignore ri associated with current
    return 0.

  return 1. // occluded


def is_pos_indoor(game_objects : GameObjects; pos : float3; search_threshold = 0.5f) : bool
  let box = BBox3(pos, search_threshold)
  var indoor = false
  game_objects.indoors |> for_scene_game_objects(box) <| $ [unused_argument(ni)] (ni : uint; tm : float3x4#)
    indoor ||= IDENT_BBOX & (inverse(tm) * pos)
  return indoor


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def sound_occlusion_update(info : ParallelUpdateFrameDelayed;
                           var sound_occlusion__targetId : int&;
                           var sound_occlusion__numTargets : int&;
                           sound_occlusion__maxFullTime : float;
                           sound_occlusion__maxTargetTime : float;
                           sound_occlusion__maxSamplesPerTarget : int)

  let maxTimePerTarget : float = min(sound_occlusion__maxFullTime / float(max(sound_occlusion__numTargets, 1)), sound_occlusion__maxTargetTime)

  var nextTargetId = 0
  let from = get_listener_pos()
  sound_occlusion__numTargets = 0

  query() <| $ [es] (occluded_sound__nearFarRange : float2;
                     occluded_sound__radius : float;
                     occluded_sound__enabled : bool;
                     occluded_sound__localOffset : float3;
                     occluded_sound__secondaryOffset : float3;
                     is_watched_sound = false;
                     transform : float3x4 const? = null;
                     animchar : AnimcharBaseComponent const? = null;
                     occluded_sound__viscosity : float;
                     var occluded_sound__occlusion : float&;
                     var occluded_sound__numSamples : int&;
                     var occluded_sound__sumTimeResult : float3&;
                     ri_extra__handle : riex_handle_t = RIEX_HANDLE_NULL)

    if !occluded_sound__enabled || is_watched_sound || (transform == null && animchar == null)
      occluded_sound__occlusion = 0.
      occluded_sound__sumTimeResult = float3(1., 0., 0.)
      return

    assume _sum = occluded_sound__sumTimeResult.x
    assume _time = occluded_sound__sumTimeResult.y
    assume _result = occluded_sound__sumTimeResult.z
    assume _nSamples = occluded_sound__numSamples

    if sound_occlusion__targetId == sound_occlusion__numTargets

      let pos = make_pos(transform, animchar, occluded_sound__localOffset)
      let distSq = length_sq(pos - from)

      var isListenerIndoor = false
      var isTargetIndoor = false

      find_query() <| $ [es] (game_objects : GameObjects)
        isListenerIndoor = is_pos_indoor(game_objects, from, 0.5)
        isTargetIndoor = is_pos_indoor(game_objects, pos, 0.5)
        return true

      if isTargetIndoor != isListenerIndoor
        _sum = 1.
        ++_nSamples

      elif distSq > square(occluded_sound__nearFarRange.y)
        _sum = 1.
        ++_nSamples

      elif occluded_sound__radius != 0.
        _sum += trace(pos + random_offset(occluded_sound__radius, pos, from), from, ri_extra__handle)
        ++_nSamples
        if info.curTime < _time + maxTimePerTarget && _nSamples < sound_occlusion__maxSamplesPerTarget
          return

      elif _nSamples == 0
        ++_nSamples
        _sum += trace(pos, from, ri_extra__handle) // first
        if _sum != 0. && length_sq(occluded_sound__secondaryOffset) != 0.
          _sum = 0.
          return // occluded if both first AND secondary occluded
      else
        _sum += trace(make_pos(transform, animchar, occluded_sound__secondaryOffset), from, ri_extra__handle) // secondary

      nextTargetId = sound_occlusion__targetId + 1

      _result = cvt(sqrt(distSq),
                    occluded_sound__nearFarRange.x,
                    occluded_sound__nearFarRange.y,
                    _sum / float(_nSamples),
                    0.) // 0=unoccluded,1=occluded

      _sum = 0.
      _nSamples = 0
      _time = info.curTime

    occluded_sound__occlusion = saturate(approach(occluded_sound__occlusion, _result, info.dt, occluded_sound__viscosity))
    ++sound_occlusion__numTargets


  sound_occlusion__targetId = nextTargetId
