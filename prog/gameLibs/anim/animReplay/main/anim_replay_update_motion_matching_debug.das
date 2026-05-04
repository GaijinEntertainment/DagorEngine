options no_aot
require ecs
require ecs.safe
require daslib/algorithm
require math
require AnimV20
require DagorMath
require MotionMatching
require %gameLibs/anim/animReplay/imports/anim_replay_common
require gameLibs.pufd_events.pufd_events


def adjust_play_speed_mult_from_recorded_data(time_from : float;
                                              time_to : float;
                                              next_transition_time : float;
                                              timestamps : FloatList;
                                              animation_progress : FloatList;
                                              play_speed_mult : FloatList;
                                              var controller : MotionMatchingController)
  let curClip = controller |> getCurrentClip()
  if curClip < 0 || time_from >= time_to
    return
  let curFrame = controller |> getCurrentFrame()
  let curAnimProgress = (float(curFrame) + controller |> getCurrentFrameProgress()) / TICKS_PER_SECOND
  let nextTimeIdx = max(lower_bound(timestamps, time_to), 1)
  assert(nextTimeIdx < length(timestamps))
  let prevTimeIdx = nextTimeIdx - 1
  let prevInterpTime = timestamps[prevTimeIdx] > time_from ? timestamps[prevTimeIdx] : time_from
  let prevAnimProgress = timestamps[prevTimeIdx] > time_from ? animation_progress[prevTimeIdx] : curAnimProgress
  let nextInterpTime = timestamps[nextTimeIdx] <= next_transition_time ? timestamps[nextTimeIdx] : next_transition_time
  var nextAnimProgress = (timestamps[nextTimeIdx] <= next_transition_time ? animation_progress[nextTimeIdx] :
    prevAnimProgress + play_speed_mult[prevTimeIdx] * (nextInterpTime - prevInterpTime))
  // We have negative animation_progress values when MM has no active animation, it can happen when MM was disabled
  // and entirely blended to animTree or character animations weren't updated at all (because not visible).
  // In both cases we don't need to adjust play speed multiplier, because character is not animated by MM.
  if prevAnimProgress < 0f || nextAnimProgress < 0f
    return
  if nextAnimProgress < prevAnimProgress
    nextAnimProgress += controller.dataBase.clips[curClip].duration
  assert(nextAnimProgress >= prevAnimProgress)
  let interpK = (time_to - prevInterpTime) / (nextInterpTime - prevInterpTime)
  var targetAnimProgress = lerp(prevAnimProgress, nextAnimProgress, interpK)
  if targetAnimProgress < curAnimProgress
    targetAnimProgress += controller.dataBase.clips[curClip].duration
  assert(targetAnimProgress >= curAnimProgress)
  controller.playSpeedMult = (targetAnimProgress - curAnimProgress) / (time_to - time_from)

def update_play_speed_mult_from_current_speed(next_time_idx : int;
                                              timestamps : FloatList;
                                              animchar_data : Object;
                                              var controller : MotionMatchingController)
  let mmTrajectoryLinearVelocity = get_ecs_Point3List(animchar_data, "mmTrajectoryLinearVelocity")
  let mmTrajectoryAngularVelocity = get_ecs_Point3List(animchar_data, "mmTrajectoryAngularVelocity")
  let dt = timestamps[next_time_idx] - timestamps[next_time_idx - 1]
  var velocity, angVelocity : float3
  if mmTrajectoryLinearVelocity != null
    velocity = (*mmTrajectoryLinearVelocity)[next_time_idx - 1]
  else
    let positions & = unsafe(*get_ecs_Point3List(animchar_data, "position"))
    velocity = (positions[next_time_idx] - positions[next_time_idx - 1]) / dt
  if mmTrajectoryAngularVelocity != null
    angVelocity = (*mmTrajectoryAngularVelocity)[next_time_idx - 1]
  else
    let rotations & = unsafe(*get_ecs_Point4List(animchar_data, "rotation"))
    let q = quat(rotations[next_time_idx]) * inverse(quat(rotations[next_time_idx - 1]))
    var angles : float3
    quat_to_euler(q, angles.x, angles.y, angles.z)
    angVelocity = float3(0f, angles.x / dt, 0f)
  controller |> updateAnimationSpeed(velocity, angVelocity)

