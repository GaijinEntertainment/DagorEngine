options no_aot
require ecs
require ecs.safe
require math
require AnimV20
require DagorSystem
require GeomNodeTree
require daslib/algorithm
require %gameLibs/anim/animReplay/imports/anim_replay_anim_state_params
require %gameLibs/anim/animReplay/imports/anim_replay_common


[es(no_order)]
def anim_replay_update_animchar(evt : UpdateStageInfoAct;
                                anim_replay__animchars : Array;
                                anim_replay__timestamps : FloatList;
                                anim_replay__currentTime : float)
  sample_timestamps(anim_replay__currentTime, anim_replay__timestamps) <| $(prevIdx; nextIdx; interpK)
    iterate_alive_replay_animchars(anim_replay__animchars, anim_replay__currentTime) <| $(animcharData)
      let animcharEid = *get_Eid(animcharData, "eid")
      let position & = unsafe(*get_ecs_Point3List(animcharData, "position"))
      let rotation & = unsafe(*get_ecs_Point4List(animcharData, "rotation"))
      let pos = lerp(position[prevIdx], position[nextIdx], interpK)
      let rot = quat_slerp(interpK, rotation[prevIdx], rotation[nextIdx])
      // NOTE: characters and weapons may be displaced for one frame because `animchar_copy_nodes`
      // (copies nodes that will be used in render) for characters is called during animchar_es in PUFD,
      // so they will be rendered only in next frame after PUFD. But it is not too easy to fix because:
      //  1. `animchar_copy_nodes` for animchar itself is useless, we need updated non-weapon attachments
      //  (clothes, hands, head and etc)
      //  2. All animations should be updated before this copy too, now most problematic place is
      //  `anim_replay_update_motion_matching_root` and `mm_calculate_root_offset_es`, they are also executed
      //  in PUFD, and even more tricky. Character position and rotation are updated immediately because
      //  replay time is updated in imgui (beforeRender stage), then in PUFD MM overwrite pos/rot with fresh data.
      //  But in `mm_calculate_root_offset_es` we also use MotionMatchingController::resultAnimation, which is not
      //  updated yet, so it will be outdated for one frame.
      query(animcharEid) <| $[es(REQUIRE=anim_replay_character)] (var transform : float3x4&)
        transform = make_tm(rot, pos)
      // attachments
      query(animcharEid) <| $[es(REQUIRE_NOT=anim_replay_character)] (var animchar : AnimcharBaseComponent;
                                                                      var animchar_node_wtm : AnimcharNodesMat44&;
                                                                      var animchar_render__root_pos : vec4f&)
        let tm = make_tm(rot, pos)
        animchar_set_tm(animchar, tm, true)
        geomtree_calcWtmForBranch(*animchar.nodeTree, 0)
        animchar_copy_nodes(animchar, animchar_node_wtm, animchar_render__root_pos)

def apply_tracked_anim_state_params(var anim_state : IAnimStateHolder; anim_params : Array; frame_idx : int)
  for animParamChildComp in anim_params
    assume animParam = *get_ecs_object(animParamChildComp)
    let paramId = *get_int(animParam, "id")
    assume paramValues = *get_ecs_FloatList(animParam, "values")
    let paramValue = paramValues[frame_idx]
    anim_state_holder_setParam(anim_state, paramId, paramValue)

def apply_interpolated_anim_state_params(var anim_state : IAnimStateHolder;
                                         anim_params : Array;
                                         prev_frame_idx : int;
                                         next_frame_idx : int;
                                         interpK : float)
  for animParamChildComp in anim_params
    assume animParam = *get_ecs_object(animParamChildComp)
    let paramId = *get_int(animParam, "id")
    assume paramValues = *get_ecs_FloatList(animParam, "values")
    var paramValue = 0f
    if get_bool(animParam, "cyclic") ?? false
      var diff = paramValues[next_frame_idx] - paramValues[prev_frame_idx]
      let min = *get_float(animParam, "min")
      let max = *get_float(animParam, "max")
      let cycleRange = max - min
      if abs(diff) > cycleRange * 0.5f
        diff = diff < 0f ? diff + cycleRange : diff - cycleRange
      paramValue = paramValues[prev_frame_idx] + diff * interpK
      if paramValue < min
        paramValue += cycleRange
      elif paramValue > max
        paramValue -= cycleRange
    else
      paramValue = lerp(paramValues[prev_frame_idx], paramValues[next_frame_idx], interpK)
    anim_state_holder_setParam(anim_state, paramId, paramValue)

