require ecs
require math.random
require DagorMath
require DagorShaders
require AnimV20
require DagorMaterials
require math.base
require app // get_sync_time
require danetlibs.light_flicker.render.light_flicker_common
require daSkies


def is_night_time()
  let skies = get_daskies()
  if skies == null
    return true
  return skies.sunDir.y < NIGHT_SUN_COS


def calc_effect_tm(pos : float3; norm : float3)
  var tm = IDENT_TM
  tm[1] = normalize(norm)
  tm[2] = normalize(cross(float3(1, 0, 0), tm[1]))
  tm[0] = normalize(cross(tm[1], tm[2]))
  tm[3] = pos
  return tm

def smoothstep(edge0 : float; edge1 : float; x : float)
   // Scale, and clamp x to 0..1 range
  let c = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0)
  return c * c * (3.0 - 2.0 * c)

def lf_hash(v : uint)
  var n = (v << 13U) ^ v
  n = n * (n * n * 15731U + 789221U) + 1376312589U
  return float(n & 0x0fffffffU) / float(0x0fffffff)

def grad_noise(x : float)
  let i = uint(floor(x))

  let f = frac(x)
  let u = f * f * (3.0 - 2.0 * f)

  let g0 = lf_hash(i + 0u)
  let g1 = lf_hash(i + 1u)

  return g0 + (g1 - g0) * u

[es(on_appear)]
def init_flicker_light(evt : Event;
                       eid : EntityId;
                       var light_flicker__seed : int&;
                       var light_flicker__time : float&)
  if (light_flicker__seed != 0)
    return

  light_flicker__time = get_sync_time()
  light_flicker__seed = int(uint(eid)) ^ get_rnd_seed()

[es(tag=render, after=after_camera_sync)]
def light_flicker_update2_es(info : UpdateStageInfoAct;
                             light_flicker__brightnesses : float3;
                             light_flicker__timings : float3;
                             light_flicker__timingMul : float;
                             light_flicker__brigtnessDeviation : float;
                             light_flicker__timeDeviation : float;
                             light_flicker__noiseToFlatRatio : float;
                             light_flicker__noisePhase : float;
                             light_flicker__noiseVelocity : float;
                             var light_flicker__currentBase : float&;
                             var light_flicker__changeAt : float&;
                             var light_flicker__seed : int&;
                             var light_flicker__brightness : float&)
  if light_flicker__changeAt < info.curTime
    let nextStateRnd = _rnd_float(light_flicker__seed, .0, 3.0)
    if nextStateRnd < 1.0
      light_flicker__currentBase = light_flicker__brightnesses.x
    elif nextStateRnd < 2.0
      light_flicker__currentBase = light_flicker__brightnesses.y
    else
      light_flicker__currentBase = light_flicker__brightnesses.z

    light_flicker__currentBase *= _rnd_float(light_flicker__seed, 1.0 - light_flicker__brigtnessDeviation, 1.0 + light_flicker__brigtnessDeviation)

    let nextTimeRnd = _rnd_float(light_flicker__seed, .0, 3.0)
    var duration = .0
    if nextTimeRnd < 1.0
      duration = light_flicker__timings.x
    elif nextTimeRnd < 2.0
      duration = light_flicker__timings.y
    else
      duration = light_flicker__timings.z

    duration *= light_flicker__timingMul
    light_flicker__changeAt = info.curTime + duration * _rnd_float(light_flicker__seed, 1.0 - light_flicker__timeDeviation, 1.0 + light_flicker__timeDeviation)

  let noise = perlin_noise1(light_flicker__noiseVelocity * info.curTime + light_flicker__noisePhase)
  light_flicker__brightness = light_flicker__currentBase + noise * light_flicker__noiseToFlatRatio



[es(tag=render, after=after_camera_sync)]
def light_flicker_update_es(info : UpdateStageInfoAct;
                            light_flicker__on_time : float2;
                            light_flicker__off_time : float2;
                            light_flicker__attack_time : float2;
                            light_flicker__release_time : float2;
                            light_flicker__on_brightness : float2;
                            light_flicker__off_brightness : float2;
                            light_flicker__noise_brightness_on : float2;
                            light_flicker__noise_brightness_off : float2;
                            light_flicker__noise_time : float;
                            light_flicker__nightly : bool;
                            var light_flicker__time : float&;
                            var light_flicker__seed : int&;
                            var light_flicker__brightness : float&;
                            var light_flicker__state : int&)
  var seed = light_flicker__seed
  let local_time = info.curTime - light_flicker__time

  let off_time = _rnd_float(seed, light_flicker__off_time.x, light_flicker__off_time.y)
  let on_time = _rnd_float(seed, light_flicker__on_time.x, light_flicker__on_time.y)
  let on_brightness = _rnd_float(seed, light_flicker__on_brightness.x, light_flicker__on_brightness.y)
  let off_brightness = _rnd_float(seed, light_flicker__off_brightness.x, light_flicker__off_brightness.y)
  let attack_time = _rnd_float(seed, light_flicker__attack_time.x, light_flicker__attack_time.y)
  let release_time = _rnd_float(seed, light_flicker__release_time.x, light_flicker__release_time.y)

  let total_cycle_time = off_time + attack_time + on_time + release_time

  let flicker_on = smoothstep(off_time, off_time + attack_time, local_time)
  let flicker_off = (1.0 - smoothstep(off_time + attack_time + on_time, total_cycle_time, local_time))
  let flicker_status = flicker_on * flicker_off

  let isInAttack = flicker_on > 0.0 && flicker_on < 1.0
  let isInOn = flicker_status == 1.0
  let isInOff = flicker_status == 0.0 || (light_flicker__nightly && !is_night_time())
  light_flicker__state = int(isInOff ? LightFlickerState.Off
                            : isInAttack ? LightFlickerState.Attack
                            : isInOn ? LightFlickerState.On
                            : LightFlickerState.Release)

  let brightness = lerp(off_brightness, on_brightness, flicker_status)
  let min_max_noise = lerp(light_flicker__noise_brightness_off, light_flicker__noise_brightness_on, float2(flicker_status))

  let noise_time = info.curTime * light_flicker__noise_time
  let noise_brightness = lerp(min_max_noise.x, min_max_noise.y, grad_noise(noise_time))

  light_flicker__brightness = brightness * noise_brightness

  if (local_time > total_cycle_time)
    light_flicker__time += total_cycle_time
    light_flicker__seed = seed

