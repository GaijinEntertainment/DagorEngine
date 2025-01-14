options no_aot
module mm_ui_play_clips shared

require ecs
require ECSGlobalTags
require DagorTime
require DagorConsole
require MotionMatching
require imgui
require imgui/imgui_boost
require daslib/safe_addr
require daslib/static_let
require danetlibs/motion_matching/debug_ui_includes/mm_ui_imgui_helpers

enum AnimationPlayMode : int
  DISABLED = 0
  ON_PAUSE = 1
  CONTINUOUS = 2

struct MMState
  rootSynchronization : bool
  rootClamping : bool
  valid : bool

def save_mm_game_state(var state : MMState; mm_ctrl : MotionMatchingController)
  state.rootSynchronization = mm_ctrl.rootSynchronization
  state.rootClamping = mm_ctrl.rootClamping

def restore_mm_game_state(state : MMState; var mm_ctrl : MotionMatchingController; data_base_eid : EntityId)
  mm_ctrl.rootSynchronization = state.rootSynchronization
  mm_ctrl.rootClamping = state.rootClamping
  query() <| $ [es] (mm_params__footLockerEnabled : bool)
    change_foot_locker_state(mm_params__footLockerEnabled, data_base_eid)

def change_foot_locker_state(enable : bool; data_base_eid : EntityId)
  query(data_base_eid) <| $ [es] (main_database__footLockerCtrlName : string;
                                  var dataBase : AnimationDataBase)
    if dataBase.footLockerParamId >= 0
      let footLockerPBCIdx = get_post_blend_controller_idx(dataBase, main_database__footLockerCtrlName)
      dataBase |> changePBCWeightOverride(footLockerPBCIdx, enable ? 1f : 0f)

def history_slider(imgui_state_eid : EntityId; data_base : AnimationDataBase const?; var clip_frame_pair : int2&)
  query(imgui_state_eid) <| $ [es] (mm_clips_history : IPoint2List;
                                    var mm_clips_historyIndex : int2&)
    let historyLen = length(mm_clips_history)
    if historyLen > 0
      imgui::Text("History: ")
      imgui::SameLine()
      let currentIndex = (mm_clips_historyIndex.x + historyLen + mm_clips_historyIndex.y) % historyLen
      let clipId = mm_clips_history[currentIndex].x
      let frameId = mm_clips_history[currentIndex].y
      var v = mm_clips_historyIndex.x + 1
      var playAnim = false
      if imgui::Button("+##history") && v < historyLen
        v++
        playAnim = true
      imgui::SameLine()
      if imgui::Button("-##history") && v > 0
        v--
        playAnim = true
      imgui::SameLine()
      if imgui::SliderInt("##history", safe_addr(v), 1, historyLen)
        playAnim = true
      if playAnim
        mm_clips_historyIndex.x = v - 1
        let newIndex = (mm_clips_historyIndex.x + historyLen + mm_clips_historyIndex.y) % historyLen
        clip_frame_pair = mm_clips_history[newIndex]
      imgui::SameLine()
      imgui::Text("{data_base.clips[clipId].name} frame={frameId}")

def get_clip_id_by_name(data_base : AnimationDataBase const?; name : string)
  for i in range(length(data_base.clips))
    if name == data_base.clips[i].name
      return i
  return -1

