options no_aot
require ecs
require math
require AnimV20
require DagorSystem
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
      assume position = *get_ecs_Point3List(animcharData, "position")
      assume rotation = *get_ecs_Point4List(animcharData, "rotation")
      let pos = lerp(position[prevIdx], position[nextIdx], interpK)
      let rot = quat_slerp(interpK, rotation[prevIdx], rotation[nextIdx])
      query(animcharEid) <| $[es] (var transform : float3x4&)
        transform = make_tm(rot, pos)

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
                                 var animchar__animState : Object)
      assume animStates = *get_ecs_object(animcharData, "animTreeStates")
      assume animParams = *get_ecs_array(animcharData, "animParams")
      assume timeList = *get_ecs_FloatList(animStates, "time")
      assume states = *get_ecs_array(animStates, "state")
      assume animSpeedList = *get_ecs_FloatList(animStates, "animSpeed")
      let nextIdx = lower_bound(timeList, anim_replay__currentTime)
      let prevIdx = max(nextIdx - 1, 0)
      animchar__animState := *get_ecs_object(states[prevIdx])

      assume animGraph = *animchar.animGraph
      assume animState = *animchar.animState

      var animStateParams = get_ecs_object(animcharData, "initialAnimStateParams")
      var useInitialState = true
      var t = 0f
      var paramsTimeIdx = 0
      let paramsLastIdx = max(lower_bound(anim_replay__timestamps, anim_replay__currentTime) - 1, 0)
      var stateTimeIdx = 0
      let stateLastIdx = prevIdx
      let cache = get_ecs_array(animcharData, "cachedAnimStateParams")
      if cache != null
        for cachedStateChildComp in *cache
          let cachedState = get_ecs_object(cachedStateChildComp)
          let cacheTime = *get_float(*cachedState, "time")
          if cacheTime <= anim_replay__currentTime
            animStateParams = cachedState
            t = cacheTime
            paramsTimeIdx = max(lower_bound(anim_replay__timestamps, cacheTime) - 1, 0)
            stateTimeIdx = max(lower_bound(timeList, cacheTime) - 1, 0)
            useInitialState = false
            break
      if anim_replay__prevTime < anim_replay__currentTime && anim_replay__prevTime >= t
        t = anim_replay__prevTime
        paramsTimeIdx = max(lower_bound(anim_replay__timestamps, t) - 1, 0)
        stateTimeIdx = max(lower_bound(timeList, t) - 1, 0)
      else
        apply_anim_state_parameters(animState, *animStateParams)
      while paramsTimeIdx < paramsLastIdx || stateTimeIdx < stateLastIdx
        let paramsTime = paramsTimeIdx < paramsLastIdx ? anim_replay__timestamps[paramsTimeIdx + 1] : anim_replay__currentTime
        let stateTime = stateTimeIdx < stateLastIdx ? timeList[stateTimeIdx + 1] : anim_replay__currentTime
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
          let animSpeed = animSpeedList[stateTimeIdx + 1]
          for state in *get_ecs_object(states[stateTimeIdx + 1])
            let stateIdx = *get_int(state.value)
            anim_graph_enqueueState(animGraph, animState, stateIdx, -1.0f, animSpeed)
          stateTimeIdx++
          t = stateTime
      animchar_act(animchar, anim_replay__currentTime - t, true)

      if paramsLastIdx < length(anim_replay__timestamps) - 1
        let interpK = ((anim_replay__currentTime - anim_replay__timestamps[paramsLastIdx]) /
          (anim_replay__timestamps[paramsLastIdx + 1] - anim_replay__timestamps[paramsLastIdx]))
        apply_interpolated_anim_state_params(animState, animParams, paramsLastIdx, paramsLastIdx + 1, interpK)

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

def update_anim_state_cache(animchar_eid : EntityId; current_time : float; var animchar_data : Object)
  let CACHE_TIME = fixed_array(1f, 5f, 30f)
  if get_ecs_array(animchar_data, "cachedAnimStateParams") == null
    using() <| $(var cache : Array)
      set(animchar_data, "cachedAnimStateParams", cache)
  assume cache = *getRW_ecs_array(animchar_data, "cachedAnimStateParams")
  using() <| $(var cachedState : Object)
    for i in iter_range(CACHE_TIME)
      if length(cache) <= i
        if i == 0
          query(animchar_eid) <| $[es] (animchar : AnimcharBaseComponent)
            save_anim_state_parameters(*animchar.animGraph, *animchar.animState, cachedState)
          set(cachedState, "time", current_time)
        push(cache, cachedState)
        return
      let t = *get_float(*get_ecs_object(cache[i]), "time")
      if current_time >= t && current_time <= t + CACHE_TIME[i]
        return
      if i == 0
        query(animchar_eid) <| $[es] (animchar : AnimcharBaseComponent)
          save_anim_state_parameters(*animchar.animGraph, *animchar.animState, cachedState)
        set(cachedState, "time", current_time)
      swap(cachedState, *getRW_ecs_object(cache[i]))

[es(after=anim_replay_update_anim_state)]
def anim_replay_cache_anim_state(evt : UpdateStageInfoAct;
                                 var anim_replay__animchars : Array;
                                 anim_replay__currentTime : float)
  if anim_replay__currentTime == 0f
    return
  for animchar in anim_replay__animchars
    assume animcharData = *getRW_ecs_object(animchar)
    let animcharEid = *get_Eid(animcharData, "eid")
    update_anim_state_cache(animcharEid, anim_replay__currentTime, animcharData)
