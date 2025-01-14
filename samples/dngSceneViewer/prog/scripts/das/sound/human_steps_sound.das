require ecs
require ecs.safe
require soundHash
require soundSystem
require soundEvent
require sound_utils.modules.sound_player_common
require sound_utils_net.modules.sound_utils_net_events
require sound_utils.modules.sound_physmat_common
require math.base
require DagorMath
require PhysMat
require Dacoll
require common_sounds.modules.common_sounds_events
require das.character_controller.charctrl_state

let INVALID_IRQ_OBJ_IDX = -1

def is_faraway_step(is_watched : bool; pos : float3; max_distance_sq : float; listener_pos : float3) : bool
  return !is_watched && distance_sq(listener_pos, pos) > max_distance_sq

def read_state(state_flag : CCStateFlag; states : int) : bool
  return (states & int(state_flag)) != 0

def pass_steps_occlusion_var(handle; occlusion : float)
  set_var_optional(handle, "occlusion", occlusion)

def private raise_pos_above_collision(pos : float3) // need pos above collision to make proper occlusion tests
  return pos + float3(0., 0.5, 0.) // TODO: find better, correct offsets, e.g. from pelvis

def get_desc_id_from_human_state(move_state : CCMoveType; stand_state : CCStandType; irqs : Array) : int
  let name = ((stand_state == CCStandType ESS_CROUCH) ? "step" :
              (stand_state == CCStandType ESS_CRAWL) ? "crawl" :
              (move_state == CCMoveType EMS_SPRINT) ? "sprint" :
              (move_state == CCMoveType EMS_RUN) ? "run" :
              "step")

  for it, idx in irqs, range(length(irqs))
    if ((it as Object)?.name ?? "") == name
      return idx

  sound_debug("desc with name {name} not in irqs")
  return INVALID_IRQ_OBJ_IDX

def private is_on_lmesh(pos : float3; stand_state : CCStandType)
  let posXZ = pos.xz
  let ground = traceht_lmesh(posXZ)
  let threshold = stand_state == CCStandType ESS_CRAWL ? 0.2 :  0.05
  return pos.y < ground + threshold


def human_steps_sound_generate(cur_time : float;
                               dt : float;
                               is_on_ground : bool;
                               is_climbing : bool;
                               move_state : CCMoveType;
                               stand_state : CCStandType;
                               move_speed : float;
                               var lastStepAt_cooldown : float2&;
                               [[shared_comp]] irqs : Array) : int

  assume last_step_at = lastStepAt_cooldown.x
  assume cooldown = lastStepAt_cooldown.y

  let useGenerator = (move_state != CCMoveType EMS_STAND) && move_speed > VERY_SMALL_NUMBER

  if move_state == CCMoveType EMS_STAND || !is_on_ground || is_climbing
    cooldown = min(cooldown + dt, 1.)
    return INVALID_IRQ_OBJ_IDX

  cooldown = max(0., cooldown - dt)

  if useGenerator
    let intervals = float3(1.15, 1.0, 1.6)
    let interval = safediv((move_state == CCMoveType EMS_SPRINT) ? intervals.z : (move_state == CCMoveType EMS_RUN) ? intervals.y : intervals.x, move_speed)
    cooldown = min(cooldown, interval * 0.5)

    if cur_time >= last_step_at + interval
      last_step_at = cur_time
      if cooldown != 0.
        return INVALID_IRQ_OBJ_IDX
      return get_desc_id_from_human_state(move_state, stand_state, irqs)

  return INVALID_IRQ_OBJ_IDX