def find_or_create_parent(parent_template : string; flicking_eid : EntityId)
  if (parent_template == "")
    return INVALID_ENTITY_ID

  var parentEid = INVALID_ENTITY_ID
  find_query() <| $ [es] (eid : EntityId;
                          light_flicker__name : string)
    if (light_flicker__name == parent_template)
      parentEid = eid
      return true
    return false

  if parentEid == INVALID_ENTITY_ID
    parentEid = createEntitySync(parent_template) <| $(var init : ComponentsInitializer)
      set(init, "light_flicker__name", parent_template)

  // Light flicker entities are used to flicking the light and its mesh
  // We need to collect only lights into this component
  if has(flicking_eid, "light__brightness")
    query(parentEid) <| $ [es] (var light_flicker__lightEids : EidList)
      push(light_flicker__lightEids, flicking_eid)

  return parentEid

[es(on_appear, track=light_flicker_updater__template)]
def light_flicker_updater_change_parent(evt : Event;
                                        eid : EntityId;
                                        light_flicker_updater__template : string;
                                        var light_flicker_updater__parentEid : EntityId&)
  light_flicker_updater__parentEid = find_or_create_parent(light_flicker_updater__template, eid)

[es(on_appear)]
def light_flicker_updater_max_brightness(evt : Event;
                                         light__brightness : float;
                                         var light_flicker_updater__color_full_brightness : float4&)
  light_flicker_updater__color_full_brightness = float4(1.0, 1.0, 1.0, light__brightness)


[es(on_appear)]
def light_flicker_updater_max_emission(evt : Event;
                                       var animchar_render : AnimcharRendComponent;
                                       var light_flicker_updater__color_full_brightness : float4&)
  let emissive_color_var = get_shader_variable_id("emissive_color", false)
  let parameters_channel_var = get_shader_variable_id("parameters_channel", false)


  var emissive_color = Color4(float4(0.0))
  recreate_material(animchar_render) <| $(mat)
    mat |> set_int_param(parameters_channel_var, 0)
  *animchar_render.sceneInstance.lodsResource |> scene_lods_gather_mats(0) <| $(used_mat : array<ShaderMaterial?>#)
    for mat in used_mat
      if getColor4Variable(*mat, emissive_color_var, emissive_color)
        break

  light_flicker_updater__color_full_brightness = float4(emissive_color)

[es(tag=render, after=light_flicker_update_es)]
def light_flicker_updater__light_update_es(info : UpdateStageInfoAct;
                                           light_flicker_updater__parentEid : EntityId;
                                           light_flicker_updater__color_full_brightness : float4;
                                           var light__brightness : float&;
                                           light__visible = true)
  if (!light__visible || light_flicker_updater__parentEid == INVALID_ENTITY_ID)
    return

  light__brightness = get_float(light_flicker_updater__parentEid, "light_flicker__brightness") ?? 0.0
  light__brightness *= light_flicker_updater__color_full_brightness.w


[es(tag=render, after=light_flicker_update_es)]
def light_flicker_updater__emission_update_es(info : UpdateStageInfoAct;
                                              animchar__visible : bool;
                                              light_flicker_updater__parentEid : EntityId;
                                              light_flicker_updater__color_full_brightness : float4;
                                              var dynamic_material_channels_arr : Array)
  if (light_flicker_updater__parentEid == INVALID_ENTITY_ID)
    return

  if (!animchar__visible)
    return

  let brightness = get_float(light_flicker_updater__parentEid, "light_flicker__brightness") ?? 0.0

  var matParams = getRW_ecs_object(dynamic_material_channels_arr[0])
  if matParams != null
    var color = light_flicker_updater__color_full_brightness
    color.w *= brightness
    set(*matParams, "dynmat_param__emissive_color", color)


[es(tag=render, track=light_flicker__state)]
def light_flicker_spawn_sparks_fx_on_attack_state(evt : Event;
                                                  light_flicker__sparksEffectTemplate : string;
                                                  light_flicker__sparksEffectSpawnChance : float;
                                                  light_flicker__lightEids : EidList;
                                                  light_flicker__state : int)
  if light_flicker__state != int(LightFlickerState.Attack)
    return

  for lightEid in light_flicker__lightEids
    if rnd_float(0.0, 1.0) < light_flicker__sparksEffectSpawnChance
      query(lightEid) <| $ [es] (transform : float3x4)
        createEntity(light_flicker__sparksEffectTemplate) <| $(var init)
          let tm = calc_effect_tm(transform[3], transform[2])
          set(init, "transform", tm)
