from "%darg/ui_imports.nut" import *
from "dagor.workcycle" import defer
from "simpleComponents.nut" import menuBtn
from "%sqstd/string.nut" import tostring_r

let mkMessageText = @(text) {
  size = [flex(), sh(40)]
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  padding = [sh(2), 0]
  clipChildren = true
  children = type(text)=="string" ? {
    size = [sw(50), SIZE_TO_CONTENT]
    rendObj = ROBJ_TEXTAREA
    behavior = Behaviors.TextArea
    preformatted = true
    halign = text.len()>100 ? null : ALIGN_CENTER
    text
  } : {size = [sw(50), SIZE_TO_CONTENT], children = text}
}

let widgets = persist($"msgbox_widgets", @() [])
let msgboxGeneration = persist($"msgbox_msgboxGeneration", @() Watched(0))
let hasMsgBoxes = Computed(@() msgboxGeneration.get() >= 0 && widgets.len() > 0)

function getCurMsgbox(){
  if (widgets.len()==0)
    return null
  return widgets.top()
}

//let log = getroottable()?.log ?? @(...) print(" ".join(vargv))

let incGen = @() msgboxGeneration.modify(@(v) v+1)
function addWidget(w) {
  widgets.append(w)
  defer(incGen)
}

function removeWidget(w, uid=null) {
  let idx = widgets.indexof(w) ?? (uid!=null ? widgets.findindex(@(v) v?.uid == uid) : null)
  if (idx == null)
    return
  widgets.remove(idx)
  incGen()
}

function removeAllMsgboxes() {
  widgets.clear()
  incGen()
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
  incGen()
  return true
}

function isMsgboxInList(uid) {
  return widgets.findindex(@(w) w.uid == uid) != null
}

let defaultButtons = [{text="OK" customStyle={hotkeys=[["^Esc | Enter"]]}}]

let animations = [
  { prop=AnimProp.opacity, from=0, to=1, duration=0.32, play=true, easing=OutCubic }
  { prop=AnimProp.scale,  from=[1, 0], to=[1,1], duration=0.25, play=true, easing=OutQuintic }

  { prop=AnimProp.opacity, from=1, to=0, duration=0.25, playFadeOut=true, easing=OutCubic }
  { prop=AnimProp.scale,  from=[1, 1], to=[1,0.5], duration=0.15, playFadeOut=true, easing=OutQuintic }
]

let showMsgbox = kwarg(function(text, onClose = null, buttons=null, uid = null) {
  log($"[MSGBOX] show: text = '{text}'")
  uid = uid ?? {}
  let msgbox = {v=null}

  function close() {
    removeWidget(msgbox.v, uid)
    onClose?()
    log($"[MSGBOX] closed: text = '{text}'")
  }

  let mkHandleButton = @(button_action) function() {
    button_action?()
    close()
  }

  let buttonsBlock = {
    flow = FLOW_HORIZONTAL
    gap = hdpx(40)
    children = (buttons ?? defaultButtons).map(@(desc) menuBtn(desc.text, mkHandleButton(desc?.onClick), desc?.customStyle))
  }

  let content = {
    rendObj = ROBJ_SOLID
    color = Color(30,30,30,250)
    size = [sw(100), sh(50)]
    vplace = ALIGN_CENTER
    padding = sh(2)
    key = uid
    flow = FLOW_VERTICAL
    halign = ALIGN_CENTER
    children = [
      mkMessageText(text)
      buttonsBlock
    ]
  }

  msgbox.v = {
    uid
    rendObj = ROBJ_SOLID
    size = [sw(100), sh(100)]
    color = Color(0, 0, 0, 200)
    behavior = Behaviors.Button
    transform = {}
    animations
    stopMouse = true
    children = content
    hotkeys = [["Esc", close]]
  }

  updateWidget(msgbox.v, uid)

  return msgbox
})

let msgboxComponent = @(){watch=msgboxGeneration children = getCurMsgbox() size = flex()}

return {
  showMsgbox
  getCurMsgbox
  msgboxGeneration
  removeAllMsgboxes
  isMsgboxInList
  removeMsgboxByUid
  msgboxComponent
  hasMsgBoxes
  showWarning = function(m) {
    let text = tostring_r(m)
    showMsgbox({text, uid=text})
  }
}