def trace_footstep(pos : float3;
                   trace_offset_len : float2;
                   stand_state : CCStandType;
                   biome_pmid : int;
                   biome_overrides_water : bool;
                   puddle_depth : float;
                   var step_material_id : float&;
                   var step_water_depth : float&)

  var pmid = PHYSMAT_INVALID

  let from = float3(pos.x, pos.y + trace_offset_len.x, pos.z)
  var len = trace_offset_len.y
  if !tracedown_normalized(from, len, pmid, ETF_DEFAULT)
    len = trace_offset_len.y
    pmid = PHYSMAT_INVALID

  step_water_depth = 0.
  var water = 0.
  if traceht_water(from, water) && pos.y < water
    let posXZ = pos.xz
    let ground = traceht_lmesh(posXZ)
    let floor = max(from.y - len, ground)
    step_water_depth = max(0., water - floor)

  let isOnLMesh = is_on_lmesh(pos, stand_state)
  let waterDepth = isOnLMesh ? max(step_water_depth, puddle_depth) : step_water_depth
  let isInWater = waterDepth > 0.
  if isOnLMesh && biome_pmid != PHYSMAT_INVALID && (!isInWater || biome_overrides_water)
    pmid = biome_pmid
  elif isInWater
    pmid = get_material_id("water")

  if pmid == PHYSMAT_INVALID
    pmid = get_material_id("default")
  if pmid != PHYSMAT_INVALID
    step_material_id = get_sound_step_material_id(pmid)


def play_footstep(desc : Object;
                  path : Object;
                  sound_tags : Object;
                  volume : float;
                  occlusion : float;

                  pos : float3;
                  trace_offset_len : float2;
                  biome_pmid : int;
                  biome_overrides_water : bool;
                  puddle_depth : float;
                  stand_state : CCStandType;
                  var step_material_id : float&;
                  var step_water_depth : float&)

  trace_footstep(pos, trace_offset_len, stand_state, biome_pmid, biome_overrides_water, puddle_depth, step_material_id, step_water_depth)

  if step_material_id >= 0.
    var pathStr : string
    if sound_player_common::get_option_path(path, sound_tags, true, pathStr)
      let offsetedPos = raise_pos_above_collision(pos)
      var handle = sound_player_common::play_desc_path(desc, apply_watched_prefix(pathStr, true), true, offsetedPos, false)
      set_volume(handle, volume)
      set_var(handle, "mat", step_material_id)
      pass_steps_occlusion_var(handle, occlusion)
      abandon(handle)


def human_steps_sound_update_states(cur_time : float;
                                    state_flg : int;
                                    var human_steps_sound__stateFlags : int&;
                                    var human_steps_sound__lastTimeOnGround : float&) : string
  if human_steps_sound__stateFlags < 0
    human_steps_sound__stateFlags = state_flg
  let raisedStates = state_flg & ~human_steps_sound__stateFlags
  let clearedStates = human_steps_sound__stateFlags & ~state_flg
  human_steps_sound__stateFlags = state_flg

  if (raisedStates | clearedStates) != 0
    if read_state(CCStateFlag ST_ON_GROUND, clearedStates)
      human_steps_sound__lastTimeOnGround = cur_time

    if read_state(CCStateFlag ST_JUMP, raisedStates)
      return "jump"
    elif read_state(CCStateFlag ST_ON_GROUND, raisedStates)
      return "land"
    elif read_state(CCStateFlag ST_SWIM, raisedStates)
      if !read_state(CCStateFlag ST_ON_GROUND, state_flg)
        return "land"
    elif read_state(CCStateFlag ST_CROUCH, raisedStates)
      return "downKnee"
    elif read_state(CCStateFlag ST_CROUCH, clearedStates)
      return "upKnee"
    elif read_state(CCStateFlag ST_CRAWL, raisedStates)
      return "downGround"
    elif read_state(CCStateFlag ST_CRAWL, clearedStates)
      return "upGround"
  return ""


