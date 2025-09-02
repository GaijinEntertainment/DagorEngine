require ecs
require ecs.common
require ecs.soa_template
require DngCamera
require DngPhysObj
require math.base
require AnimV20
require GeomNodeTree
require fx
require strings

[soa_template]
struct JetFlameFxEmitter
  jetflame_fx_emitter__nodeId : int
  jetflame_fx_emitter__main : bool
  jetflame_fx_emitter__fxEid : EntityId


[soa_es, es(tag=render, on_appear)]
def jetflame_fx_init_es(evt : Event;
                        animchar : AnimcharBaseComponent;
                        jetflame_fx__emitterNodes : StringList;
                        var JetFlameFxEmitter : JetFlameFxEmitter_SOA)
  for node in jetflame_fx__emitterNodes
    peek(node) <| $(nodeName)
      let main = character_at(nodeName, 0) == '*' ? true : false
      let nodeId = *animchar.nodeTree |> geomtree_findNodeIndex(main ? slice(nodeName, 1) : nodeName)
      if nodeId < 0
        return

      push(JetFlameFxEmitter, JetFlameFxEmitter(
        jetflame_fx_emitter__nodeId = nodeId,
        jetflame_fx_emitter__main = main,
        jetflame_fx_emitter__fxEid = INVALID_ENTITY_ID
      ))


[soa_def]
def stop_jetflame_fxs(var JetFlameFxEmitter : JetFlameFxEmitter_SOA; var jetflame_fx__haveActiveEmitters : bool&)
  for fx in JetFlameFxEmitter
    destroyEntity(fx.jetflame_fx_emitter__fxEid)
    fx.jetflame_fx_emitter__fxEid = INVALID_ENTITY_ID
  jetflame_fx__haveActiveEmitters = false


[soa_es, es(tag=render, on_disappear)]
def jetflame_fx_on_disappear_es(evt : Event;
                                var JetFlameFxEmitter : JetFlameFxEmitter_SOA;
                                var jetflame_fx__haveActiveEmitters : bool&)
  stop_jetflame_fxs(JetFlameFxEmitter, jetflame_fx__haveActiveEmitters)


def calc_effect_tm(animchar : AnimcharBaseComponent; node_id : int)
  var effTm = IDENT_TM
  geomtree_getNodeWtmScalar(*animchar.nodeTree, node_id, effTm)
  effTm[0] = -effTm[0]
  return effTm


[soa_es, es(tag=render, after=animchar_es, before=effect_es)]
def jetflame_fx_control_effects_es(evt : UpdateStageInfoAct;
                                   transform : float3x4;
                                   animchar : AnimcharBaseComponent;
                                   animchar__visible : bool;
                                   jetflame_main_fx__fx : string = "";
                                   jetflame_agile_fx__fx : string = "";
                                   jetflame_fx__updateDistSq : float = square(400.0f);
                                   var JetFlameFxEmitter : JetFlameFxEmitter_SOA;
                                   var jetflame_fx__haveActiveEmitters : bool&)
  if !animchar__visible
    let cameraTm = get_cur_cam_entity() |> get_TMatrix("transform") ?? IDENT_TM
    let sqDistToCam = distance_sq(cameraTm[3], transform[3])
    if sqDistToCam >= jetflame_fx__updateDistSq
      if jetflame_fx__haveActiveEmitters
        stop_jetflame_fxs(JetFlameFxEmitter, jetflame_fx__haveActiveEmitters)
      return

  for eff in JetFlameFxEmitter
    if !eff.jetflame_fx_emitter__fxEid
      let effTm = calc_effect_tm(animchar, eff.jetflame_fx_emitter__nodeId)
      let effectTemplate = eff.jetflame_fx_emitter__main ? jetflame_main_fx__fx : jetflame_agile_fx__fx
      eff.jetflame_fx_emitter__fxEid = createEntity(effectTemplate) <| $(var init)
        set(init, "transform", effTm)
      jetflame_fx__haveActiveEmitters = true


