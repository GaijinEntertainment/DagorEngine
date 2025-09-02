require ecs
require DagorMath
require soundEvent
require soundSystem
require strings
require level
require Dacoll
require danetlibs.renderer.includes.pufd_events


// ..........................................................
//
// rain
//
// ..........................................................

[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def rain_global_sound_var_appear(evt : Event;
                                 rain_global_sound_var__name : string;
                                 var rain_global_sound_var__id : SoundVarId&)
  if sound_banks_is_master_preset_loaded()
    rain_global_sound_var__id = get_var_id_global(rain_global_sound_var__name)
    if !rain_global_sound_var__id
      sound_debug("rain '{rain_global_sound_var__name}' global var is missing: may remove template from scene")
  else
    rain_global_sound_var__id = invalid_sound_var_id()


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def rain_global_sound_var_update(info : ParallelUpdateFrameDelayed;
                                 rain_global_sound_var__mult : float;
                                 rain_global_sound_var__interval : float;
                                 rain_global_sound_var__id : SoundVarId;
                                 var rain_global_sound_var__nextTime : float&)
  if info.curTime >= rain_global_sound_var__nextTime && !!rain_global_sound_var__id
    rain_global_sound_var__nextTime = info.curTime + rain_global_sound_var__interval
    var rain = 0.
    query() <| $ [es] (far_rain__density, far_rain__speed : float)
      rain = far_rain__density * far_rain__speed
    set_var_global(rain_global_sound_var__id, rain * rain_global_sound_var__mult)


// ..........................................................
//
// daytime
//
// ..........................................................

[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def daytime_global_sound_var_appear(evt : Event;
                                    daytime_global_sound_var__name : string;
                                    var daytime_global_sound_var__id : SoundVarId&)
  if sound_banks_is_master_preset_loaded()
    daytime_global_sound_var__id = get_var_id_global(daytime_global_sound_var__name)
    if !daytime_global_sound_var__id
      sound_debug("daytime '{daytime_global_sound_var__name}' global var is missing: may remove template from scene")
  else
    daytime_global_sound_var__id = invalid_sound_var_id()


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def daytime_global_sound_var_update(info : ParallelUpdateFrameDelayed;
                                    daytime_global_sound_var__interval : float;
                                    daytime_global_sound_var__id : SoundVarId;
                                    var daytime_global_sound_var__nextTime : float&)
  if info.curTime >= daytime_global_sound_var__nextTime && !!daytime_global_sound_var__id
    daytime_global_sound_var__nextTime = info.curTime + daytime_global_sound_var__interval
    var timeOfDay = 0.
    query() <| $ [es] (level__timeOfDay : float)
      timeOfDay = level__timeOfDay
    set_var_global(daytime_global_sound_var__id, timeOfDay)


// ..........................................................
//
// altitude
//
// ..........................................................

[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def altitude_global_sound_var_appear(evt : Event;
                                     altitude_global_sound_var__name : string;
                                     var altitude_global_sound_var__id : SoundVarId&)
  if sound_banks_is_master_preset_loaded()
    altitude_global_sound_var__id = get_var_id_global(altitude_global_sound_var__name)
    if !altitude_global_sound_var__id
      sound_debug("altitude '{altitude_global_sound_var__name}' global var is missing: may remove template from scene")
  else
    altitude_global_sound_var__id = invalid_sound_var_id()


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def altitude_global_sound_var_update(info : ParallelUpdateFrameDelayed;
                                     altitude_global_sound_var__interval : float;
                                     altitude_global_sound_var__altitudeToVar : float2;
                                     altitude_global_sound_var__id : SoundVarId;
                                     var altitude_global_sound_var__nextTime : float&)
  if info.curTime >= altitude_global_sound_var__nextTime && !!altitude_global_sound_var__id
    altitude_global_sound_var__nextTime = info.curTime + altitude_global_sound_var__interval
    var altitude = 0.
    let pos = get_listener_pos()
    let ret = find_query() <| $ [es] (water__level : float)
      altitude = pos.y - water__level
      return true
    if !ret
      let pos2d = pos.xz
      let ground = traceht_lmesh(pos2d)
      altitude = pos.y - ground
    set_var_global(altitude_global_sound_var__id, altitude * altitude_global_sound_var__altitudeToVar.x + altitude_global_sound_var__altitudeToVar.y)


// ..........................................................
//
// region
//
// ..........................................................

[es(tag=sound, on_appear, on_event=EventOnMasterSoundPresetLoaded)]
def region_global_sound_var_appear(evt : Event;
                                   region_global_sound_var__name : string;
                                   var region_global_sound_var__id : SoundVarId&)
  if sound_banks_is_master_preset_loaded()
    region_global_sound_var__id = get_var_id_global(region_global_sound_var__name)
    if !region_global_sound_var__id
      sound_debug("region '{region_global_sound_var__name}' global var is missing: may remove template from scene")
  else
    region_global_sound_var__id = invalid_sound_var_id()


[es(tag=sound, after=sound_begin_update_es, before=sound_end_update_es)]
def region_global_sound_var_update(info : ParallelUpdateFrameDelayed;
                                   region_global_sound_var__updateInterval : float;
                                   var region_global_sound_var__nextTime : float&;
                                   region_global_sound_var__id : SoundVarId)
  if info.curTime >= region_global_sound_var__nextTime && !!region_global_sound_var__id
    region_global_sound_var__nextTime = info.curTime + region_global_sound_var__updateInterval
    // will work only if region name is not empty, may bind something at daNetGame/dasModules/level.cpp to fix this
    // or may add & bind some sound specific property to region objects
    let pozXZ = get_listener_pos().xz
    let hasRegionAtPos = !empty(get_region_name_by_pos(pozXZ))
    set_var_global(region_global_sound_var__id, hasRegionAtPos ? 1. : 0.)
