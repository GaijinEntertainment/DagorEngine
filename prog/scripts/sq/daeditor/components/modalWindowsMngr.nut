from "%darg/ui_imports.nut" import *

let WND_PARAMS = {
  key = null //generate automatically when not set
  children= null
  onClick = null //remove current modal window when not set

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

let function mkModalWindowsMngr(wndParams = null){
  let modalWindows = []
  let modalWindowsGeneration = Watched(0)
  let hasModalWindows = Computed(@() modalWindowsGeneration.value >= 0 && modalWindows.len() > 0)
  wndParams = WND_PARAMS.__merge(wndParams ?? {})
  let function removeModalWindow(key) {
    let idx = modalWindows.findindex(@(w) w.key == key)
    if (idx == null)
      return false
    modalWindows.remove(idx)
    modalWindowsGeneration(modalWindowsGeneration.value+1)
    return true
  }

  local lastWndIdx = 0
  let function addModalWindow(wnd = WND_PARAMS) {
    wnd = wndParams.__merge(wnd)
    if (wnd.key != null)
      removeModalWindow(wnd.key)
    else {
      lastWndIdx++
      wnd.key = $"modal_wnd_{lastWndIdx}"
    }
    wnd.onClick = wnd.onClick ?? @() removeModalWindow(wnd.key)
    modalWindows.append(wnd)
    modalWindowsGeneration(modalWindowsGeneration.value+1)
  }

  let function hideAllModalWindows() {
    if (modalWindows.len() == 0)
      return
    modalWindows.clear()
    modalWindowsGeneration(modalWindowsGeneration.value+1)
  }

  let modalWindowsComponent = @() {
    watch = modalWindowsGeneration
    size = flex()
    children = modalWindows
  }

  return {
    addModalWindow
    removeModalWindow
    hideAllModalWindows
    modalWindowsComponent
    hasModalWindows
  }
}

return mkModalWindowsMngr
