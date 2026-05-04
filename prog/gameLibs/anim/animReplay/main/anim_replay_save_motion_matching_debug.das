options no_aot
require ecs
require ecs.safe
require daslib/algorithm
require strings
require AnimV20
require DagorConsole
require DagorSystem
require MotionMatching
require %gameLibs/anim/animReplay/imports/anim_replay_events
require %gameLibs/anim/animReplay/imports/anim_replay_common
require gameLibs.pufd_events.pufd_events


// Need them for correct timers synchronization between MM and animTree in replay
def prepare_anim_tree_timer_scale_param_for_save(motion_matching__controller : MotionMatchingController;
                                                 var animchar_data : Object)
  var timers : array<int>
  assume dataBase = motion_matching__controller.dataBase
  for clip in dataBase.clips
    if clip.animTreeTimer >= 0 && find_index(timers, clip.animTreeTimer) < 0
      push(timers, clip.animTreeTimer)
  var animParams & = unsafe(*getRW_ecs_array(animchar_data, "animParams"))
  let animcharEid = *get_Eid(animchar_data, "eid")
  query(animcharEid) <| $ [es] (animchar : AnimcharBaseComponent)
    for timerId in timers
      let timerScaleId = anim_state_holder_getTimeScaleParamId(*animchar.animState, timerId)
      if timerScaleId < 0
        continue
      var found = false
      for animParam in animParams
        if ((animParam as Object).id ?? -1) == timerScaleId
          found = true
          break
      if found
        continue
      using() <| $(var animParam : Object)
        using() <| $(var values : FloatList)
          set(animParam, "values", values)
        set(animParam, "id", timerScaleId)
        set(animParam, "name", anim_graph_getParamName(*animchar.animGraph, timerScaleId))
        push(animParams, animParam)

[es(on_appear, after=anim_replay_gather_animchars_for_save)]
def anim_replay_prepare_motion_matching_for_save(evt : Event;
                                                 eid : EntityId;
                                                 var anim_replay__savedAnimchars : Array)
  let templateInfo = getTemplateByName(getEntityTemplateName(eid))
  let defaultAnimcharComponents = get_ecs_object(get_ecs_array(getTemplateComponent(*templateInfo, "anim_replay__savedAnimchars"))?[0])

  var invalidAnimcharIndices : array<int>
  for animcharChildComp, idx in anim_replay__savedAnimchars, count()
    assume animcharData = *getRW_ecs_object(animcharChildComp)
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (motion_matching__blendTimeToAnimtree : float;
                                  motion_matching__dataBaseTemplateName : string;
                                  motion_matching__updateProgress : float;
                                  motion_matching__controller : MotionMatchingController)
      if motion_matching__controller.dataBase == null
        visual_log("Entity <{animcharEid}> ({getEntityTemplateName(animcharEid)}) has not loaded MM database. It will be excluded from anim replay")
        push(invalidAnimcharIndices, idx)
        return

      for defComp in *defaultAnimcharComponents
        if starts_with(defComp.key, "mm")
          if !has(animcharData, defComp.key)
            set(animcharData, defComp.key, defComp.value)
          else
            logerr("component '{defComp.key}' already exists")

      assume mmInitialNodeWeights = *getRW_ecs_FloatList(animcharData, "mmInitialNodeWeights")
      for weight in motion_matching__controller.perNodeWeights
        push(mmInitialNodeWeights, weight)
      set(animcharData, "mmInitialUpdateProgress", motion_matching__updateProgress)
      set(animcharData, "mmPrevUpdateProgress", motion_matching__updateProgress)

      if get_ecs_object(animcharData, "additionalTemplates") == null
        using() <| $(var additionalTemplates : Object)
          set(animcharData, "additionalTemplates", additionalTemplates)
      assume additionalTemplates = *getRW_ecs_object(animcharData, "additionalTemplates")
      using <| $(var mm_component : Object)
        set(mm_component, "motion_matching__blendTimeToAnimtree", motion_matching__blendTimeToAnimtree)
        set(mm_component, "motion_matching__dataBaseTemplateName", motion_matching__dataBaseTemplateName)
        set(additionalTemplates, "motion_matching_animchar_component", mm_component)
      prepare_anim_tree_timer_scale_param_for_save(motion_matching__controller, animcharData)

  for i in iter_range(invalidAnimcharIndices)
    let reverseIdx = length(invalidAnimcharIndices) - 1 - i
    erase(anim_replay__savedAnimchars, invalidAnimcharIndices[reverseIdx])