def perform_motion_matching_search(animchar_data : Object;
                                   timestamps : FloatList;
                                   search_time : float;
                                   prev_transition_time : float;
                                   var controller : MotionMatchingController;
                                   var goal_features : FrameFeatures;
                                   var best_clip : int&;
                                   var best_frame : int&)
  best_clip = -1
  best_frame = -1
  query() <| $ [es] (var mm_visualization__prevTransitionClipIdx : int&;
                     var mm_visualization__prevTransitionFrameIdx : int&)
    mm_visualization__prevTransitionClipIdx = -1
    mm_visualization__prevTransitionFrameIdx = -1

  get_trajectory_features(goal_features, 0) <| $(trajectory_pos, trajectory_dir)
    sample_trajectory_features(animchar_data, timestamps, search_time, trajectory_pos, trajectory_dir)
  if controller.motionMatchingWeight > 0f
    controller |> copyPoseFeaturesFromActiveAnimation(goal_features)

  assume dataBase = *controller.dataBase
  update_mm_tags(search_time, animchar_data, controller)
  let presetIdx = dataBase |> getPresetIdxFromTags(controller.currentTags)
  let preset & = unsafe(dataBase.tagsPresets[presetIdx])
  let timeSinceLastTransition = search_time - prev_transition_time
  let additionalTransitionCost = lerp(preset.metricaToleranceMax, preset.metricaToleranceMin,
    min(timeSinceLastTransition / (preset.metricaToleranceDecayTime + 0.0001f), 1.f))

  let nextDt = get_next_timestamp(search_time, timestamps) - search_time
  let forceTransition = (controller |> willCurrentAnimationEnd(nextDt) ||
    !controller |> isAnimationTagsMatch() || controller.motionMatchingWeight <= 0f)

  using() <| $(var searchFeatures : MotionMatchingSearchFeatures)
    prepare_features_for_mm_search(dataBase, goal_features, searchFeatures)
    var currentState : MatchingResult
    currentState.clip = controller.getCurrentClip()
    currentState.frame = controller.getCurrentFrame()
    if forceTransition
      currentState.metrica = FLT_MAX
    else
      currentState.metrica = calculate_trajectory_features_cost(dataBase, currentState.clip, currentState.frame, preset.weights, searchFeatures)
      if currentState.metrica < additionalTransitionCost
        return
    let bestState = motion_matching(dataBase, currentState, controller.currentTags, preset.weights, searchFeatures)
    if bestState.metrica + additionalTransitionCost < currentState.metrica
      best_clip = bestState.clip
      best_frame = bestState.frame

def reset_mm_controller(initial_node_weights : FloatList;
                        var controller : MotionMatchingController)
  controller |> clearAnimations()
  for i in range(length(initial_node_weights))
    controller.perNodeWeights[i] = initial_node_weights[i]
  for pos in controller.offset.position
    pos = float4(0)
  for vel in controller.offset.velocity
    vel = float4(0)
  for rot in controller.offset.rotation
    rot = float4(0, 0, 0, 1)
  for angVel in controller.offset.angular_velocity
    angVel = float4(0)

