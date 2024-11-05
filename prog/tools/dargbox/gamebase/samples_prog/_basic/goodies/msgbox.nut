from "%darg/ui_imports.nut" import *
from "dagor.workcycle" import defer

let cursorC = Color(180,180,180,180)

let dstyling = {
  cursor = Cursor({
    rendObj = ROBJ_VECTOR_CANVAS
    size = [sh(2), sh(2)]
    commands = [
      [VECTOR_WIDTH, hdpx(1)],
      [VECTOR_FILL_COLOR, cursorC],
      [VECTOR_COLOR, Color(20, 40, 70, 250)],
      [VECTOR_POLY, 0,0, 100,50, 56,56, 50,100],
    ]
  })

  Root = {
    rendObj = ROBJ_SOLID
    color = Color(30,30,30,250)
    size = [sw(100), sh(50)]
    vplace = ALIGN_CENTER
    padding = sh(2)
  }

  closeKeys = "Esc"
  leftKeys = "Left"
  rightKeys = "Right | Tab"
  activateKeys = "Space | Enter"
  closeTxt = "Close"
  maskKeys = ""
  BgOverlay = {
    rendObj = ROBJ_SOLID
    size = [sw(100), sh(100)]
    color = Color(0, 0, 0, 200)
    behavior = Behaviors.Button
    transform = {}
    animations = [
      { prop=AnimProp.opacity, from=0, to=1, duration=0.32, play=true, easing=OutCubic }
      { prop=AnimProp.scale,  from=[1, 0], to=[1,1], duration=0.25, play=true, easing=OutQuintic }

      { prop=AnimProp.opacity, from=1, to=0, duration=0.25, playFadeOut=true, easing=OutCubic }
      { prop=AnimProp.scale,  from=[1, 1], to=[1,0.5], duration=0.15, playFadeOut=true, easing=OutQuintic }
    ]
  }

  button = function(desc, on_click) {
    let buttonGrp = ElemGroup()
    let stateFlags = Watched(0)
    return function(){
      let sf = stateFlags.value
      return {
        key = desc
        behavior = Behaviors.Button
        //focusOnClick = true
        group = buttonGrp

        rendObj = ROBJ_BOX
        onElemState = @(v) stateFlags(v)
        fillColor = (sf & S_HOVER)
                    ? (sf & S_ACTIVE)
                      ? Color(0,0,0,255)
                      : Color(90, 90, 80, 250)
                    : Color(30, 30, 30, 200)

        borderWidth = (sf & S_KB_FOCUS) ? hdpx(2) : hdpx(1)
        onHover = function(on) { if(on) set_kb_focus(desc)}
        borderRadius = hdpx(4)
        borderColor = (sf & S_KB_FOCUS)
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

        onClick = on_click
      }
    }
  }

  messageText = function(params) {
    return {
      size = flex()
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      padding = [sh(2), 0]
      children = {
        size = [flex(), SIZE_TO_CONTENT]
        rendObj = ROBJ_TEXTAREA
        behavior = Behaviors.TextArea
        halign = ALIGN_CENTER
        text = params?.text
      }
    }
  }

}