def apply_precise_anim_state_params(var anim_state : IAnimStateHolder;
                                    precise_anim_params : Array;
                                    t : float)
  unsafe
    for animParamChildComp in precise_anim_params
      let animParam & = *get_ecs_object(animParamChildComp)
      let paramId = *get_int(animParam, "id")
      let values & = *get_ecs_FloatList(animParam, "values")
      let time & = *get_ecs_FloatList(animParam, "time")
      let paramValue = values[get_current_timestamp_idx(t, time)]
      anim_state_holder_setParam(anim_state, paramId, paramValue)

[es(before=anim_replay_update_anim_state)]
def anim_replay_update_attachment_slots(evt : UpdateStageInfoAct;
                                        anim_replay__animchars : Array;
                                        anim_replay__currentTime : float)
  iterate_alive_replay_animchars(anim_replay__animchars, anim_replay__currentTime) <| $(animchar_data)
    let slotTime = get_ecs_FloatList(animchar_data, "slotTime")
    if slotTime == null
      return
    let slotIdx & = unsafe(*get_ecs_IntList(animchar_data, "slotIdx"))
    let timeIdx = get_current_timestamp_idx(anim_replay__currentTime, slotTime)
    let currentSlotIdx = slotIdx[timeIdx]
    let attachmentEid = *get_Eid(animchar_data, "eid")
    query(attachmentEid) <| $[es] (var animchar aka attached_animchar : AnimcharBaseComponent;
                                   var animchar_render__enabled : bool&;
                                   anim_replay__parentEid : EntityId)
      query(anim_replay__parentEid) <| $[es] (var animchar aka parent_animchar : AnimcharBaseComponent)
        if currentSlotIdx >= 0
          animchar_setAttachedChar(parent_animchar, currentSlotIdx, uint(attachmentEid), attached_animchar, false)
          animchar_render__enabled = true
        else
          animchar_render__enabled = false