[es(before=anim_replay_update_foot_locker)]
def anim_replay_update_motion_matching(evt : UpdateStageInfoAct;
                                       anim_replay__animchars : Array;
                                       anim_replay__timestamps : FloatList;
                                       anim_replay__prevTime : float;
                                       anim_replay__currentTime : float;
                                       anim_replay__runMMSearch : bool;
                                       anim_replay__disableMMTransitionBlend : bool;
                                       var anim_replay__prevMMTransitionTime : float&;
                                       var anim_replay__prevMMTransitionFrame : int&)
  if anim_replay__currentTime > 0.f && anim_replay__prevTime == anim_replay__currentTime
    return
  var selectedAnimcharEid = INVALID_ENTITY_ID
  query() <| $ [es] (mm_imguiAnimcharEid : EntityId)
    selectedAnimcharEid = mm_imguiAnimcharEid
  iterate_alive_replay_animchars(anim_replay__animchars, anim_replay__currentTime) <| $(animcharData)
    assume mmEnabledRanges = *get_ecs_Point2List(animcharData, "mmEnabledRanges")
    assume mmTransitionTime = *get_ecs_FloatList(animcharData, "mmTransitionTime")
    assume mmTransitionClipIdx = *get_ecs_IntList(animcharData, "mmTransitionClipIdx")
    assume mmTransitionFrameIdx = *get_ecs_IntList(animcharData, "mmTransitionFrameIdx")
    assume mmActiveAnimationProgress = *get_ecs_FloatList(animcharData, "mmActiveAnimationProgress")
    assume mmPlaySpeedMult = *get_ecs_FloatList(animcharData, "mmPlaySpeedMult")
    assume mmTransitionBlendTime = *get_ecs_FloatList(animcharData, "mmTransitionBlendTime")
    assume mmInitialNodeWeights = *get_ecs_FloatList(animcharData, "mmInitialNodeWeights")

    let animcharEid = *get_Eid(animcharData, "eid")
    let selectedAnimchar = animcharEid == selectedAnimcharEid
    let runMMSearch = selectedAnimchar && anim_replay__runMMSearch
    query(animcharEid) <| $ [es] (motion_matching__blendTimeToAnimtree : float;
                                  var motion_matching__enabled : bool&;
                                  var motion_matching__goalFeature : FrameFeatures;
                                  var motion_matching__controller : MotionMatchingController)
      var t = anim_replay__prevTime
      if t > anim_replay__currentTime || anim_replay__currentTime == 0f
        t = 0.f
        reset_mm_controller(mmInitialNodeWeights, motion_matching__controller)
        anim_replay__prevMMTransitionTime = 0f
        anim_replay__prevMMTransitionFrame = -1
      var nextTransitionIdx = lower_bound(mmTransitionTime, t)
      var enabledRangeIdx = lower_bound(mmEnabledRanges, float2(0, t)) <| $(left, right)
        return left.y < right.y
      var mmEnabledRange = mmEnabledRanges?[enabledRangeIdx] ?? float2(FLT_MAX, FLT_MAX)
      while t < anim_replay__currentTime
        let nextTransitionTime = mmTransitionTime?[nextTransitionIdx] ?? FLT_MAX
        let mmEnabled = mmEnabledRange.x <= t && t < mmEnabledRange.y
        motion_matching__enabled = mmEnabled
        let nextEnabledToggleTime = mmEnabled ? mmEnabledRange.y : mmEnabledRange.x
        var nextMMSearchTime = FLT_MAX
        if runMMSearch
          let nextTimestampIdx = get_next_timestamp_idx(t, anim_replay__timestamps)
          // We have saved trajectory features only at these timestamps
          nextMMSearchTime = min(anim_replay__timestamps[nextTimestampIdx], nextTransitionTime)
          // Do not perform MM search until first transtition, we need it because pose features are not properly initialized
          if nextMMSearchTime <= mmTransitionTime?[0] ?? FLT_MAX
            nextMMSearchTime = FLT_MAX
          update_play_speed_mult_from_current_speed(nextTimestampIdx, anim_replay__timestamps, animcharData, motion_matching__controller)

        let nextTime = min(min(anim_replay__currentTime, nextMMSearchTime), min(nextTransitionTime, nextEnabledToggleTime))
        if !runMMSearch
          adjust_play_speed_mult_from_recorded_data(t, nextTime, nextTransitionTime, anim_replay__timestamps,
            mmActiveAnimationProgress, mmPlaySpeedMult, motion_matching__controller)
        sample_timestamps(t, anim_replay__timestamps) <| $(idx0; idx1; interpK)
          //approximate value
          motion_matching__controller.transitionBlendTime = lerp(mmTransitionBlendTime[idx0], mmTransitionBlendTime[idx1], interpK)
        let dt = nextTime - t
        motion_matching__controller |> updateNodeWeights(mmEnabled, motion_matching__blendTimeToAnimtree, dt)
        motion_matching__controller |> updateAnimationProgress(dt)
        let keepPoseFeatures = (nextTime <= anim_replay__prevMMTransitionTime + 0.02f &&
          motion_matching__controller.getCurrentFrame() == anim_replay__prevMMTransitionFrame)
        if selectedAnimchar && !keepPoseFeatures
          if motion_matching__controller.motionMatchingWeight > 0f
            motion_matching__controller |> copyPoseFeaturesFromActiveAnimation(motion_matching__goalFeature)
          else
            get_node_features(motion_matching__goalFeature, 0) <| $(node_pos, node_vel)
              udpate_pose_features(animcharData, enabledRangeIdx, node_pos, node_vel)
          query() <| $ [es] (var mm_visualization__prevTransitionClipIdx : int&;
                             var mm_visualization__prevTransitionFrameIdx : int&)
            mm_visualization__prevTransitionClipIdx = -1
            mm_visualization__prevTransitionFrameIdx = -1
        t = nextTime
        var transitionClipIdx = -1
        var transitionFrameIdx = -1
        if nextTime == nextTransitionTime
          transitionClipIdx = mmTransitionClipIdx[nextTransitionIdx]
          transitionFrameIdx = mmTransitionFrameIdx[nextTransitionIdx]
          nextTransitionIdx++
        if runMMSearch && nextTime == nextMMSearchTime && (motion_matching__controller.motionMatchingWeight > 0f || nextTime == mmEnabledRange.x)
          perform_motion_matching_search(animcharData, anim_replay__timestamps, nextTime, anim_replay__prevMMTransitionTime,
            motion_matching__controller, motion_matching__goalFeature, transitionClipIdx, transitionFrameIdx)
        if transitionClipIdx >= 0
          if selectedAnimchar
            anim_replay__prevMMTransitionTime = nextTime
            anim_replay__prevMMTransitionFrame = transitionFrameIdx
            query() <| $ [es] (var mm_visualization__prevTransitionClipIdx : int&;
                               var mm_visualization__prevTransitionFrameIdx : int&)
              mm_visualization__prevTransitionClipIdx = motion_matching__controller.getCurrentClip()
              mm_visualization__prevTransitionFrameIdx = motion_matching__controller.getClosestFrame()
          let needTransitionBlend = (motion_matching__controller |> hasActiveAnimation() &&
            motion_matching__controller.motionMatchingWeight > 0f && !anim_replay__disableMMTransitionBlend)
          motion_matching__controller |> playAnimation(transitionClipIdx, transitionFrameIdx, needTransitionBlend)
          if nextTime == anim_replay__currentTime
            motion_matching__controller |> updateAnimationProgress(0f)
        if nextTime == nextEnabledToggleTime && mmEnabled
          enabledRangeIdx++
          mmEnabledRange = mmEnabledRanges?[enabledRangeIdx] ?? float2(FLT_MAX, FLT_MAX)

      if !motion_matching__enabled && motion_matching__controller.motionMatchingWeight <= 0.f
        motion_matching__controller |> clearAnimations()

