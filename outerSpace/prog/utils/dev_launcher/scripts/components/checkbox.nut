from "%darg/ui_imports.nut" import *
import "style.nut" as style

let {setTooltip} = require("cursors.nut")

let CheckBoxContentHover = Color(255,255,255)
let CheckBoxContentDefault = Color(220,220,220)
let boxHeight = style.defControlHeight

let calcColor = @(sf)
  (sf & S_HOVER) ? CheckBoxContentHover : CheckBoxContentDefault

let checkMark = @() {
  rendObj = ROBJ_VECTOR_CANVAS
  size = [boxHeight,boxHeight]
  commands = [
    [VECTOR_FILL_COLOR, style.BTN_TXT_NORMAL],
    [VECTOR_WIDTH, 0],
    [VECTOR_ELLIPSE, 50, 50, 25, 25],
  ]
}

function switchbox(stateFlags, state, group, tooltip=null) {
  let onHover = tooltip!=null ? @(on) setTooltip(on ? tooltip : null) : null
  let checkMk = @() {
    watch = state
    children = state.get() ? checkMark : null
    vplace = ALIGN_CENTER
    margin = [0,0,0,hdpx(2)]
  }
  return function(){
    let sf = stateFlags.get()
    let switchKnob = {
      size = [boxHeight-hdpx(2),boxHeight-hdpx(2)] rendObj = ROBJ_BOX, borderRadius = hdpx(3),
      borderWidth = hdpx(1),
      fillColor = calcColor(sf)
      borderColor = Color(0,0,0,30)
      hplace = state.get() ? ALIGN_RIGHT : ALIGN_LEFT
      vplace = ALIGN_CENTER
      margin = hdpx(1)
    }
    return {
      behavior = Behaviors.Button
      onElemState = @(s) stateFlags.set(s)
      onClick = @() state.modify(@(v) !v)
      onHover
      group
      rendObj = ROBJ_BOX
      fillColor = sf & S_HOVER ? style.BTN_BG_HOVER : style.BTN_BG_NORMAL
      borderWidth = hdpx(1)
      borderColor = style.BTN_BD_NORMAL
      borderRadius = hdpx(3)
      watch = stateFlags
      size = [boxHeight*2+hdpx(4), boxHeight]
      children = [checkMk, switchKnob]
    }
  }
}

function label(text, group) {
  return {
    clipChildren = true
    group
    children = {
      behavior = Behaviors.Marquee
      group
      speed = [hdpx(40),hdpx(40)]
      children = {
        rendObj = ROBJ_TEXT
        text
        color = style.LABEL_TXT
      }
      scrollOnHover = true
      delay = 0.3
    }
  }
}

function mkCheckbox(state, label_text=null, params = null) {
  let stateFlags = Watched(0)
  let group = ElemGroup()
  return function(){
    return {
      flow = FLOW_HORIZONTAL
      valign = ALIGN_CENTER
      gap = sh(1)
      size = [flex(), SIZE_TO_CONTENT]
      children = [
        label(label_text, group),
        {size  =[flex(), 0]},
        switchbox(stateFlags, state, group, params?.tooltip)
      ]
    }
  }
}

return {mkCheckbox}