def pause_on_transition(imgui_state_eid : EntityId; data_base : AnimationDataBase const?)
  static_let <|
    var lastHistoryIdx = -1
  query(imgui_state_eid) <| $ [es] (var mm_clips__pauseOnTransitionFrom : das_string&;
                                    var mm_clips__pauseOnTransitionFromIdx : int&;
                                    var mm_clips__pauseOnTransitionTo : das_string&;
                                    var mm_clips__pauseOnTransitionToIdx : int&;
                                    var mm_clips__pausedOnTransition : bool&;
                                    mm_clips_history : IPoint2List;
                                    mm_clips_historyIndex : int2)
    imgui::Text("Pause on transition")
    imgui::Text("From")
    imgui::SameLine()
    imgui_input_text("##From", mm_clips__pauseOnTransitionFrom)
    if imgui::IsItemDeactivatedAfterEdit()
      mm_clips__pauseOnTransitionFromIdx = get_clip_id_by_name(data_base, string(mm_clips__pauseOnTransitionFrom))
    if mm_clips__pauseOnTransitionFromIdx >= 0
      imgui::SameLine()
      TextColored(float4(1., 1., 1., 1.), "Clip idx = {mm_clips__pauseOnTransitionFromIdx}")
    elif !empty(mm_clips__pauseOnTransitionFrom) && !imgui::IsItemActive()
      imgui::SameLine()
      TextColored(float4(1., 0., 0., 1.), "Not found")
    imgui::Text("To")
    imgui::SameLine()
    imgui_input_text("##To", mm_clips__pauseOnTransitionTo)
    if imgui::IsItemDeactivatedAfterEdit()
      mm_clips__pauseOnTransitionToIdx = get_clip_id_by_name(data_base, string(mm_clips__pauseOnTransitionTo))
    if mm_clips__pauseOnTransitionToIdx >= 0
      imgui::SameLine()
      TextColored(float4(1., 1., 1., 1.), "Clip idx = {mm_clips__pauseOnTransitionToIdx}")
    elif !empty(mm_clips__pauseOnTransitionTo) && !imgui::IsItemActive()
      imgui::SameLine()
      TextColored(float4(1., 0., 0., 1.), "Not found")
    if mm_clips__pausedOnTransition && imgui::Button("unpause")
      console_command("app.timeSpeed 1")
      mm_clips__pausedOnTransition = false
    let historyLen = length(mm_clips_history)
    if historyLen < 2 || mm_clips_historyIndex.y == lastHistoryIdx
      return
    lastHistoryIdx = -1
    let curClipFrame = mm_clips_history[(mm_clips_historyIndex.y + historyLen - 1) % historyLen]
    let prevClipFrame = mm_clips_history[(mm_clips_historyIndex.y + historyLen - 2) % historyLen]
    if curClipFrame.x == prevClipFrame.x && (prevClipFrame.y + 1) % data_base.clips[curClipFrame.x].tickDuration == curClipFrame.y
      return //no transition
    if ((mm_clips__pauseOnTransitionFromIdx == prevClipFrame.x && mm_clips__pauseOnTransitionToIdx == curClipFrame.x) ||
        (mm_clips__pauseOnTransitionFromIdx == prevClipFrame.x && mm_clips__pauseOnTransitionToIdx == -1) ||
        (mm_clips__pauseOnTransitionFromIdx == -1 && mm_clips__pauseOnTransitionToIdx == curClipFrame.x))
      console_command("app.timeSpeed 0")
      mm_clips__pausedOnTransition = true
      lastHistoryIdx = mm_clips_historyIndex.y

def history_section(imgui_state_eid : EntityId; data_base : AnimationDataBase const?; var clip_frame_pair : int2&)
  if imgui::CollapsingHeader("History")
    history_slider(imgui_state_eid, data_base, clip_frame_pair)
    pause_on_transition(imgui_state_eid, data_base)
    imgui::Separator()

def available_animation_sliders(data_base : AnimationDataBase const?;
                                controller : MotionMatchingController;
                                animation_play_mode : int;
                                var clip_progress : IntList;
                                var anim_play_speed : float&;
                                var clip_frame_pair : int2&)
  static_let <|
    var animationPlaySpeed = 1f

  if safe_slider("animation play speed", animationPlaySpeed, 0f, 2f) && animation_play_mode == int(AnimationPlayMode CONTINUOUS)
    anim_play_speed = animationPlaySpeed
    console_command("app.timeSpeed {animationPlaySpeed}")

  imgui::BeginChild("animation_list", float2(0, 0), ImGuiChildFlags None, ImGuiWindowFlags None)
  resize(clip_progress, length(data_base.clips))
  let curClipIdx = controller |> hasActiveAnimation() ? controller |> getCurrentClip() : -1
  for clip, clipIdx in data_base.clips, iter_range(data_base.clips)
    if animation_play_mode != int(AnimationPlayMode DISABLED) && curClipIdx == clipIdx
      clip_progress[clipIdx] = controller |> getCurrentFrame()
      if !clip.looped && anim_play_speed > 0.0f && int(clip_progress[clipIdx]) == clip.tickDuration - 1
        clip_frame_pair = int2(clipIdx, 0)
    let curProgress = int(clip_progress[clipIdx])
    if imgui::Button("+##{clipIdx}")
      clip_frame_pair = int2(clipIdx, (curProgress + 1) % clip.tickDuration)
      anim_play_speed = 0.0f
    imgui::SameLine()
    if imgui::Button("-##{clipIdx}")
      clip_frame_pair = int2(clipIdx, curProgress - 1 >= 0 ? curProgress - 1 : clip.tickDuration - 1)
      anim_play_speed = 0.0f
    imgui::SameLine()
    if imgui::Button("play##{clipIdx}")
      clip_frame_pair = int2(clipIdx, curProgress)
      anim_play_speed = animationPlaySpeed
    imgui::SameLine()

    var v = int(curProgress)
    SetNextItemWidth(150.f)
    if imgui::SliderInt("{clip.name}", safe_addr(v), 0, clip.tickDuration - 1)
      clip_frame_pair = int2(clipIdx, v)
      anim_play_speed = 0.0f
  imgui::EndChild()