[es(after=anim_replay_update_motion_matching, before=anim_replay_update_anim_state)]
def anim_replay_update_foot_locker(evt : UpdateStageInfoAct;
                                   anim_replay__animchars : Array;
                                   anim_replay__currentTime : float)
  iterate_alive_replay_animchars(anim_replay__animchars, anim_replay__currentTime) <| $(animcharData)
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (var animchar : AnimcharBaseComponent;
                                  motion_matching__controller : MotionMatchingController)
      motion_matching_update_anim_tree_foot_locker(animchar, motion_matching__controller)

def update_root_orientation(current_time : float;
                            timestamps : FloatList;
                            root_position : Point3List;
                            root_rotation : Point4List;
                            var controller : MotionMatchingController)
  sample_timestamps(current_time, timestamps) <| $(idx0; idx1; interpK)
    controller.rootPosition = lerp(root_position[idx0], root_position[idx1], interpK)
    controller.rootRotation = quat_slerp(interpK, root_rotation[idx0], root_rotation[idx1])

[es(after=mm_update_root_orientation_es, before=mm_calculate_root_offset_es)]
def anim_replay_update_motion_matching_root(evt : ParallelUpdateFrameDelayed;
                                            anim_replay__animchars : Array;
                                            anim_replay__timestamps : FloatList;
                                            anim_replay__currentTime : float)
  iterate_alive_replay_animchars(anim_replay__animchars, anim_replay__currentTime) <| $(animcharData)
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (var motion_matching__controller : MotionMatchingController)
      motion_matching__controller.rootSynchronization = false
      assume mmRootPosition = *get_ecs_Point3List(animcharData, "mmRootPosition")
      assume mmRootRotation = *get_ecs_Point4List(animcharData, "mmRootRotation")
      update_root_orientation(anim_replay__currentTime, anim_replay__timestamps, mmRootPosition, mmRootRotation,
        motion_matching__controller)

def update_mm_tags(time : float;
                   tags_change_time : FloatList;
                   tags_value : IntList;
                   tags_mask : IntList;
                   var controller : MotionMatchingController)
  let timeIdx = get_current_timestamp_idx(time, tags_change_time)
  let value = tags_value[timeIdx]
  let mask = tags_mask[timeIdx]
  assume dataBase = *controller.dataBase
  for i in range(dataBase |> getTagsCount())
    let tagIdx = uint(i)
    controller.currentTags |> removeTag(tagIdx)
    if (mask & (1 << i)) != 0
      if (value & (1 << i)) != 0
        controller.currentTags |> requireTag(tagIdx)
      else
        controller.currentTags |> excludeTag(tagIdx)

def update_mm_tags(time : float;
                   animcharData : Object;
                   var controller : MotionMatchingController)
  assume mmTagsChangeTime = *get_ecs_FloatList(animcharData, "mmTagsChangeTime")
  assume mmTagsValue = *get_ecs_IntList(animcharData, "mmTagsValue")
  assume mmTagsMask = *get_ecs_IntList(animcharData, "mmTagsMask")
  update_mm_tags(time, mmTagsChangeTime, mmTagsValue, mmTagsMask, controller)