def track_mm_enabled_state(mm_enabled : bool; time : float; var mm_enabled_ranges : Point2List)
  if mm_enabled && (empty(mm_enabled_ranges) || back(mm_enabled_ranges).y != 0f)
    push(mm_enabled_ranges, float2(time, 0f))
    return true
  elif !mm_enabled && !empty(mm_enabled_ranges) && back(mm_enabled_ranges).y == 0f
    back(mm_enabled_ranges).y = time
  return false

def get_transition_frame(controller : MotionMatchingController;
                         clip : AnimationClip;
                         cur_frame : int;
                         dt : float)
  let passedFrames = int(dt * TICKS_PER_SECOND * controller.playSpeedMult)
  let transitionFrame = cur_frame - passedFrames
  if transitionFrame >= 0
    return transitionFrame
  elif clip.looped
    return (clip.tickDuration - transitionFrame % clip.tickDuration) % clip.tickDuration
  else
    logerr("Negative transition frame {transitionFrame} for non looped anim, dt {dt}, cur_frame {cur_frame}," +
           " last trans {controller.lastTransitionTime}")
    return 0

def track_mm_transitions(time : float;
                         prev_time : float;
                         controller : MotionMatchingController;
                         var transition_time : FloatList;
                         var transition_clip : IntList;
                         var transition_frame : IntList)
  let curClip = controller |> getCurrentClip()
  let curFrame = controller |> getCurrentFrame()
  let timeSinceLastTransition = controller.lastTransitionTime
  if curClip >= 0 && (empty(transition_time) || timeSinceLastTransition + 0.0001f < time - back(transition_time))
    push(transition_time, prev_time)
    push(transition_clip, curClip)
    push(transition_frame, get_transition_frame(controller, controller.dataBase.clips[curClip], curFrame, time - prev_time))

def track_mm_tags(time : float;
                  controller : MotionMatchingController;
                  var tags_change_time : FloatList;
                  var tags_value : IntList;
                  var tags_mask : IntList)
  var value = 0
  var mask = 0
  assume dataBase = *controller.dataBase
  assert(dataBase |> getTagsCount() <= 32)
  for i in range(dataBase |> getTagsCount())
    let tagIdx = uint(i)
    if controller.currentTags |> isTagRequired(tagIdx)
      value |= 1 << i
      mask |= 1 << i
    elif controller.currentTags |> isTagExcluded(tagIdx)
      mask |= 1 << i
  if empty(tags_change_time) || back(tags_value) != value || back(tags_mask) != mask
    push(tags_change_time, time)
    push(tags_value, value)
    push(tags_mask, mask)

