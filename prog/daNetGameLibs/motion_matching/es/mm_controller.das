require ecs
require AnimV20
require MotionMatching
require danetlibs.renderer.includes.pufd_events


[es(after=wait_motion_matching_job_es)]
def mm_lod_selector(info : ParallelUpdateFrameDelayed;
                    var motion_matching__distanceFactor : float&;
                    transform aka character_transform : float3x4)
  motion_matching__distanceFactor = 1.f
  find_query() <| $ [es] (camera__active : bool; transform aka camera_transform : float3x4)
    if !camera__active
      return false
    let distance = length(camera_transform[3] - character_transform[3])
    motion_matching__distanceFactor += 0.2f * sqrt(distance)
    return true

[es(after=wait_motion_matching_job_es)]
def mm_update_metrica_tolerance(info : ParallelUpdateFrameDelayed;
                                motion_matching__controller : MotionMatchingController;
                                motion_matching__presetIdx : int;
                                var motion_matching__metricaTolerance : float&)
  assume presets = motion_matching__controller.dataBase.tagsPresets
  if motion_matching__presetIdx >= 0 && motion_matching__presetIdx < length(presets)
    assume preset = presets[motion_matching__presetIdx]
    let decaySpeed = (preset.metricaToleranceMax - preset.metricaToleranceMin) / (preset.metricaToleranceDecayTime + 0.0001)
    motion_matching__metricaTolerance = max(motion_matching__metricaTolerance - decaySpeed * info.dt, preset.metricaToleranceMin)

[es(after=wait_motion_matching_job_es, before=animchar_es)]
def mm_synchronize_anim_tree_timer(info : ParallelUpdateFrameDelayed;
                                   motion_matching__controller : MotionMatchingController;
                                   var animchar : AnimcharBaseComponent)
  if !(motion_matching__controller |> hasActiveAnimation())
    return
  let clipId = motion_matching__controller |> getCurrentClip()
  assume curClip = motion_matching__controller.dataBase.clips[clipId]
  let timeParamId = curClip.animTreeTimer
  if timeParamId < 0
    return
  assume animState = *animchar.animState
  let mmClipProgress = (float(motion_matching__controller |> getCurrentFrame()) +
                        motion_matching__controller |> getCurrentFrameProgress()) / float(curClip.tickDuration)
  if curClip.immediateAnimTreeTimerSync
    anim_state_holder_setParam(animState, timeParamId, mmClipProgress * curClip.animTreeTimerCycle)
    return
  let timeScaleParamId = anim_state_holder_getTimeScaleParamId(animState, timeParamId)
  if timeScaleParamId < 0
    return
  let timerProgress = anim_state_holder_getParam(animState, timeParamId) % curClip.animTreeTimerCycle / curClip.animTreeTimerCycle
  var timeScale = motion_matching__controller.playSpeedMult * curClip.animTreeTimerCycle / curClip.duration
  // Detect whatever is better to synchronize anitTree timer with MM clip in assumption that animations are looped.
  var accelerateRange = mmClipProgress - timerProgress
  if accelerateRange < 0.f
    accelerateRange = 1.f + accelerateRange
  let decelerateRange = 1.f - accelerateRange
  if accelerateRange < decelerateRange
    timeScale *= 1.f + accelerateRange
  else
    timeScale /= 1.f + decelerateRange
  anim_state_holder_setParam(animState, timeScaleParamId, timeScale)