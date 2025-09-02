from "%darg/ui_imports.nut" import *
from "dagor.workcycle" import defer

let cursorC = Color(180,180,180,180)

function mkButton(desc, onClick) {
  let buttonGrp = ElemGroup()
  let stateFlags = Watched(0)
  return function(){
    let sf = stateFlags.get()
    return {
      key = desc
      behavior = Behaviors.Button
      //focusOnClick = true
      group = buttonGrp

      rendObj = ROBJ_BOX
      onElemState = @(v) stateFlags.set(v)
      fillColor = sf & S_ACTIVE ? Color(0,0,0,255)
        : sf & S_HOVER ? Color(90, 90, 80, 250)
        : Color(30, 30, 30, 200)

      borderWidth = sf & S_KB_FOCUS ? hdpx(2) : hdpx(1)
      onHover = function(on) { if(on) set_kb_focus(desc)}
      borderRadius = hdpx(4)
      borderColor = sf & S_KB_FOCUS
        ? Color(255,255,200,120)
        : Color(120,120,120,120)

      size = SIZE_TO_CONTENT
      margin = [sh(0.5), sh(1)]
      watch = stateFlags

      children = {
        rendObj = ROBJ_TEXT
        margin = sh(1)
        text = desc?.text ?? "???"
        group = buttonGrp
      }

      onClick
    }
  }
}

let cursor = static Cursor({
  rendObj = ROBJ_VECTOR_CANVAS
  size = [sh(2), sh(2)]
  commands = [
    [VECTOR_WIDTH, hdpx(1)],
    [VECTOR_FILL_COLOR, cursorC],
    [VECTOR_COLOR, Color(20, 40, 70, 250)],
    [VECTOR_POLY, 0,0, 100,50, 56,56, 50,100],
  ]
})

let Root = static {
  rendObj = ROBJ_SOLID
  color = Color(30,30,30,250)
  size = [sw(100), sh(50)]
  vplace = ALIGN_CENTER
  padding = sh(2)
  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
}

const closeKeys = "Esc"
const leftKeys = "Left"
const rightKeys = "Right | Tab"
const activateKeys = "Space | Enter"
const closeTxt = "Close"
const maskKeys = ""

let BgOverlay = static {
  rendObj = ROBJ_SOLID
  size = [sw(100), sh(100)]
  color = Color(0, 0, 0, 200)
  behavior = Behaviors.Button
  transform = {}
  cursor
  stopMouse = true
  animations = [
    { prop=AnimProp.opacity, from=0, to=1, duration=0.32, play=true, easing=OutCubic }
    { prop=AnimProp.scale,  from=[1, 0], to=[1,1], duration=0.25, play=true, easing=OutQuintic }

    { prop=AnimProp.opacity, from=1, to=0, duration=0.25, playFadeOut=true, easing=OutCubic }
    { prop=AnimProp.scale,  from=[1, 1], to=[1,0.5], duration=0.15, playFadeOut=true, easing=OutQuintic }
  ]
}

function messageText(params) {
  return {
    size = flex()
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    padding = static [sh(2), 0]
    children = {
      size = FLEX_H
      rendObj = ROBJ_TEXTAREA
      behavior = Behaviors.TextArea
      halign = ALIGN_CENTER
      text = params?.text
    }
  }
}


let widgets = persist("editor_widgets", @() [])
let msgboxGeneration = persist("editor_msgboxGeneration", @() Watched(0))
let hasMsgBoxes = Computed(@() msgboxGeneration.get() >= 0 && widgets.len() > 0)

function getCurMsgbox(){
  if (widgets.len()==0)
    return null
  return widgets.top()
}

function addWidget(w) {
  widgets.append(w)
  defer(@() msgboxGeneration.set(msgboxGeneration.get()+1))
}

function removeWidget(w, uid=null) {
  let idx = widgets.indexof(w) ?? (uid!=null ? widgets.findindex(@(v) v?.uid == uid) : null)
  if (idx == null)
    return
  widgets.remove(idx)
  msgboxGeneration.set(msgboxGeneration.get()+1)
}

function removeAllMsgboxes() {
  widgets.clear()
  msgboxGeneration.set(msgboxGeneration.get()+1)
}

function updateWidget(w, uid){
  let idx = widgets.findindex(@(wd) wd.uid == uid)
  if (idx == null)
    addWidget(w)
  else {
    widgets.remove(idx)
    addWidget(w)
  }
}