def track_history(imgui_state_eid : EntityId; controller : MotionMatchingController)
  if controller |> hasActiveAnimation()
    query(imgui_state_eid) <| $ [es] (var mm_clips_history : IPoint2List;
                                      var mm_clips_historyIndex : int2&)
      let maxHistorySize = 4096
      var prevValue = int2(-1, -1)
      let historyLen = length(mm_clips_history)
      if historyLen > 0
        prevValue = mm_clips_history[(mm_clips_historyIndex.y + historyLen - 1) % historyLen]
      let value = int2(controller |> getCurrentClip(), controller |> getCurrentFrame())
      if prevValue != value
        if historyLen < maxHistorySize
          mm_clips_history |> push(value)
          mm_clips_historyIndex.y = historyLen + 1
        else
          mm_clips_historyIndex.y = mm_clips_historyIndex.y % maxHistorySize
          mm_clips_history[mm_clips_historyIndex.y] = value
          mm_clips_historyIndex.y++

def play_clips(character_eid : EntityId; data_base_eid : EntityId; imgui_state_eid : EntityId)
  static_let <|
    var lockRootNode = false
    var enableFootLocker = false
    var savedGameState : MMState
  query(character_eid) <| $ [es] (var motion_matching__controller : MotionMatchingController;
                                  var motion_matching__updateProgress : float&)
    query(imgui_state_eid) <| $ [es] (var mm_clips_clipProgress : IntList;
                                      var mm_clips_animationPlaySpeed : float&;
                                      var mm_clips_animationPlayMode : int&)
      if imgui::BeginTabItem("clips")
        assume dataBase = motion_matching__controller.dataBase
        if motion_matching__controller |> hasActiveAnimation()
          let currentClip = motion_matching__controller |> getCurrentClip()
          imgui::Text("Current clip: ")
          imgui::SameLine()
          var currentFrame = motion_matching__controller |> getCurrentFrame()
          imgui::SliderInt("{dataBase.clips[currentClip].name}##display", safe_addr(currentFrame), 1, dataBase.clips[currentClip].tickDuration)
          imgui::Separator()

        var clipFramePair = int2(-1, -1)
        history_section(imgui_state_eid, dataBase, clipFramePair)
        if !ecs_has_tag("tools") // not implemented in tools currently
          safe_checkbox("lock root node", lockRootNode)
          imgui::SameLine()
          safe_checkbox("foot locker", enableFootLocker)
        available_animation_sliders(dataBase, motion_matching__controller, mm_clips_animationPlayMode,
                                    mm_clips_clipProgress, mm_clips_animationPlaySpeed, clipFramePair)
        if clipFramePair != int2(-1, -1)
          motion_matching__controller |> clearAnimations()
          motion_matching__controller |> playAnimation(clipFramePair.x, clipFramePair.y, false)
          mm_clips_animationPlayMode = mm_clips_animationPlaySpeed > 0.0f ? int(AnimationPlayMode CONTINUOUS) : int(AnimationPlayMode ON_PAUSE)
          console_command("app.timeSpeed {mm_clips_animationPlaySpeed}")
          if ecs_has_tag("tools") //console command for time doesn't work in tools, so just disable MM update at all
            query(data_base_eid) <| $ [es] (var main_database__perFrameLimit : int&)
              main_database__perFrameLimit = 0
        if mm_clips_animationPlayMode != int(AnimationPlayMode DISABLED)
          motion_matching__updateProgress = 0.0f //disables MM search in next frame
          if !savedGameState.valid
            save_mm_game_state(savedGameState, motion_matching__controller)
            savedGameState.valid = true
          motion_matching__controller.rootClamping = false
          motion_matching__controller.rootSynchronization = lockRootNode
          change_foot_locker_state(enableFootLocker, data_base_eid)
        imgui::EndTabItem()
      elif mm_clips_animationPlayMode != int(AnimationPlayMode DISABLED) //not in 'clips' tab anymore
        mm_clips_animationPlayMode = int(AnimationPlayMode DISABLED)
        if savedGameState.valid
          restore_mm_game_state(savedGameState, motion_matching__controller, data_base_eid)
          savedGameState.valid = false
        console_command("app.timeSpeed 1")
        if ecs_has_tag("tools")
          query(data_base_eid) <| $ [es] (var main_database__perFrameLimit : int&)
            main_database__perFrameLimit = -1

      if mm_clips_animationPlayMode == int(AnimationPlayMode DISABLED)
        track_history(imgui_state_eid, motion_matching__controller)