[es(tag=sound, after=(sound_landmesh_queries_update, sound_begin_update_es), before=sound_end_update_es)]
def human_steps_sound_update(info : ParallelUpdateFrameDelayed)

  let listenerPos = get_listener_pos()
  let is_watched_sound = true

  query() <| $ [es(REQUIRE_NOT=deadEntity)] (eid : EntityId;
                                             sound_control__soundEnabled : bool = true;

                                             transform : float3x4;
                                             phys__vel : float3;
                                             charctrl__state : int3;

                                             [[shared_comp]] human_steps_sound__irqs : Array;

                                             var human_steps_sound__stateFlags : int&;
                                             var human_steps_sound__lastTimeOnGround : float&;

                                             var sound_landmesh_queries_vars__speedSq : float&;
                                             var sound_landmesh_queries_vars__isOnGround : bool&;

                                             var human_steps_sound_generator__lastStepAt_cooldown : float2&)

    if !sound_control__soundEnabled || is_faraway_step(is_watched_sound, transform[3], square(50.f), listenerPos)
      sound_landmesh_queries_vars__speedSq = 0.
      sound_landmesh_queries_vars__isOnGround = true
      human_steps_sound__stateFlags = charctrl__state.z
      return

    let standState = unsafe(reinterpret<CCStandType>(charctrl__state.x))
    let moveState  = unsafe(reinterpret<CCMoveType>(charctrl__state.y))
    let animSpeed  = length(phys__vel.xz)

    let irqObjIdx = human_steps_sound_generate(info.curTime,
                                               info.dt,
                                               read_state(CCStateFlag ST_ON_GROUND, charctrl__state.z),
                                               /*climbing*/false,
                                               moveState,
                                               standState,
                                               animSpeed,
                                               human_steps_sound_generator__lastStepAt_cooldown,
                                               human_steps_sound__irqs)

    sound_landmesh_queries_vars__speedSq = length_sq(phys__vel.xz)
    sound_landmesh_queries_vars__isOnGround = read_state(CCStateFlag ST_ON_GROUND, charctrl__state.z)

    let descId = human_steps_sound_update_states(info.curTime,
                                                 charctrl__state.z,
                                                 human_steps_sound__stateFlags,
                                                 human_steps_sound__lastTimeOnGround)

    if irqObjIdx == INVALID_IRQ_OBJ_IDX && empty(descId)
      return

    query(eid) <| $ [es] (sound_tags : Object;
                          var sound_event_group : SoundEventGroup&;
                          human_steps_sound__traceStepOffsetLen : float2;
                          occluded_sound__occlusion = 0.;

                          [[shared_comp]] human_steps_sound__path : Object;
                          [[shared_comp]] human_steps_sound__descs : Object;
                          [[shared_comp]] human_steps_sound__irqs : Array;
                          [[shared_comp]] human_steps_sound__stepFx : Object;

                          var human_steps_sound__waterDepth : float&;
                          var human_steps_sound__smid : float&;
                          var human_steps_sound__stepIdx : uint&;

                          sound_landmesh_queries__biomePhysMatId : int;
                          sound_landmesh_queries__biomeOverridesWater : bool;
                          sound_landmesh_queries__puddleDepth : float)

      var desc : Object const? = null
      if !empty(descId)
        desc = sound_player_common::get_desc(human_steps_sound__descs, descId)
      elif irqObjIdx != INVALID_IRQ_OBJ_IDX
        desc = human_steps_sound__irqs?[irqObjIdx] ?as Object
        ++human_steps_sound__stepIdx

        if (human_steps_sound__stepIdx & 3u) == 0u && desc != null
          let stepFxPath = human_steps_sound__stepFx[desc.name ?? ""] ?as Object
          if stepFxPath != null
            let handle = sound_player_common::play_path(*stepFxPath, sound_tags, is_watched_sound, transform[3], sound_hash(""), sound_event_group)
            pass_steps_occlusion_var(handle, occluded_sound__occlusion)

      if desc != null
        let volume = 1.
        play_footstep(*desc,
                      human_steps_sound__path,
                      sound_tags,
                      volume,
                      occluded_sound__occlusion,
                      transform[3],
                      human_steps_sound__traceStepOffsetLen,
                      sound_landmesh_queries__biomePhysMatId,
                      sound_landmesh_queries__biomeOverridesWater,
                      sound_landmesh_queries__puddleDepth,
                      standState,
                      human_steps_sound__smid,
                      human_steps_sound__waterDepth)


[es(tag=sound, on_appear)]
def human_steps_sound_on_appear(evt : Event;
                                human_steps_sound__tag : string;
                                var sound_flags : Object&)
  raise_sound_tag(human_steps_sound__tag, sound_flags)
