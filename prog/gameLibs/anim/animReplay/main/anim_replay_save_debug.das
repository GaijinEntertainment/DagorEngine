options no_aot
require ecs
require ecs.common
require math
require strings
require AnimV20
require DagorSystem
require %gameLibs/anim/animReplay/imports/anim_replay_events
require %gameLibs/anim/animReplay/imports/anim_replay_anim_state_params
require %gameLibs/anim/animReplay/imports/anim_replay_create_animchars
require %gameLibs/anim/animReplay/imports/anim_replay_common


[es]
def anim_replay_start_record(evt : AnimReplayStartRecord)
  createEntity("anim_replay_save_context") <| $(var init : ComponentsInitializer)
    set(init, "anim_replay__animcharsRecordingFilter", int(evt.animcharsFilter))
    set(init, "anim_replay__sampleRate", int(evt.sampleRate))

def fill_tracked_anim_params(var anim_params : Array; anim_graph : AnimationGraph)
  clear(anim_params)
  using() <| $(var emptyAnimParam : Object)
    using() <| $(var values : FloatList)
      set(emptyAnimParam, "values", values)
    anim_graph_getParamNames(anim_graph) <| $(params)
      for param in params
        let paramType = anim_graph_getParamType(anim_graph, param.id)
        if paramType != PT_ScalarParam
          continue
        //todo: need to save params for RandomSwitcher controller
        //todo: effectors?
        // maybe more?
        if character_at(param.name, 0) == ':' //internal params
          continue
        push(anim_params, emptyAnimParam)
        let lastParamIdx = length(anim_params) - 1
        assume animParam = *getRW_ecs_object(anim_params[lastParamIdx])
        set(animParam, "id", param.id)
        set(animParam, "name", string(param.name))

def fill_initial_animchar_data(var animchar_data : Object; eid : EntityId)
  query(eid) <| $ [es] (animchar : AnimcharBaseComponent;
                        animchar__res : string;
                        animchar__appliedAnimSpeed : float;
                        animchar__animState : Object)
    assume animGraph = *animchar.animGraph
    set(animchar_data, "eid", eid)
    set(animchar_data, "animcharRes", animchar__res)
    set(animchar_data, "originalTemplate", getEntityTemplateName(eid))
    using() <| $(var position : Point3List)
      set(animchar_data, "position", position)
    using() <| $(var rotation : Point4List)
      set(animchar_data, "rotation", rotation)
    using() <| $(var animTreeStates : Object)
      using() <| $(var timeList : FloatList)
        push(timeList, 0.f)
        set(animTreeStates, "time", timeList)
      using() <| $(var states : Array)
        push(states, animchar__animState)
        set(animTreeStates, "state", states)
      using() <| $(var animSpeedList : FloatList)
        push(animSpeedList, animchar__appliedAnimSpeed)
        set(animTreeStates, "animSpeed", animSpeedList)
      using() <| $(var stateNames : StringList)
        for i in range(animGraph.stateCount)
          let stateName = anim_graph_getStateNameByStateIdx(animGraph, i)
          if !empty(stateName) // alias states return empty string
            push(stateNames, stateName)
        set(animTreeStates, "stateNames", stateNames)
      set(animchar_data, "animTreeStates", animTreeStates)
    using() <| $(var initialAnimStateParams : Object)
      save_anim_state_parameters(animGraph, *animchar.animState, initialAnimStateParams)
      set(animchar_data, "initialAnimStateParams", initialAnimStateParams)
    using() <| $(var trackedAnimStateParams : Array)
      fill_tracked_anim_params(trackedAnimStateParams, animGraph)
      set(animchar_data, "animParams", trackedAnimStateParams)

[es(on_appear)]
def anim_replay_gather_animchars_for_save(evt : Event; anim_replay__animcharsRecordingFilter : int; var anim_replay__savedAnimchars : Array)
  clear(anim_replay__savedAnimchars)
  let addAnimcharToList = $ <| (eid : EntityId)
    using() <| $(var animcharData : Object)
      fill_initial_animchar_data(animcharData, eid)
      if !empty(animcharData)
        push(anim_replay__savedAnimchars, animcharData)

  if anim_replay__animcharsRecordingFilter == int(AnimcharsRecordingFilter WATCHED_BY_PLAYER)
    query() <| $ [es(REQUIRE=(watchedByPlr))] (eid : EntityId)
      addAnimcharToList(eid)
  elif anim_replay__animcharsRecordingFilter == int(AnimcharsRecordingFilter DAEDITOR_SELECTED)
    query() <| $ [es(REQUIRE=(daeditor__selected))] (eid : EntityId)
      addAnimcharToList(eid)
  elif anim_replay__animcharsRecordingFilter == int(AnimcharsRecordingFilter ALL_MM_CHARACTERS)
    query() <| $ [es(REQUIRE=(motion_matching__controller))] (eid : EntityId)
      addAnimcharToList(eid)

