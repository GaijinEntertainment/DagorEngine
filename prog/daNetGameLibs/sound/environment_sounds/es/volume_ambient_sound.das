require ecs
require math.base
require math
require DagorMath
require soundEvent
require soundSystem
require soundHash
require danetlibs.renderer.includes.pufd_events
require environment_sounds.modules.game_object_sound_common
require DagorSystem
require DngCamera


def update_impl(info : ParallelUpdateFrameDelayed;
                listener : float3;
                clamped_pos : float3;
                game_object_sound__path : string;
                game_object_sound__name : string;
                volume_ambient_sound__range : float;
                var volume_ambient_sound__closestDistSq : float&;
                var volume_ambient_sound__event : SoundEvent&;
                var volume_ambient_sound__volume : float&;
                volume_ambient_sound__fadeInOutSpeed : float2;
                volume_ambient_sound__reverb_intensity_roomSize_absorption_blend : float4)

  volume_ambient_sound__closestDistSq = distance_sq(listener, clamped_pos)
  let isValidHandle = volume_ambient_sound__event.enabled
  let inRange = (isValidHandle ? (volume_ambient_sound__closestDistSq < square(volume_ambient_sound__range)) :
                                 (volume_ambient_sound__closestDistSq < square(volume_ambient_sound__range * 0.9)))
  volume_ambient_sound__event.enabled = inRange

  if inRange
    if !is_playing(volume_ambient_sound__event)
      release_immediate(volume_ambient_sound__event)
      volume_ambient_sound__event |> reset(play(game_object_sound__name, game_object_sound__path, listener))
      set_volume(volume_ambient_sound__event, volume_ambient_sound__volume)
    set_var(volume_ambient_sound__event, "dist", sqrt(volume_ambient_sound__closestDistSq))

    if volume_ambient_sound__closestDistSq > 0.
      // when outside: update pos once after totalVolumes reached
      set_pos(volume_ambient_sound__event, clamped_pos)
    else
      // when inside: place event to listener each frame
      set_pos(volume_ambient_sound__event, get_listener_pos())

    if volume_ambient_sound__volume < 1.
      volume_ambient_sound__volume = min(volume_ambient_sound__volume + volume_ambient_sound__fadeInOutSpeed.x * info.dt, 1.)
      set_volume(volume_ambient_sound__event, volume_ambient_sound__volume)
  elif is_valid_handle_value(volume_ambient_sound__event)
    volume_ambient_sound__volume -= volume_ambient_sound__fadeInOutSpeed.y * info.dt
    if volume_ambient_sound__volume > 0.
      set_volume(volume_ambient_sound__event, volume_ambient_sound__volume)
    else
      volume_ambient_sound__volume = 0.
      abandon(volume_ambient_sound__event)

  if volume_ambient_sound__volume > 0. && volume_ambient_sound__reverb_intensity_roomSize_absorption_blend.w > 0.
    find_query() <| $ [es] (var reverb_sound__intensity_roomSize_absorption_blend : float4&)
      reverb_sound__intensity_roomSize_absorption_blend.x = volume_ambient_sound__reverb_intensity_roomSize_absorption_blend.x
      reverb_sound__intensity_roomSize_absorption_blend.y = volume_ambient_sound__reverb_intensity_roomSize_absorption_blend.y
      reverb_sound__intensity_roomSize_absorption_blend.z = volume_ambient_sound__reverb_intensity_roomSize_absorption_blend.z
      reverb_sound__intensity_roomSize_absorption_blend.w = volume_ambient_sound__volume * volume_ambient_sound__reverb_intensity_roomSize_absorption_blend.w
      return true


[es(tag=sound, REQUIRE=boxSoundShape, after=(reverb_sound_update, sound_begin_update_es), before=sound_end_update_es)]
def box_volume_ambient_sound_update(info : ParallelUpdateFrameDelayed;
                                    volume_ambient_sound__extent_maxRadiusSq : float4;
                                    game_object_sound__path : string;
                                    game_object_sound__name : string;
                                    volume_ambient_sound__range : float;
                                    var volume_ambient_sound__closestDistSq : float&;
                                    var volume_ambient_sound__event : SoundEvent&;
                                    volume_ambient_sound__transform : float3x4;
                                    var volume_ambient_sound__volume : float&;
                                    volume_ambient_sound__fadeInOutSpeed : float2;
                                    volume_ambient_sound__reverb_intensity_roomSize_absorption_blend : float4)

  if !sound_banks_is_master_preset_loaded()
    return

  let listener = (get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM)[3]
  let clampedPos = place_box_game_object_sound(listener, volume_ambient_sound__extent_maxRadiusSq, volume_ambient_sound__transform)

  update_impl(info,
              listener,
              clampedPos,
              game_object_sound__path,
              game_object_sound__name,
              volume_ambient_sound__range,
              volume_ambient_sound__closestDistSq,
              volume_ambient_sound__event,
              volume_ambient_sound__volume,
              volume_ambient_sound__fadeInOutSpeed,
              volume_ambient_sound__reverb_intensity_roomSize_absorption_blend)


