require ecs
require DagorMath
require Dacoll
require DagorShaders
require app
require DngWeapon
require DngHuman
require Gun
require math.base
require DngCamera
require player
require danetlibs.renderer.includes.render_events

//to be moved to daNetGameLibs
[cpp_event(broadcast)]
struct ParallelUpdateFrameDelayed
  curTime : float
  dt : float

[cpp_event(unicast)]
struct QueryHeroWtmAndBoxForRender
  _paddingPre : uint8[8] // sizeof(ecs::Event)==8 and alignof(resWtm)==16, so we add 8 bytes padding here
  resWtmC0, resWtmC1, resWtmC2, resWtmC3 : float4 // mat44f resWtm
  resWofs : float4                                // vec3f  resWofs
  resLboxMin, resLboxMax : float4                 // bbox3f resLbox
  resReady : bool
  resFlags : uint8
  _paddingMid : uint8[14] // alignof(HeroWtmAndBox)==16, so we add 14 bytes padding here
  onlyWeapons : bool
  _paddingPost : uint8[15] // alignof(QueryHeroWtmAndBoxForRender)==16, so we add 15 bytes padding here

def get_hero_cockpit_vec_impl(hero_cockpit__gunMaxLength, hero_cockpit__traceMaxBias,
                              hero_cockpit__startFadeBeforeIntersection, hero_cockpit__fadeInterval,
                              hero_cockpit__sampleOffsetAboveCamera, hero_cockpit__sampleBias : float;
                              var hero_cockpit_vec_value : float4&;
                              var hero_cockpit_camera_to_point_value : float4&;
                              var hero_cockpit__intersected : bool&)
  let heroEid = get_Eid(get_local_player_eid(), "possessed") ?? INVALID_ENTITY_ID
  if !heroEid || getEntityTemplateId(heroEid) == INVALID_TEMPLATE_INDEX
    return false

  var shootTm : float3x4

  var gun_data : QueryHeroWtmAndBoxForRender
  gun_data.resReady = false
  gun_data.onlyWeapons = true
  sendEventImmediate(heroEid, gun_data)
  if !gun_data.resReady
    return false

  var humanTraceHandle : TraceMeshFaces?
  var isAiming, inVehicle : bool
  query(heroEid) <| $ [es] (var human_net_phys : HumanActor;
                            human_net_phys__isAiming : bool;
                            isInVehicle : bool)
    humanTraceHandle = human_net_phys.phys |> human_phys_getTraceHandle()
    isAiming = human_net_phys__isAiming
    inVehicle = isInVehicle

  if inVehicle
    return false

  shootTm[0] = -gun_data.resWtmC2.xyz
  shootTm[1] = gun_data.resWtmC1.xyz
  shootTm[2] = gun_data.resWtmC0.xyz
  shootTm[3] = gun_data.resWtmC3.xyz + gun_data.resWofs.xyz

  let back = shootTm * float3(0, 0, -1)
  let front = shootTm * float3(0, 0, 1)
  let sampleDir = -normalize(front - back).zx * hero_cockpit__sampleBias
  let cameraTracePos = float4(sampleDir.x, hero_cockpit__sampleOffsetAboveCamera, sampleDir.y, 0.0)
  hero_cockpit_camera_to_point_value = cameraTracePos

  let weaponDir = cross(-shootTm[0], shootTm[2])
  let cameraTm = get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM
  let startBias = clamp(dot(shootTm[3] - cameraTm[3], weaponDir),
                        -hero_cockpit__traceMaxBias, hero_cockpit__traceMaxBias)
  let tracePos = shootTm[3] - startBias * weaponDir
  var t : float = startBias + hero_cockpit__gunMaxLength
  var norm = float3()
  var pmid : int
  var riDesc = RendInstDesc()

  var startFadeDistance = 1000.0
  var applyFade = false
  let traceDir = normalize(weaponDir)
  if traceray_normalized(tracePos, traceDir, t, pmid, norm, ETF_DEFAULT | ETF_RI_TREES, riDesc, -1, humanTraceHandle)
    applyFade = true
    startFadeDistance = t - hero_cockpit__startFadeBeforeIntersection

  if isAiming
    query(get_human_gun_eid(heroEid))  <| $ [es] (gun__distanceToDisableAOinAiming : float)
      if gun__distanceToDisableAOinAiming > 0.0
        applyFade = true
        startFadeDistance = min(startFadeDistance, gun__distanceToDisableAOinAiming + startBias)
  hero_cockpit__intersected = applyFade
  if !applyFade
    return false
  let startFadePos = tracePos + startFadeDistance * weaponDir
  let normalizedDir = weaponDir / hero_cockpit__fadeInterval
  hero_cockpit_vec_value = float4(normalizedDir, -dot(normalizedDir, startFadePos) + 1.0)
  return true

[es(tag=render, after=animchar_before_render_es)]
def get_hero_cockpit_params(event : ParallelUpdateFrameDelayed;
                            hero_cockpit__gunMaxLength, hero_cockpit__traceMaxBias,
                            hero_cockpit__startFadeBeforeIntersection, hero_cockpit__fadeInterval,
                            hero_cockpit__sampleOffsetAboveCamera, hero_cockpit__sampleBias : float;
                            var hero_cockpit_vec_value : float4&;
                            var hero_cockpit_camera_to_point_value : float4&;
                            var hero_cockpit__intersected : bool&)
  if !get_hero_cockpit_vec_impl(hero_cockpit__gunMaxLength, hero_cockpit__traceMaxBias,
                                hero_cockpit__startFadeBeforeIntersection, hero_cockpit__fadeInterval,
                                hero_cockpit__sampleOffsetAboveCamera, hero_cockpit__sampleBias,
                                hero_cockpit_vec_value, hero_cockpit_camera_to_point_value, hero_cockpit__intersected)
    hero_cockpit_vec_value = float4(0.0, 0.0, 0.0, 0.0)

[es(tag=render,  after=animchar_before_render_es)] //require for execute animchar_before_render_es as early as possible
def set_hero_cockpit_params(info : UpdateStageInfoBeforeRender;
                            hero_cockpit_vec : ShaderVar;
                            hero_cockpit_vec_value : float4;
                            hero_cockpit_camera_to_point : ShaderVar;
                            hero_cockpit_camera_to_point_value : float4)
  DagorShaders::set_color4(hero_cockpit_vec.varId, hero_cockpit_vec_value)
  DagorShaders::set_color4(hero_cockpit_camera_to_point.varId, hero_cockpit_camera_to_point_value)