[es(after=anim_replay_track_animchar, before=anim_replay_end_of_saving_frame)]
def anim_replay_track_motion_matching_state(evt : UpdateStageInfoAct;
                                            anim_replay__prevTime : float;
                                            anim_replay__time : float;
                                            anim_replay__timestamps : FloatList;
                                            var anim_replay__savedAnimchars : Array)
  for animcharChildComp in anim_replay__savedAnimchars
    assume animcharData = *getRW_ecs_object(animcharChildComp)
    if is_animchar_dead(animcharData, anim_replay__time)
      continue
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (motion_matching__controller : MotionMatchingController;
                                  motion_matching__goalFeature : FrameFeatures;
                                  mm_trajectory__featurePositions : Point3List;
                                  mm_trajectory__featureDirections : Point3List;
                                  mm_trajectory__linearVelocity : float3;
                                  mm_trajectory__angularVelocity : float3;
                                  motion_matching__enabled : bool)
      assume mmEnabledRanges = *getRW_ecs_Point2List(animcharData, "mmEnabledRanges")
      assume mmPoseFeatures = *getRW_ecs_Point3List(animcharData, "mmPoseFeatures")
      let justEnabled = track_mm_enabled_state(motion_matching__enabled, anim_replay__time, mmEnabledRanges)
      if justEnabled
        assume goalFeatures = unsafe(reinterpret<FrameFeatures>(motion_matching__goalFeature)) //remove constness
        get_node_features(goalFeatures, 0) <| $(node_position_features, node_velocity_features)
          for pos in node_position_features
            push(mmPoseFeatures, pos)
          for vel in node_velocity_features
            push(mmPoseFeatures, vel)

      assume mmTransitionTime = *getRW_ecs_FloatList(animcharData, "mmTransitionTime")
      assume mmTransitionClipIdx = *getRW_ecs_IntList(animcharData, "mmTransitionClipIdx")
      assume mmTransitionFrameIdx = *getRW_ecs_IntList(animcharData, "mmTransitionFrameIdx")
      track_mm_transitions(anim_replay__time, anim_replay__prevTime, motion_matching__controller,
        mmTransitionTime, mmTransitionClipIdx, mmTransitionFrameIdx)

      assume mmTagsChangeTime = *getRW_ecs_FloatList(animcharData, "mmTagsChangeTime")
      assume mmTagsValue = *getRW_ecs_IntList(animcharData, "mmTagsValue")
      assume mmTagsMask = *getRW_ecs_IntList(animcharData, "mmTagsMask")
      track_mm_tags(anim_replay__time, motion_matching__controller, mmTagsChangeTime, mmTagsValue, mmTagsMask)

      assume mmActiveAnimationProgress = *getRW_ecs_FloatList(animcharData, "mmActiveAnimationProgress")
      assume mmPlaySpeedMult = *getRW_ecs_FloatList(animcharData, "mmPlaySpeedMult")
      assume mmTransitionBlendTime = *getRW_ecs_FloatList(animcharData, "mmTransitionBlendTime")
      assume mmRootPosition = *getRW_ecs_Point3List(animcharData, "mmRootPosition")
      assume mmRootRotation = *getRW_ecs_Point4List(animcharData, "mmRootRotation")
      assume mmTrajectoryFeatures = *getRW_ecs_Point2List(animcharData, "mmTrajectoryFeatures")
      assume mmTrajectoryLinearVelocity = *getRW_ecs_Point3List(animcharData, "mmTrajectoryLinearVelocity")
      assume mmTrajectoryAngularVelocity = *getRW_ecs_Point3List(animcharData, "mmTrajectoryAngularVelocity")

      if back(anim_replay__timestamps) == anim_replay__time && length(anim_replay__timestamps) > length(mmRootPosition)
        let curFrame = motion_matching__controller |> getCurrentFrame()
        let curFrameProgress = motion_matching__controller |> getCurrentFrameProgress()
        push(mmActiveAnimationProgress, (float(curFrame) + curFrameProgress) / TICKS_PER_SECOND)
        push(mmPlaySpeedMult, motion_matching__controller.playSpeedMult)
        push(mmRootPosition, motion_matching__controller.rootPosition)
        push(mmRootRotation, motion_matching__controller.rootRotation)
        push(mmTransitionBlendTime, motion_matching__controller.transitionBlendTime)
        for pos in mm_trajectory__featurePositions
          push(mmTrajectoryFeatures, pos.xz)
        for dir in mm_trajectory__featureDirections
          push(mmTrajectoryFeatures, dir.xz)
        push(mmTrajectoryLinearVelocity, mm_trajectory__linearVelocity)
        push(mmTrajectoryAngularVelocity, mm_trajectory__angularVelocity)

