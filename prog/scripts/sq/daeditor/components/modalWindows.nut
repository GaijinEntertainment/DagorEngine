from "%darg/ui_imports.nut" import *

let { getNumberOfRegisteredWindows } = require("%daeditor/components/window.nut")

let WND_PARAMS = const {
  key = null //generate automatically when not set
  children = null
  onClick = null //remove current modal window when not set
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  rendObj = ROBJ_WORLD_BLUR_PANEL
  size = flex()
  behavior = Behaviors.Button
  stopMouse = true
  stopHotkeys = true
  hotkeys = [["Esc"]]
  animations = [
    { prop=AnimProp.opacity, from=0.0, to=1.0, duration=0.15, play=true, easing=OutCubic }
    { prop=AnimProp.opacity, from=1.0, to=0.0, duration=0.125, playFadeOut=true, easing=OutCubic }
  ]
}

let modalWindows = []
let modalWindowsGeneration = Watched(0)
let hasModalWindows = Computed(@() modalWindowsGeneration.get() >= 0 && modalWindows.len() > 0)

let nextModalGeneration = @() modalWindowsGeneration.modify(@(v) v + 1)

function removeModalWindow(key) {
  let idx = modalWindows.findindex(@(w) w.key == key)
  if (idx == null)
    return false

  modalWindows.remove(idx)
  nextModalGeneration()
  return true
}

local lastWndIdx = 0
function addModalWindow(wnd = null) {
  wnd = WND_PARAMS.__merge(wnd ?? {})
  if (wnd?.zOrder == null) {
    wnd.zOrder <- getNumberOfRegisteredWindows() + 1
  }
  if (wnd.key != null)
    removeModalWindow(wnd.key)
  else {
    lastWndIdx++
    wnd.key = $"modal_wnd_{lastWndIdx}"
  }
  wnd.onClick = wnd.onClick ?? @() removeModalWindow(wnd.key)
  modalWindows.append(wnd)
  nextModalGeneration()
}

function hideAllModalWindows() {
  if (modalWindows.len() == 0)
    return

  modalWindows.clear()
  nextModalGeneration()
}

let modalWindowsComponent = @() {
  watch = modalWindowsGeneration
  size = const flex()
  children = modalWindows
}

return {
  addModalWindow
  removeModalWindow
  hideAllModalWindows
  modalWindowsComponent
  hasModalWindows
}