[es(no_order)]
def anim_replay_update_motion_matching_tags(evt : UpdateStageInfoAct;
                                            anim_replay__animchars : Array;
                                            anim_replay__currentTime : float)
  iterate_alive_replay_animchars(anim_replay__animchars, anim_replay__currentTime) <| $(animcharData)
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (var motion_matching__controller : MotionMatchingController)
      update_mm_tags(anim_replay__currentTime, animcharData, motion_matching__controller)

def udpate_pose_features(animchar_data : Object; mm_enabled_idx; var node_pos : array<float3>; var node_vel : array<float3>)
  let mmPoseFeatures & = unsafe(*get_ecs_Point3List(animchar_data, "mmPoseFeatures"))
  let featuresCount = length(node_pos) + length(node_vel)
  var featureIdx = mm_enabled_idx * featuresCount
  if length(mmPoseFeatures) < featureIdx + featuresCount
    return
  for pos in node_pos
    pos = mmPoseFeatures[featureIdx]
    featureIdx++
  for vel in node_vel
    vel = mmPoseFeatures[featureIdx]
    featureIdx++

def sample_trajectory_features(animchar_data : Object; timestamps : FloatList; cur_time : float;
                               var trajectory_pos : array<float2>; var trajectory_dir : array<float2>)
  unsafe
    let mmTrajectoryFeatures & = *get_ecs_Point2List(animchar_data, "mmTrajectoryFeatures")
    let mmSearchTrajectoryFeatures & = *get_ecs_Point2List(animchar_data, "mmSearchTrajectoryFeatures")
    let mmSearchTime & = *get_ecs_FloatList(animchar_data, "mmSearchTime")
    if empty(mmTrajectoryFeatures)
      return
    let timeIdx = get_current_timestamp_idx(cur_time, timestamps)
    let searchTimeIdx = get_current_timestamp_idx(cur_time, mmSearchTime)
    let useSearchTrajectory = (length(mmSearchTime) > 0 &&
      cur_time >= mmSearchTime[searchTimeIdx] && mmSearchTime[searchTimeIdx] >= timestamps[timeIdx])
    let features & = useSearchTrajectory ? mmSearchTrajectoryFeatures : mmTrajectoryFeatures
    let featuresCount = length(trajectory_pos) + length(trajectory_dir)
    assert(length(features) % featuresCount == 0)
    var featureIdx = (useSearchTrajectory ? searchTimeIdx : timeIdx) * featuresCount
    for pos in trajectory_pos
      pos = features[featureIdx]
      featureIdx++
    for dir in trajectory_dir
      dir = features[featureIdx]
      featureIdx++

[es(after=mm_trajectory_prediction, before=mm_update_goal_features_es)]
def anim_replay_update_motion_matching_trajectory(evt : ParallelUpdateFrameDelayed;
                                                  anim_replay__animchars : Array;
                                                  anim_replay__timestamps : FloatList;
                                                  anim_replay__currentTime : float)
  var trajectoryPos, trajectoryDir : array<float2>
  iterate_alive_replay_animchars(anim_replay__animchars, anim_replay__currentTime) <| $(animcharData)
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (var mm_trajectory__featurePositions : Point3List;
                                  var mm_trajectory__featureDirections : Point3List)
      resize_and_init(trajectoryPos, length(mm_trajectory__featurePositions), float2(0, 0))
      resize_and_init(trajectoryDir, length(mm_trajectory__featureDirections), float2(1, 0))
      sample_trajectory_features(animcharData, anim_replay__timestamps, anim_replay__currentTime, trajectoryPos, trajectoryDir)
      for pos, sampledPos in mm_trajectory__featurePositions, trajectoryPos
        pos = float3(sampledPos.x, 0f, sampledPos.y)
      for dir, sampledDir in mm_trajectory__featureDirections, trajectoryDir
        dir = float3(sampledDir.x, 0f, sampledDir.y)

[es(after=wait_motion_matching_job_es)]
def anim_replay_update_motion_matching_anim_speed(evt : ParallelUpdateFrameDelayed;
                                                  anim_replay__animchars : Array;
                                                  anim_replay__timestamps : FloatList;
                                                  anim_replay__currentTime : float)
  iterate_alive_replay_animchars(anim_replay__animchars, anim_replay__currentTime) <| $(animcharData)
    let animcharEid = *get_Eid(animcharData, "eid")
    query(animcharEid) <| $ [es] (var motion_matching__controller : MotionMatchingController)
      let nextTimestampIdx = get_next_timestamp_idx(anim_replay__currentTime, anim_replay__timestamps)
      update_play_speed_mult_from_current_speed(nextTimestampIdx, anim_replay__timestamps, animcharData, motion_matching__controller)