[es(before=anim_replay_end_of_saving_frame)]
def anim_replay_track_animchar(evt : UpdateStageInfoAct;
                               anim_replay__sampleRate : int;
                               anim_replay__time : float;
                               var anim_replay__timestamps : FloatList;
                               var anim_replay__savedAnimchars : Array)
  let prevTimestamp = length(anim_replay__timestamps) > 0 ? back(anim_replay__timestamps) : -1.f
  if (anim_replay__time - prevTimestamp) * float(anim_replay__sampleRate) < 1.0f
    return
  push(anim_replay__timestamps, anim_replay__time)

  for animcharChildComp in anim_replay__savedAnimchars
    assume animcharData = *getRW_ecs_object(animcharChildComp)
    if is_animchar_dead(animcharData, anim_replay__time)
      continue
    let animcharEid = *get_Eid(animcharData, "eid")
    var isDead = false
    query(animcharEid) <| $ [es] (animchar : AnimcharBaseComponent;
                                  transform : float3x4;
                                  deadEntity : Tag const?)
      isDead = deadEntity != null
      assume posList = *getRW_ecs_Point3List(animcharData, "position")
      assume rotList = *getRW_ecs_Point4List(animcharData, "rotation")
      push(posList, transform[3])
      push(rotList, math::quat(transform))
      assume animState = *animchar.animState
      assume animParams = *getRW_ecs_array(animcharData, "animParams")
      for animParamChildComp in animParams
        assume animParam = *getRW_ecs_object(animParamChildComp)
        let paramId = *get_int(animParam, "id")
        assume paramValues = *getRW_ecs_FloatList(animParam, "values")
        let paramValue = anim_state_holder_getParam(animState, paramId)
        push(paramValues, paramValue)
    if isDead || !doesEntityExist(animcharEid)
      set(animcharData, "deathTime", prevTimestamp)

[es(before=anim_replay_end_of_saving_frame)]
def anim_replay_track_anim_states(evt : UpdateStageInfoAct;
                                  anim_replay__time : float;
                                  var anim_replay__savedAnimchars : Array)
  for animchar in anim_replay__savedAnimchars
    assume animcharData = *getRW_ecs_object(animchar)
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (animchar__appliedAnimSpeed : float;
                                  animchar__animState : Object)
      assume animStates = *getRW_ecs_object(animcharData, "animTreeStates")
      assume timeList = *getRW_ecs_FloatList(animStates, "time")
      assume states = *getRW_ecs_array(animStates, "state")
      assume animSpeedList = *getRW_ecs_FloatList(animStates, "animSpeed")
      let prevAnimSpeed = back(animSpeedList)
      let lastIdx = length(states) - 1
      assume prevAnimState = *get_ecs_object(states[lastIdx])
      var isSameState = true
      for prevState, curState in prevAnimState, animchar__animState
        if *get_int(prevState.value) != *get_int(curState.value)
          isSameState = false
          break
      if !isSameState || prevAnimSpeed != animchar__appliedAnimSpeed
        push(timeList, anim_replay__time)
        push(states, animchar__animState)
        push(animSpeedList, animchar__appliedAnimSpeed)

// Should be before real updates (physics, animations and etc). In our case most of this
// happens in ParallelUpdateFrameDelayed, so any order in regular update is ok.
[es(no_order)]
def anim_replay_end_of_saving_frame(evt : UpdateStageInfoAct;
                                    var anim_replay__prevTime : float&;
                                    var anim_replay__time : float&)
  anim_replay__prevTime = anim_replay__time
  anim_replay__time += evt.dt

[es]
def anim_replay_stop_record(evt : AnimReplayStopRecord;
                            eid : EntityId;
                            anim_replay__savedAnimchars : Array;
                            anim_replay__timestamps : FloatList)
  createEntitySync("anim_replay_load_context") <| $(var init : ComponentsInitializer)
    set(init, "anim_replay__timestamps", anim_replay__timestamps)
    set(init, "anim_replay__loadingAnimchars", anim_replay__savedAnimchars)
  destroyEntity(eid)

[es(on_appear)]
def configure_anim_state_params(evt : Event; var anim_replay__animchars : Array; anim_replay__ignoredAnimParams : StringList)
  for animcharChildComp in anim_replay__animchars
    assume animcharData = *getRW_ecs_object(animcharChildComp)
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (animchar : AnimcharBaseComponent)
      assume animParams = *getRW_ecs_array(animcharData, "animParams")
      var paramsCount = length(animParams)
      var i = 0
      while i < paramsCount
        assume animParam = *getRW_ecs_object(animParams[i])
        let paramId = *get_int(animParam, "id")
        let paramName = anim_graph_getParamName(*animchar.animGraph, paramId)

        //todo: this should be done somehow based on some config with soldier anim params
        if paramName == "pers_course" || paramName == "walk_dir" || paramName == "body_dir"
          set(animParam, "cyclic", true)
          set(animParam, "min", -90f)
          set(animParam, "max", 270f)
        elif paramName == "pelvis_move_course"
          set(animParam, "cyclic", true)
          set(animParam, "min", -180f)
          set(animParam, "max", 180f)
        elif find_index(anim_replay__ignoredAnimParams, paramName) != -1
          erase(animParams, i)
          paramsCount--
          i--
        i++