function removeMsgboxByUid(uid) {
  let idx = widgets.findindex(@(w) w.uid == uid)
  if (idx == null)
    return false
  widgets.remove(idx)
  msgboxGeneration.set(msgboxGeneration.get()+1)
  return true
}

function isMsgboxInList(uid) {
  return widgets.findindex(@(w) w.uid == uid) != null
}

/// Adds messagebox to widgets list
/// params = {
///   text = 'message text'
///   onClose = function() {} // optional close event callback
///   buttons = [   // array
///      {
///         text = 'button_caption'
///         action = function() {} // click handler
///         isCurrent = false // flag for focused button
///         isCancel = false // flag for button activated by Esc
///      }
///      ...
///   ]
///

let skip = {skip=true}
let skpdescr = {description = skip}
let defaultButtons = [{text="OK" customStyle={hotkeys=[["^Esc | Enter", skpdescr]]}}]


function showMsgbox(params) {
  log($"[MSGBOX] show: text = '{params?.text}'")
  let self = {v = null}
  let uid = params?.uid ?? {}

  function doClose() {
    removeWidget(self.v, uid)
    if ("onClose" in params && params.onClose)
      params.onClose()

    log($"[MSGBOX] closed: text = '{params?.text}'")
  }

  function handleButton(button_action) {
    if (button_action) {
      if (button_action?.getfuncinfos?().parameters.len()==2) {
        // handler performs closing itself
        button_action({doClose})
        return // stop processing, handler will do everything what is needed
      }

      button_action()
    }

    doClose()
  }

  local btnsDesc = params?.buttons ?? defaultButtons
  if (!(isObservable(btnsDesc)))
    btnsDesc = Watched(btnsDesc, FRP_DONT_CHECK_NESTED)

  local defCancel = null
  local initialBtnIdx = 0

  foreach (idx, bd in btnsDesc.get()) {
    if (bd?.isCurrent)
      initialBtnIdx = idx
    if (bd?.isCancel)
      defCancel = bd
  }

  let curBtnIdx = Watched(initialBtnIdx)

  function moveBtnFocus(dir) {
    curBtnIdx.set((curBtnIdx.get() + dir + btnsDesc.get().len()) % btnsDesc.get().len())
  }

  function activateCurBtn() {
    log($"[MSGBOX] handling active '{btnsDesc.get()[curBtnIdx.get()]?.text}' button: text = '{params?.text}'")
    handleButton(btnsDesc.get()[curBtnIdx.get()]?.action)
  }

  let buttonsBlockKey = {}

  function buttonsBlock() {
    return {
      watch = [curBtnIdx, btnsDesc]
      key = buttonsBlockKey
      size = SIZE_TO_CONTENT
      flow = FLOW_HORIZONTAL
      gap = hdpx(40)

      children = btnsDesc.get().map(function(desc, idx) {
        let conHover = desc?.onHover
        function onHover(on){
          if (!on)
            return
          curBtnIdx.set(idx)
          conHover?()
        }
        local behavior = desc?.customStyle?.behavior ?? desc?.customStyle?.behavior
        behavior = type(behavior) == "array" ? behavior : [behavior]
        behavior.append(Behaviors.Button)
        let customStyle = (desc?.customStyle ?? {}).__merge({
          onHover
          behavior
        })
        function onClick() {
          log($"[MSGBOX] clicked '{desc?.text}' button: text = '{params?.text}'")
          handleButton(desc?.action)
        }
        return mkButton(desc.__merge({customStyle, key=desc}), onClick)
      })

      hotkeys = [
        [closeKeys, {action= @() handleButton(params?.onCancel ?? defCancel?.action), description = closeTxt}],
        [rightKeys, {action = @() moveBtnFocus(1) description = skip}],
        [leftKeys, {action = @() moveBtnFocus(-1) description = skip}],
        [activateKeys, {action= activateCurBtn, description= skip}],
        [maskKeys, {action = @() null, description = skip}]
      ]
    }
  }

  let root = Root.__merge({
    key = uid
    children = [
      messageText(params.__merge({ handleButton }))
      buttonsBlock
    ]
  })

  self.v = BgOverlay.__merge({
    uid
    children = root
  })

  updateWidget(self.v, uid)

  return self
}
let msgboxComponent = @(){watch=msgboxGeneration children = getCurMsgbox()}

return {
  showMsgbox
  getCurMsgbox
  msgboxGeneration
  removeAllMsgboxes
  isMsgboxInList
  removeMsgboxByUid
  msgboxComponent
  hasMsgBoxes
}
