require ecs

require imgui
require imgui/imgui_boost
require daslib/static_let
require MotionMatching
require DagorConsole
require danetlibs/motion_matching/debug_ui/motion_matching_ui_common
require danetlibs/motion_matching/debug_ui_includes/mm_ui_play_clips

[cpp_event(broadcast)]
struct ImGuiStage {}

[es(tag=tools, REQUIRE=motion_matching__controller)]
def mm_ui(event : ImGuiStage; eid : EntityId)

  if imgui::Begin("motion matching")
    motion_matching_imgui(eid)
  imgui::End()

let TICKS_PER_SECOND = 30f

[es(tag=tools)]
def mm_play_animation_update(info : UpdateStageInfoAct; var motion_matching__controller : MotionMatchingController)
  static_let <|
    var prevClipFrame = int2(-1, -1)
    var currentFrameProgress = 0.0f

  if motion_matching__controller |> empty()
    return

  var curClipFrame = int2(motion_matching__controller |> getCurrentClip(), motion_matching__controller |> getCurrentFrame())
  if curClipFrame != prevClipFrame
    currentFrameProgress = float(curClipFrame.y)

  query() <| $ [es] (mm_clips_animationPlaySpeed : float;
                     mm_clips_animationPlayMode : int)
    if mm_clips_animationPlayMode != int(AnimationPlayMode DISABLED)
      currentFrameProgress += info.dt * mm_clips_animationPlaySpeed * TICKS_PER_SECOND

  if int(currentFrameProgress) > curClipFrame.y
    assume curClip = motion_matching__controller.dataBase.clips[curClipFrame.x]
    curClipFrame.y = min(int(currentFrameProgress), curClip.tickDuration - 1)
    motion_matching__controller |> clearAnimations()
    motion_matching__controller |> playAnimation(curClipFrame.x, curClipFrame.y, 0.1)

  prevClipFrame = curClipFrame