[soa_es, es(tag=render, after=animchar_es, before=effect_es)]
def jetflame_fx_update_effects_es(evt : UpdateStageInfoAct;
                                  phys_obj_net_phys : PhysObjActor;
                                  animchar : AnimcharBaseComponent;
                                  csu_engine_mainA__thrust : float;
                                  csu_engine_mainD__thrust : float;
                                  csu_engine_agileU__thrust : float3;
                                  csu_engine_agileD__thrust : float3;
                                  csu_engine_agileL__thrust : float3;
                                  csu_engine_agileR__thrust : float3;
                                  var JetFlameFxEmitter : JetFlameFxEmitter_SOA)
  // split actual thrust of main/agile engines into 26 actual nozzles
  var thrust : float[26 + 1]
  let main_thres = 3.f
  let agile_thres = 3.f
  if csu_engine_mainA__thrust > +main_thres
    let main_f = csu_engine_mainA__thrust > +100.f ? 0.4f : 0.0f
    let aux_f = csu_engine_mainA__thrust > +100.f ? 0.05f : 0.25f
    thrust[5] += csu_engine_mainA__thrust * main_f
    thrust[6] += csu_engine_mainA__thrust * main_f
    thrust[16] += csu_engine_mainA__thrust * aux_f
    thrust[17] += csu_engine_mainA__thrust * aux_f
    thrust[18] += csu_engine_mainA__thrust * aux_f
    thrust[19] += csu_engine_mainA__thrust * aux_f
  if csu_engine_mainD__thrust < -main_thres
    thrust[20] += -csu_engine_mainD__thrust * 0.25f
    thrust[21] += -csu_engine_mainD__thrust * 0.25f
    thrust[22] += -csu_engine_mainD__thrust * 0.25f
    thrust[23] += -csu_engine_mainD__thrust * 0.25f

  if csu_engine_agileU__thrust.y < -agile_thres
    thrust[3] += -csu_engine_agileU__thrust.y * 0.5f
    thrust[4] += -csu_engine_agileU__thrust.y * 0.5f
  if csu_engine_agileU__thrust.x > +agile_thres
    thrust[16] += csu_engine_agileU__thrust.x * 0.5f
    thrust[17] += csu_engine_agileU__thrust.x * 0.5f
  elif csu_engine_agileU__thrust.x < -agile_thres
    thrust[22] += -csu_engine_agileU__thrust.x * 0.5f
    thrust[23] += -csu_engine_agileU__thrust.x * 0.5f

  if csu_engine_agileD__thrust.y > +agile_thres
    thrust[12] += csu_engine_agileD__thrust.y * 0.25f
    thrust[13] += csu_engine_agileD__thrust.y * 0.25f
    thrust[14] += csu_engine_agileD__thrust.y * 0.25f
    thrust[15] += csu_engine_agileD__thrust.y * 0.25f
  if csu_engine_agileD__thrust.x > +agile_thres
    thrust[18] += csu_engine_agileD__thrust.x * 0.5f
    thrust[19] += csu_engine_agileD__thrust.x * 0.5f
  elif csu_engine_agileD__thrust.x < -agile_thres
    thrust[20] += -csu_engine_agileD__thrust.x * 0.5f
    thrust[21] += -csu_engine_agileD__thrust.x * 0.5f

  if csu_engine_agileL__thrust.z > +agile_thres
    thrust[1] += csu_engine_agileL__thrust.z * 0.5f
    thrust[26] += csu_engine_agileL__thrust.z * 0.5f
  if csu_engine_agileL__thrust.x > +agile_thres
    thrust[20] += csu_engine_agileL__thrust.x * 0.5f
    thrust[22] += csu_engine_agileL__thrust.x * 0.5f
  elif csu_engine_agileL__thrust.x < -agile_thres
    thrust[16] += -csu_engine_agileL__thrust.x * 0.5f
    thrust[18] += -csu_engine_agileL__thrust.x * 0.5f
  if csu_engine_agileL__thrust.y > +agile_thres
    thrust[4] += csu_engine_agileL__thrust.y
  elif csu_engine_agileL__thrust.y < -agile_thres
    thrust[12] += -csu_engine_agileL__thrust.y * 0.5f
    thrust[14] += -csu_engine_agileL__thrust.y * 0.5f

  if csu_engine_agileR__thrust.z < -agile_thres
    thrust[2] += -csu_engine_agileR__thrust.z * 0.5f
    thrust[9] += -csu_engine_agileR__thrust.z * 0.5f
  if csu_engine_agileR__thrust.x > +agile_thres
    thrust[21] += csu_engine_agileR__thrust.x * 0.5f
    thrust[23] += csu_engine_agileR__thrust.x * 0.5f
  elif csu_engine_agileR__thrust.x < -agile_thres
    thrust[17] += -csu_engine_agileR__thrust.x * 0.5f
    thrust[19] += -csu_engine_agileR__thrust.x * 0.5f
  if csu_engine_agileR__thrust.y > +agile_thres
    thrust[3] += csu_engine_agileR__thrust.y
  elif csu_engine_agileR__thrust.y < -agile_thres
    thrust[13] += -csu_engine_agileR__thrust.y * 0.5f
    thrust[15] += -csu_engine_agileR__thrust.y * 0.5f

  // setup FX for all nozzles (active and idle)
  var ord : int = 1
  for eff in JetFlameFxEmitter
    query(eff.jetflame_fx_emitter__fxEid) <| $ [es] (var transform : float3x4&;
                                                     var effect__velocity : float3&;
                                                     var effect__spawnRate : float&;
                                                     var effect__scale : float&)
      transform = calc_effect_tm(animchar, eff.jetflame_fx_emitter__nodeId)

      let velocity = phys_obj_net_phys.phys.currentState.velocity
      let omega = phys_obj_net_phys.phys.currentState.omega
      let localVelocity = rotate(transform, (cross(omega, (inverse(transform) * transform[3]))))

      effect__velocity = velocity + localVelocity
      effect__scale = saturate(thrust[ord] * 0.001f)  // scale influences lighting intensity
      effect__spawnRate = saturate(thrust[ord] * 0.1f)// spawn rate influences visible jet
      ord ++