[es(tag=sound, REQUIRE=sphereSoundShape, after=(reverb_sound_update, sound_begin_update_es), before=sound_end_update_es)]
def sphere_volume_ambient_sound_update(info : ParallelUpdateFrameDelayed;
                                       volume_ambient_sound__extent_maxRadiusSq : float4;
                                       game_object_sound__path : string;
                                       game_object_sound__name : string;
                                       var volume_ambient_sound__closestDistSq : float&;
                                       volume_ambient_sound__range : float;
                                       var volume_ambient_sound__event : SoundEvent&;
                                       volume_ambient_sound__transform : float3x4;
                                       var volume_ambient_sound__volume : float&;
                                       volume_ambient_sound__fadeInOutSpeed : float2;
                                       volume_ambient_sound__reverb_intensity_roomSize_absorption_blend : float4)

  if !sound_banks_is_master_preset_loaded()
    return

  let listener = (get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM)[3]
  let clampedPos = place_sphere_game_object_sound(listener, volume_ambient_sound__extent_maxRadiusSq, volume_ambient_sound__transform)

  update_impl(info,
              listener,
              clampedPos,
              game_object_sound__path,
              game_object_sound__name,
              volume_ambient_sound__range,
              volume_ambient_sound__closestDistSq,
              volume_ambient_sound__event,
              volume_ambient_sound__volume,
              volume_ambient_sound__fadeInOutSpeed,
              volume_ambient_sound__reverb_intensity_roomSize_absorption_blend)


[es(tag=sound, REQUIRE=capsuleSoundShape, after=(reverb_sound_update, sound_begin_update_es), before=sound_end_update_es)]
def capsule_volume_ambient_sound_update(info : ParallelUpdateFrameDelayed;
                                        volume_ambient_sound__extent_maxRadiusSq : float4;
                                        game_object_sound__path : string;
                                        game_object_sound__name : string;
                                        var volume_ambient_sound__closestDistSq : float&;
                                        volume_ambient_sound__range : float;
                                        var volume_ambient_sound__event : SoundEvent&;
                                        volume_ambient_sound__transform : float3x4;
                                        var volume_ambient_sound__volume : float&;
                                        volume_ambient_sound__fadeInOutSpeed : float2;
                                        volume_ambient_sound__reverb_intensity_roomSize_absorption_blend : float4)

  if !sound_banks_is_master_preset_loaded()
    return

  let listener = (get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM)[3]
  let clampedPos = place_capsule_game_object_sound(listener, volume_ambient_sound__extent_maxRadiusSq, volume_ambient_sound__transform)

  update_impl(info,
              listener,
              clampedPos,
              game_object_sound__path,
              game_object_sound__name,
              volume_ambient_sound__range,
              volume_ambient_sound__closestDistSq,
              volume_ambient_sound__event,
              volume_ambient_sound__volume,
              volume_ambient_sound__fadeInOutSpeed,
              volume_ambient_sound__reverb_intensity_roomSize_absorption_blend)


[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def volume_ambient_sound_appear(evt : Event;
                                game_object_sound__path : string;
                                game_object_sound__name : string;
                                sound_shape__type : string;
                                transform : float3x4;
                                var volume_ambient_sound__range : float&;
                                volume_ambient_sound__autoRange : bool;
                                var volume_ambient_sound__event : SoundEvent&;
                                var volume_ambient_sound__volume : float&;
                                var volume_ambient_sound__transform : float3x4&;
                                var volume_ambient_sound__extent_maxRadiusSq : float4&)

  release_immediate(volume_ambient_sound__event)
  volume_ambient_sound__event.enabled = false
  volume_ambient_sound__volume = 0.

  if sound_banks_is_master_preset_loaded()

    if volume_ambient_sound__autoRange
      volume_ambient_sound__range = get_max_distance("{game_object_sound__path}/{game_object_sound__name}")

    volume_ambient_sound__transform = transform
    volume_ambient_sound__extent_maxRadiusSq.xyz = make_sound_shape(volume_ambient_sound__transform, sound_hash(sound_shape__type))
    volume_ambient_sound__extent_maxRadiusSq.w = square(find_sound_shape_radius(volume_ambient_sound__extent_maxRadiusSq.xyz, sound_hash(sound_shape__type)) + volume_ambient_sound__range)