[es(no_order)]
def anim_replay_update_anim_state(evt : UpdateStageInfoAct;
                                  anim_replay__animchars : Array;
                                  anim_replay__timestamps : FloatList;
                                  anim_replay__prevTime : float;
                                  anim_replay__currentTime : float)
  if anim_replay__currentTime > 0.f && anim_replay__prevTime == anim_replay__currentTime
    return
  iterate_alive_replay_animchars(anim_replay__animchars, anim_replay__currentTime) <| $(animcharData)
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $[es] (var animchar : AnimcharBaseComponent;
                                 var animchar__animState : Object?)
      if animchar.animGraph == null || animchar.animState == null
        return
      let animStates = get_ecs_object(animcharData, "animTreeStates")
      assume animParams = *get_ecs_array(animcharData, "animParams")
      let timeList = get_ecs_FloatList(animStates, "time")
      let states = get_ecs_array(animStates, "state")
      let animSpeedList = get_ecs_FloatList(animStates, "animSpeed")
      let preciseAnimParams = get_ecs_array(animcharData, "preciseAnimParams")
      let curStateIdx = get_current_timestamp_idx(anim_replay__currentTime, timeList)
      if animchar__animState != null && states != null
        *animchar__animState := *get_ecs_object(states?[curStateIdx])

      assume animGraph = *animchar.animGraph
      assume animState = *animchar.animState

      var animStateParams = get_ecs_object(animcharData, "initialAnimStateParams")
      var useInitialState = true
      var t = 0f
      var paramsTimeIdx = 0
      let paramsLastIdx = max(lower_bound(anim_replay__timestamps, anim_replay__currentTime) - 1, 0)
      var stateTimeIdx = 0
      let stateLastIdx = curStateIdx
      let cache = get_ecs_array(animcharData, "cachedAnimStateParams")
      if cache != null
        for cachedStateChildComp in *cache
          let cachedState = get_ecs_object(cachedStateChildComp)
          let cacheTime = *get_float(*cachedState, "time")
          if cacheTime <= anim_replay__currentTime
            animStateParams = cachedState
            t = cacheTime
            paramsTimeIdx = max(lower_bound(anim_replay__timestamps, cacheTime) - 1, 0)
            stateTimeIdx = get_current_timestamp_idx(cacheTime, timeList)
            useInitialState = false
            break
      if anim_replay__prevTime < anim_replay__currentTime && anim_replay__prevTime >= t
        t = anim_replay__prevTime
        paramsTimeIdx = max(lower_bound(anim_replay__timestamps, t) - 1, 0)
        stateTimeIdx = get_current_timestamp_idx(t, timeList)
      else
        apply_anim_state_parameters(animState, *animStateParams)
      while paramsTimeIdx < paramsLastIdx || stateTimeIdx < stateLastIdx
        if preciseAnimParams != null
          apply_precise_anim_state_params(animState, *preciseAnimParams, t)
        let paramsTime = paramsTimeIdx < paramsLastIdx ? anim_replay__timestamps[paramsTimeIdx + 1] : anim_replay__currentTime
        let stateTime = timeList.safe_at(stateTimeIdx + 1, anim_replay__currentTime)
        if paramsTime < stateTime
          // NOTE: we need only `animState.advance()` + `animGraph.blend()` here and in following `animchar_act` calls.
          // Blend method is not binded to das, so it is simpler to call whole animchar update. It is also needed only to
          // rewind timers, because they depend on `rewind_bitmap`, which is updated only during `buildBlendingList()`
          animchar_act(animchar, paramsTime - t, true)
          apply_tracked_anim_state_params(animState, animParams, paramsTimeIdx + 1)
          paramsTimeIdx++
          t = paramsTime
        else
          animchar_act(animchar, stateTime - t, true)
          let animSpeed = animSpeedList.safe_at(stateTimeIdx + 1, -1f)
          let multiState = get_ecs_object(states?[stateTimeIdx + 1])
          if multiState != null
            for state in *multiState
              let stateIdx = *get_int(state.value)
              anim_graph_enqueueState(animGraph, animState, stateIdx, -1.0f, animSpeed)
          stateTimeIdx++
          t = stateTime
      animchar_act(animchar, anim_replay__currentTime - t, true)

      if paramsLastIdx < length(anim_replay__timestamps) - 1
        let interpK = ((anim_replay__currentTime - anim_replay__timestamps[paramsLastIdx]) /
          (anim_replay__timestamps[paramsLastIdx + 1] - anim_replay__timestamps[paramsLastIdx]))
        apply_interpolated_anim_state_params(animState, animParams, paramsLastIdx, paramsLastIdx + 1, interpK)
      if preciseAnimParams != null
        apply_precise_anim_state_params(animState, *preciseAnimParams, anim_replay__currentTime)

      // NOTE: It is a hack to recalculate post controllers with new anim state params.
      // The problem is that animGraph can be very dependent on dt. Updating it with interpolated
      // params and small dt can give different result than updating with sampled rate dt and params.
      // When rewind in forward direction it is not a problem because we can reuse previously calculated
      // state, but when rewind in backward direction we should recalculate state from some cached point
      // and it can be very computation heavy to recalculate it with small dt. So we will recalculate it
      // with sampled rate even if it gives not smooth result.
      //
      // Update with zero dt is needed to prevent one frame flickering when we render animchar with new
      // anim params and old data from post controllers. But it doesn't solve possible pose twitches
      // while rewinding because of recalculation with larger dt.
      //
      // TODO: maybe it worth to use small dt update for smooth backward rewind only on watched character.
      if anim_replay__prevTime > anim_replay__currentTime
        animchar_act(animchar, 0f, true)

def update_anim_state_cache(anim_graph : AnimationGraph; anim_state : IAnimStateHolder; current_time : float; var animchar_data : Object)
  let CACHE_TIME = fixed_array(1f, 5f, 30f)
  if get_ecs_array(animchar_data, "cachedAnimStateParams") == null
    using() <| $(var cache : Array)
      set(animchar_data, "cachedAnimStateParams", cache)
  assume cache = *getRW_ecs_array(animchar_data, "cachedAnimStateParams")
  using() <| $(var cachedState : Object)
    for i in iter_range(CACHE_TIME)
      if length(cache) <= i
        if i == 0
          save_anim_state_parameters(anim_graph, anim_state, cachedState)
          set(cachedState, "time", current_time)
        push(cache, cachedState)
        return
      let t = *get_float(*get_ecs_object(cache[i]), "time")
      if current_time >= t && current_time <= t + CACHE_TIME[i]
        return
      if i == 0
        save_anim_state_parameters(anim_graph, anim_state, cachedState)
        set(cachedState, "time", current_time)
      swap(cachedState, *getRW_ecs_object(cache[i]))

[es(after=anim_replay_update_anim_state)]
def anim_replay_cache_anim_state(evt : UpdateStageInfoAct;
                                 var anim_replay__animchars : Array;
                                 anim_replay__currentTime : float)
  if anim_replay__currentTime == 0f
    return
  iterate_replay_animchars(anim_replay__animchars) <| $(animchar_data; animchar_eid)
    query(animchar_eid) <| $[es] (animchar : AnimcharBaseComponent)
      if animchar.animGraph != null && animchar.animState != null
        update_anim_state_cache(*animchar.animGraph, *animchar.animState, anim_replay__currentTime, animchar_data)
