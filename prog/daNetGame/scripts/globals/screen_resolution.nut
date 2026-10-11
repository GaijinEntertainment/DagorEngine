import "platform.nut" as platform
from "videomode" import is_vulkan, is_dx12
from "dagor.system" import get_primary_screen_info
from "settings" import get_setting_by_blk_path
from "types" import Array

function getPrimaryScreenResolution() {
  try {
    let { pixelsWidth, pixelsHeight } = get_primary_screen_info()
    if (pixelsWidth > 0 && pixelsHeight > 0)
      return [ pixelsWidth, pixelsHeight ]
  }
  catch(e) {}
  return [0, 0]
}

function isAutoResolutionOnPrimaryDisplay(mode, monitor) {
  if (platform.is_linux)
    return mode == "fullscreen"
  if (!platform.is_windows || monitor != "auto" || (mode != "fullscreen" && mode != "fullscreenwindowed"))
    return false
  if (is_dx12() || is_vulkan())
    return true
  return get_setting_by_blk_path("video/mode") != "fullscreen" || (get_setting_by_blk_path("video/resolution") ?? "auto") == "auto"
}

function guessTargetResolution(chosenResolution, chosenMode, chosenMonitor) {
  if (chosenResolution instanceof Array)
    return chosenResolution
  return isAutoResolutionOnPrimaryDisplay(chosenMode, chosenMonitor) ? getPrimaryScreenResolution() : [0, 0]
}

return freeze({ getPrimaryScreenResolution, isAutoResolutionOnPrimaryDisplay, guessTargetResolution })
