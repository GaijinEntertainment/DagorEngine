require ecs
require DagorTime
require strings
require das.main_events

[es(tag=playingReplay, on_event=EventLevelLoaded)]
def replay_create_cinematic_mode_settings_entity(evt : Event)
  createEntity("cinematic_mode_settings")

[es(tag=playingReplay, on_appear, track=(isTpsView, isInVehicle), REQUIRE=watchedByPlr)]
def set_all_camera_fov_by_hero(evt : Event; isTpsView : bool; isInVehicle : bool)
  find_query() <| $ [es] (replay__vehicleFov : float; replay__humanFov : float)
    let fov = isInVehicle && !isTpsView ? replay__vehicleFov : replay__humanFov
    broadcastEvent([[CmdSetCameraFov fov=fov]])
    return true

[es(tag=render)]
def set_all_camera_fov_es(evt : CmdSetCameraFov; var fovSettings : float&)
  find_query() <| $ [es(REQUIRE=watchedByPlr)] (isTpsView : bool; isInVehicle : bool)
    find_query() <| $ [es] (var replay__vehicleFov : float&; var replay__humanFov : float&)
      if isInVehicle && !isTpsView
        replay__vehicleFov = evt.fov
      else
        replay__humanFov = evt.fov
      return true
    return true
  fovSettings = evt.fov

[es(tag=render)]
def set_free_tps_camera_speed_es(evt : CmdSetFreeTpsCameraSpeed; var free_cam__move_speed : float&)
  free_cam__move_speed = evt.speed

[es(tag=render)]
def enable_camera_dof(evt : CmdSetCameraDofEnabled)
  query() <| $ [es(REQUIRE=is_dof)] (var dof__on : bool&)
    dof__on = evt.enabled

[es(tag=render, REQUIRE=cinematic_mode__fps)]
def set_cinematic_mode_enabled_es(evt : CmdSetCinematicModeEnabled; eid : EntityId)
  if evt.enabled
    addSubTemplate(eid, "cinematic_mode_manager")
  else
    removeSubTemplate(eid, "cinematic_mode_manager")

[es(tag=render)]
def set_motion_blur_scale_es(evt : CmdSetMotionBlurScale; var motion_blur__scale : float&)
  motion_blur__scale = evt.scale

[es(tag=render)]
def set_bloom_threshold_es(evt : CmdSetBloomThreshold; var bloom__threshold : float&)
  bloom__threshold = evt.threshold

[es(tag=render)]
def set_chromatic_aberration_offset_es(evt : CmdSetChromaticAberrationOffset; var cinematic_mode__chromaticAberration : float3&)
  cinematic_mode__chromaticAberration.z = evt.offset

[es(tag=render)]
def set_film_grain_es(evt : CmdSetFilmGrain; var cinematic_mode__filmGrain : float3&)
  cinematic_mode__filmGrain.x = evt.strength

[es(tag=render)]
def set_vignette_strength_es(evt : CmdSetVignetteStrength; var cinematic_mode__vignetteStrength : float&)
  cinematic_mode__vignetteStrength = evt.strength

[es(tag=render, REQUIRE=is_dof)]
def set_dof_filmic_es(evt : CmdSetDofIsFilmic; var dof__is_filmic : bool&)
  dof__is_filmic = evt.isFilmic

[es(tag=render, REQUIRE=is_dof)]
def set_dof_focus_distance_es(evt : CmdSetDofFocusDistance; var dof__focusDistance : float&)
  dof__focusDistance = evt.focusDistance

[es(tag=render, REQUIRE=is_dof)]
def set_dof_focal_length_es(evt : CmdSetDofFocalLength; var dof__focalLength_mm : float&)
  dof__focalLength_mm = evt.focalLength

[es(tag=render, REQUIRE=is_dof)]
def set_dof_fstop_es(evt : CmdSetDofFStop; var dof__fStop : float&)
  dof__fStop = evt.fStop

[es(tag=render, REQUIRE=is_dof)]
def set_dof_bokeh_corners_es(evt : CmdSetDofBokehCorners; var dof__bokehShape_bladesCount : float&)
  dof__bokehShape_bladesCount = evt.bokehCorners

