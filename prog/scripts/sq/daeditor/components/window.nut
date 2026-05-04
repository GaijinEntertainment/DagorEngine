from "%darg/ui_imports.nut" import *
import "math" as math
import "cursors.nut" as cursors
from "%sqstd/underscore.nut" import isEqual

let closeButton = require("closeButton.nut")


let windowsOrder = persist("windowsOrder", @() [])
let windowsGeneration = Watched(0)
let incGen = @() windowsGeneration.modify(@(v) v+1)

let registeredWindows = {}

let windowsManager = @() {
  watch = windowsGeneration
  size = flex()
  children = {
    children = windowsOrder.map(@(v) registeredWindows?[v])
  }
}

function updateWindowOrder(newOrder){
  if (!isEqual(newOrder, windowsOrder)) {
    windowsOrder.replace(newOrder)
    incGen()
  }
}

function showWindow(id){
  if (type(id) == "string"  && id not in registeredWindows)
    logerr($"window {id} is not registered")
  if (type(id) != "string")
    id = id.key
  if (!windowsOrder.contains(id))
    updateWindowOrder((clone windowsOrder).append(id))
  else {
    updateWindowOrder(windowsOrder.filter(@(v) v != id).append(id))
  }
}

function hideWindow(window){
  updateWindowOrder(windowsOrder.filter(@(v) v != (window?.key ?? window)))
}

function hideAllWindows(){
  updateWindowOrder([])
}

function toggleWindow(window){
  if (windowsOrder.contains(window?.key ?? window))
    hideWindow(window)
  else
    showWindow(window)
}

function mkHeader(id, headerText, onClose) {
  return {
    size = [flex(), SIZE_TO_CONTENT]
    rendObj = ROBJ_BOX
    flow = FLOW_HORIZONTAL
    fillColor = Color(0,10,20,210)
    borderColor = Color(30,30,30,20)
    borderWidth = hdpx(1)
    padding = [0,hdpx(5)]

    children = [
      {
        size = [flex(), SIZE_TO_CONTENT]
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        rendObj = ROBJ_TEXT
        text = headerText
        margin = [hdpx(5), 0]
      }
      {
        key = id
        halign = ALIGN_RIGHT
        valign = ALIGN_CENTER
        children = closeButton(function() {
          hideWindow(id)
          onClose()
        })
      }
    ]
  }
}

let isWindowVisible = @(window) windowsOrder.contains(window?.key ?? window)
let unused = @(...) null

let mkIsWindowVisible = @(id_or_window) Computed(function(){
  unused(windowsGeneration.get())
  return isWindowVisible(id_or_window)
})

let windowsStates = persist("windowStates", @() {})

let mkWindow = kwarg(function(id, content=null, mkContent=null,
      onAttach=null, initialSize = const [sw(40), sh(65)], minSize = const [sw(14), sh(25)], maxSize = const [sw(80), sh(90)],
      windowStyle = null, saveState=false, onClose = @() null, headerText = ""
  ) {
  assert(content!=null || type(mkContent)=="function", "registerWindow should be called with 'content' or 'mkContent'")
  let initialState = {
    pos = [sw(50)-initialSize[0]/2 + registeredWindows.len()*sh(5), sh(46)-initialSize[1]/2+registeredWindows.len()*sh(2)]
    size = initialSize
  }
  let windowState = Watched(windowsStates?[id] ?? initialState)
  if (saveState)
    windowsStates[id] <- windowState.get()

  function onMoveResize(dx, dy, dw, dh) {
    let w = windowState.get()
    let pos = clone w.pos
    let size = clone w.size
    pos[0] = math.clamp(pos[0]+dx, -size[0]/2, (sw(100)-size[0]/2))
    pos[1] = math.clamp(pos[1]+dy, 0, (sh(100)-size[1]/2))
    size[0] = math.clamp(size[0]+dw, minSize[0], maxSize[0])
    size[1] = math.clamp(size[1]+dh, minSize[1], maxSize[1])
    w.pos = pos
    w.size = size
    if (saveState && !isEqual(w, windowsStates[id]))
      windowsStates[id] = w
    return w
  }
  let contentItem = @() {
    padding = fsh(0.5)
    children = content ?? mkContent()
    size = flex()
    key = id
    clipChildren = true
  }
  let windowFrame = @() {
    rendObj = ROBJ_WORLD_BLUR_PANEL
    fillColor = const Color(50,50,50,220)
    onMoveResize = onMoveResize
    onMoveResizeStarted = function(_0, _1, _2) { showWindow(id) } // bring to top
    size = windowState.get().size
    pos = windowState.get().pos
    moveResizeCursors = cursors.moveResizeCursors
    behavior = [Behaviors.MoveResize]
    key = id
    stopMouse = true
    watch = [windowsGeneration]
    zOrder = windowsOrder.findindex(@(v) v == id) ?? 0
    flow = FLOW_VERTICAL
    children = [
      mkHeader(id, headerText, onClose)
      contentItem
    ]
  }.__update(windowStyle ?? {})

  let window = {
    onAttach
    size = flex()
    key = id
    children = windowFrame
  }
  return window
})

function registerWindow(params) {
  let window =mkWindow(params)
  registeredWindows[params.id] <-window
  return window
}

function getNumberOfRegisteredWindows() {
  return registeredWindows.len()
}

return {
  registerWindow
  showWindow
  windowsManager
  hideAllWindows
  hideWindow
  toggleWindow
  isWindowVisible
  mkIsWindowVisible
  mkWindow
  getNumberOfRegisteredWindows
}