function mkMsgbox(id, defStyling=dstyling){
  let widgets = persist($"{id}_widgets", @() [])
  let msgboxGeneration = mkWatched(persist, $"{id}_msgboxGeneration", 0)
  let hasMsgBoxes = Computed(@() msgboxGeneration.value >= 0 && widgets.len() > 0)

  function getCurMsgbox(){
    if (widgets.len()==0)
      return null
    return widgets.top()
  }

  //let log = getroottable()?.log ?? @(...) print(" ".join(vargv))

  function addWidget(w) {
    widgets.append(w)
    defer(@() msgboxGeneration(msgboxGeneration.value+1))
  }

  function removeWidget(w, uid=null) {
    let idx = widgets.indexof(w) ?? (uid!=null ? widgets.findindex(@(v) v?.uid == uid) : null)
    if (idx == null)
      return
    widgets.remove(idx)
    msgboxGeneration(msgboxGeneration.value+1)
  }

  function removeAllMsgboxes() {
    widgets.clear()
    msgboxGeneration(msgboxGeneration.value+1)
  }

  function updateWidget(w, uid){
    let idx = widgets.findindex(@(w) w.uid == uid)
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
    msgboxGeneration(msgboxGeneration.value+1)
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


  function show(params, styling=defStyling) {
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

    foreach (idx, bd in btnsDesc.value) {
      if (bd?.isCurrent)
        initialBtnIdx = idx
      if (bd?.isCancel)
        defCancel = bd
    }

    let curBtnIdx = Watched(initialBtnIdx)

    function moveBtnFocus(dir) {
      curBtnIdx.update((curBtnIdx.value + dir + btnsDesc.value.len()) % btnsDesc.value.len())
    }

    function activateCurBtn() {
      log($"[MSGBOX] handling active '{btnsDesc.value[curBtnIdx.value]?.text}' button: text = '{params?.text}'")
      handleButton(btnsDesc.value[curBtnIdx.value]?.action)
    }

    let buttonsBlockKey = {}

    function buttonsBlock() {
      return @() {
        watch = [curBtnIdx, btnsDesc]
        key = buttonsBlockKey
        size = SIZE_TO_CONTENT
        flow = FLOW_HORIZONTAL
        gap = hdpx(40)

        children = btnsDesc.value.map(function(desc, idx) {
          let conHover = desc?.onHover
          function onHover(on){
            if (!on)
              return
            curBtnIdx.update(idx)
            conHover?()
          }
          let onRecalcLayout = (initialBtnIdx==idx)
            ? function(initial, elem) {
                if (initial && styling?.moveMouseCursor.value)
                  move_mouse_cursor(elem)
              }
            : null
          local behaviors = desc?.customStyle?.behavior ?? desc?.customStyle?.behavior
          behaviors = type(behaviors) == "array" ? behaviors : [behaviors]
          behaviors.append(Behaviors.RecalcHandler, Behaviors.Button)
          let customStyle = (desc?.customStyle ?? {}).__merge({
            onHover = onHover
            behavior = behaviors
            onRecalcLayout = onRecalcLayout
          })
          function onClick() {
            log($"[MSGBOX] clicked '{desc?.text}' button: text = '{params?.text}'")
            handleButton(desc?.action)
          }
          return styling.button(desc.__merge({customStyle = customStyle, key=desc}), onClick)
        })

        hotkeys = [
          [styling?.closeKeys ?? "Esc", {action= @() handleButton(params?.onCancel ?? defCancel?.action), description = styling?.closeTxt}],
          [styling?.rightKeys ?? "Right | Tab", {action = @() moveBtnFocus(1) description = skip}],
          [styling?.leftKeys ?? "Left", {action = @() moveBtnFocus(-1) description = skip}],
          [styling?.activateKeys ?? "Space | Enter", {action= activateCurBtn, description= skip}],
          [styling?.maskKeys ?? "", {action = @() null, description = skip}]
        ]
      }
    }

    let root = styling.Root.__merge({
      key = uid
      flow = FLOW_VERTICAL
      halign = ALIGN_CENTER
      children = [
        styling.messageText(params.__merge({ handleButton = handleButton }))
        buttonsBlock()
      ]
    })

    self.v = styling.BgOverlay.__merge({
      uid
      cursor = styling.cursor
      stopMouse = true
      children = [styling.BgOverlay?.children, root]
    })

    updateWidget(self.v, uid)

    return self
  }
  let msgboxComponent = @(){watch=msgboxGeneration children = getCurMsgbox()}

  return {
    show
    showMsgbox = show
    getCurMsgbox
    msgboxGeneration
    removeAllMsgboxes
    isMsgboxInList
    removeMsgboxByUid
    msgboxComponent
    styling = defStyling
    hasMsgBoxes
  }
}
return {
  mkMsgbox
  msgbox = mkMsgbox("msgbox")
}