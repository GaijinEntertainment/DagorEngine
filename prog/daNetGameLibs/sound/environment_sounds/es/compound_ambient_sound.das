require ecs
require math.base
require math
require DagorMath
require soundEvent
require soundSystem
require soundHash
require danetlibs.renderer.includes.pufd_events
require environment_sounds.modules.game_object_sound_common
require gameObject
require DagorSystem


[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def compound_ambient_sound_type_appear(evt : Event;
                                       compound_ambient_sound_type__path : string;
                                       var compound_ambient_sound_type__range : float&;
                                       compound_ambient_sound_type__autoRange : bool;
                                       var compound_ambient_sound_type__event : SoundEvent&;
                                       var compound_ambient_sound_type__volume : float&)
  release_immediate(compound_ambient_sound_type__event)
  compound_ambient_sound_type__event.enabled = false
  compound_ambient_sound_type__volume = 0.
  if sound_banks_is_master_preset_loaded()
    if compound_ambient_sound_type__autoRange
      compound_ambient_sound_type__range = get_max_distance(compound_ambient_sound_type__path)
    if compound_ambient_sound_type__range <= 0.
      logerr("bad range for positioned ambient sound event {compound_ambient_sound_type__path}")


def find_closest_volume(box : BBox3; sound_type_hash : uint; listener : float3; max_range_sq : float; var closest_dist_sq : float&; var closest_pt : float3&) : bool

  closest_dist_sq = max_range_sq

  enum_sounds_in_box(box, sound_type_hash) <| $(sound_shape : uint; extents : float3; tm : float3x4#)

    let clampedPos = place_game_object_sound(listener, extents, sound_shape, get_norm_tm(tm, extents))

    let distSq = min(distance_sq(listener, clampedPos), max_range_sq)
    if closest_dist_sq > distSq
      closest_dist_sq = distSq
      closest_pt = clampedPos

  return closest_dist_sq < max_range_sq



[es(tag=sound, after=(reverb_sound_update, sound_begin_update_es), before=sound_end_update_es)]
def compound_ambient_sound_controller_update(info : ParallelUpdateFrameDelayed;
                                             var compound_ambient_sound_controller__curTypeId : int&;
                                             compound_ambient_sound_controller__soundTypes : StringList)

  if !sound_banks_is_master_preset_loaded()
    return

  assume curTypeId = compound_ambient_sound_controller__curTypeId
  assume soundTypes = compound_ambient_sound_controller__soundTypes

  ++curTypeId
  if curTypeId >= length(soundTypes)
    curTypeId = 0

  if uint(curTypeId) >= uint(length(soundTypes))
    return

  let curSoundType = string(soundTypes[curTypeId])
  let curSoundTypeHash = sound_hash(curSoundType)
  var outReverb = float4(0., 0., 0., 0.)

  var ret = false
  query() <| $ [es] (compound_ambient_sound_type__soundType : string; // NOT find_query because should update each event! (set_pos(event, listener))
                     compound_ambient_sound_type__range : float;
                     compound_ambient_sound_type__path : string;
                     compound_ambient_sound_type__fadeInOutSpeed : float2;
                     compound_ambient_sound_type__reverb_intensity_roomSize_absorption_weight : float4;

                     var compound_ambient_sound_type__closestPt : float3&;
                     var compound_ambient_sound_type__closestDistSq : float&;
                     var compound_ambient_sound_type__event : SoundEvent&;
                     var compound_ambient_sound_type__volume : float&)

    assume soundType = compound_ambient_sound_type__soundType
    assume closestDistSq = compound_ambient_sound_type__closestDistSq
    assume closestPt = compound_ambient_sound_type__closestPt
    assume event = compound_ambient_sound_type__event
    assume volume = compound_ambient_sound_type__volume
    assume fadeInOutSpeed = compound_ambient_sound_type__fadeInOutSpeed
    assume reverb = compound_ambient_sound_type__reverb_intensity_roomSize_absorption_weight

    if soundType != curSoundType
      if closestDistSq == 0. && is_valid_handle_value(event)
        set_pos(event, get_listener_pos())
      outReverb = lerp(outReverb, float4(reverb.x, reverb.y, reverb.z, volume * reverb.w), 1. - saturate(outReverb.w))
      return

    ret = true

    let listener = get_listener_pos()
    let maxRange = compound_ambient_sound_type__range * (event.enabled ? 0.98 : 0.9)
    let aa = listener - float3(maxRange, maxRange, maxRange)
    let bb = listener + float3(maxRange, maxRange, maxRange)

    if find_closest_volume(BBox3(aa, bb), curSoundTypeHash, listener, square(maxRange), closestDistSq, closestPt)
      if !event.enabled
        event |> reset(play(compound_ambient_sound_type__path, closestPt))
        event.enabled = true

      if volume < 1.
        volume = min(volume + fadeInOutSpeed.x * float(length(soundTypes)) * info.dt, 1.)
        set_volume(event, volume)

      set_var(event, "dist", sqrt(closestDistSq))
      set_pos(event, closestDistSq == 0. ? listener : closestPt)

    elif event.enabled
      volume -= fadeInOutSpeed.y * float(length(soundTypes)) * info.dt
      if volume > 0.
        set_volume(event, volume)
      else
        volume = 0.
        release_immediate(event)
        event.enabled = false

    outReverb = lerp(outReverb, float4(reverb.x, reverb.y, reverb.z, volume * reverb.w), 1. - saturate(outReverb.w))

  if !ret
    sound_debug("entity/compound_ambient_sound_type with _soundType={curSoundType} not found!")

  if outReverb.w > 0.
    find_query() <| $ [es] (var reverb_sound__intensity_roomSize_absorption_blend : float4&)
      reverb_sound__intensity_roomSize_absorption_blend = outReverb
      return true