[es(after=(anim_replay_end_of_saving_frame, wait_motion_matching_job_es), before=(mm_lod_selector, mm_trajectory_prediction))]
def anim_replay_track_motion_matching_search(evt : ParallelUpdateFrameDelayed;
                                             anim_replay__time : float;
                                             anim_replay__prevTime : float;
                                             var anim_replay__savedAnimchars : Array)
  for animcharChildComp in anim_replay__savedAnimchars
    assume animcharData = *getRW_ecs_object(animcharChildComp)
    if is_animchar_dead(animcharData, anim_replay__time)
      continue
    var mmSearchTime & = unsafe(*getRW_ecs_FloatList(animcharData, "mmSearchTime"))
    var mmSearchDistanceFactor & = unsafe(*getRW_ecs_FloatList(animcharData, "mmSearchDistanceFactor"))
    var mmSearchTrajectoryFeatures & = unsafe(*getRW_ecs_Point2List(animcharData, "mmSearchTrajectoryFeatures"))
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (motion_matching__controller : MotionMatchingController;
                                  mm_trajectory__featurePositions : Point3List;
                                  mm_trajectory__featureDirections : Point3List;
                                  motion_matching__updateProgress : float;
                                  motion_matching__distanceFactor : float)
      if !motion_matching__controller.hasActiveAnimation()
        return
      let eps = 1e-6f;
      let hadTransition = (motion_matching__controller.lastTransitionTime <= evt.dt + eps &&
        (empty(mmSearchTime) || anim_replay__prevTime - back(mmSearchTime) > 2f * eps))
      let expectedUpdateProgress = *get_float(animcharData, "mmPrevUpdateProgress") + evt.dt * TICKS_PER_SECOND - eps
      if hadTransition || motion_matching__updateProgress < expectedUpdateProgress
        push(mmSearchTime, anim_replay__prevTime)
        push(mmSearchDistanceFactor, motion_matching__distanceFactor)
        for pos in mm_trajectory__featurePositions
          push(mmSearchTrajectoryFeatures, pos.xz)
        for dir in mm_trajectory__featureDirections
          push(mmSearchTrajectoryFeatures, dir.xz)
      set(animcharData, "mmPrevUpdateProgress", motion_matching__updateProgress)

[es(before=anim_replay_stop_record)]
def anim_replay_finalize_motion_matching_state(evt : AnimReplayStopRecord;
                                               anim_replay__time : float;
                                               anim_replay__timestamps : FloatList;
                                               var anim_replay__savedAnimchars : Array)
  for animcharChildComp in anim_replay__savedAnimchars
    assume animcharData = *getRW_ecs_object(animcharChildComp)
    assume mmEnabledRanges = *getRW_ecs_Point2List(animcharData, "mmEnabledRanges")
    if !empty(mmEnabledRanges) && back(mmEnabledRanges).y == 0f
      back(mmEnabledRanges).y = anim_replay__time
    var trajectoryFeaturesCount = 0
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (mm_trajectory__featurePositions : Point3List;
                                  mm_trajectory__featureDirections : Point3List)
      trajectoryFeaturesCount = length(mm_trajectory__featurePositions) + length(mm_trajectory__featureDirections)
    let savedTrajectoryFeaturesSize = length(*get_ecs_Point2List(animcharData, "mmTrajectoryFeatures"))

    var numTimestamps = length(anim_replay__timestamps)
    let deathTime = get_float(animcharData, "deathTime")
    if deathTime != null
      numTimestamps = lower_bound(anim_replay__timestamps, *deathTime) + 1
    if numTimestamps * trajectoryFeaturesCount != savedTrajectoryFeaturesSize
      logerr("Incorrect vector size for saved trajectory features.\n" +
        "Timestamps {numTimestamps}, features count {trajectoryFeaturesCount}, saved features {savedTrajectoryFeaturesSize}")
      clear(*getRW_ecs_Point2List(animcharData, "mmTrajectoryFeatures"))
    erase(animcharData, "mmPrevUpdateProgress")