[es(tag=render, REQUIRE=is_dof)]
def set_dof_bokeh_size_es(evt : CmdSetDofBokehSize; var dof__bokehShape_kernelSize : float&)
  dof__bokehShape_kernelSize = evt.bokehSize

[es(tag=render)]
def set_change_time_of_day_es(evt : CmdChangeTimeOfDay; var cinematic_mode__dayTime : float&)
  cinematic_mode__dayTime = evt.timeOfDay

[es(tag=render)]
def set_weather_preset_of_day_es(evt : CmdWeather; var cinematic_mode__weatherPreset : das_string&)
  cinematic_mode__weatherPreset := evt.preset

[es(tag=render)]
def cinematic_enable_rain_es(evt : CmdSetRain; var cinematic_mode__rain : bool&)
  cinematic_mode__rain = evt.enabled

[es(tag=render)]
def cinematic_enable_show_es(evt : CmdSetSnow; var cinematic_mode__snow : bool&)
  cinematic_mode__snow = evt.enabled

[es(tag=render)]
def cinematic_enable_lightning_es(evt : CmdSetLightning; var cinematic_mode__lightning : bool&)
  cinematic_mode__lightning = evt.enabled

[es(tag=render)]
def cinematic_set_lense_flare_intensity_es(evt : CmdSetLenseFlareIntensity; var cinematic_mode__lenseFlareIntensity : float&)
  cinematic_mode__lenseFlareIntensity = evt.intensity

[es(tag=render, REQUIRE=cinematic_mode_tag, on_disappear)]
def stop_record_on_disable_cinematic_es(evt : Event;
                                        var cinematic_mode__recording : bool&)
  cinematic_mode__recording = false

[es(tag=render, REQUIRE=cinematic_mode_tag)]
def enable_cinematic_recording_es(evt : CmdSetCinemaRecording;
                                  var cinematic_mode__recording : bool&;
                                  var cinematic_mode__fname : das_string&)
  if evt.enabled && !cinematic_mode__recording
    var dt = iso8601_format_sec(uint64(int64(get_clock())))
    dt = dt |> replace("-", "_") |> replace(":", ".")
    cinematic_mode__fname := "records/{dt}.mp4"
  cinematic_mode__recording = evt.enabled

[es(tag=render)]
def set_cinematic_post_fx_bloom_es(evt : CmdSetCinematicPostFxBloom; var cinematic_mode__enablePostBloom : bool&)
  cinematic_mode__enablePostBloom = evt.enabled

[es(tag=render, track=cinematic_mode__enablePostBloom)]
def set_lut_switcher_enabled_es(evt : Event; cinematic_mode__enablePostBloom : bool)
  query() <| $ [es(REQUIRE=watchedByPlr)] (var lut_switcher__enabled : bool&)
    lut_switcher__enabled = cinematic_mode__enablePostBloom

[es(tag=render, on_appear, REQUIRE=watchedByPlr, before=tonemapper_switcher_input)]
def set_lut_switcher_on_hero_change_es(evt : Event; var lut_switcher__enabled : bool&)
  find_query() <| $ [es] (cinematic_mode__enablePostBloom : bool)
    lut_switcher__enabled = cinematic_mode__enablePostBloom
    return true

[es(tag=render)]
def set_cinematic_super_pixels(evt : CmdSetCinematicSetSuperPixels; var cinematic_mode__superPixels : int&)
  cinematic_mode__superPixels = evt.super_pixels

[es(tag=render)]
def cinematic_enable_custom_settings_es(evt : CmdSetCinematicCustomSettings; var settings_override__useCustomSettings : bool&)
  settings_override__useCustomSettings = evt.enabled

[es(tag=(playingReplay, render))]
def set_camera_lerp_factor_es(evt : CmdSetCameraLerpFactor; var replay_camera__tpsLerpFactor : float&)
  replay_camera__tpsLerpFactor = evt.lerpFactor

[es(tag=(playingReplay, render))]
def set_camera_stop_lerp_factor_es(evt : CmdSetCameraStopLerpFactor; var replay_camera__stopInertia : float&)
  replay_camera__stopInertia = evt.stopLerpFactor
