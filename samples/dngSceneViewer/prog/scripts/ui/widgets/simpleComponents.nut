from "%darg/ui_imports.nut" import *
from "%sqstd/underscore.nut" import flatten
from "math" import max

let dtext = @(text, style=null) {text, rendObj=ROBJ_TEXT, color = Color(180,180,180)}.__update(style ?? {})

let cursorC = Color(255,255,255,255)
let cursor = {
  rendObj = ROBJ_VECTOR_CANVAS
  size = [fsh(2), fsh(2)]
  hotspot = [0, 0]
  commands = [
    [VECTOR_WIDTH, hdpx(1)],
    [VECTOR_FILL_COLOR, cursorC],
    [VECTOR_COLOR, Color(20, 40, 70, 250)],
    [VECTOR_POLY, 0,0, 100,50, 56,56, 50,100],
  ]
    transform = {
      pivot = [0, 0]
      rotate = 29
    }
}

let normalCursor = Cursor(@() cursor)


function textBtn(text, onClick, style = null) {
  let stateFlags = Watched(0)
  let keyOnHover = {}
  let key = {}
  return @() {
    rendObj = ROBJ_TEXT
    hotkeys = stateFlags.get() & S_HOVER ? [["Enter | Space"]] : null
    key = stateFlags.get() & S_HOVER ? keyOnHover : key
    watch = stateFlags
    text
    behavior = Behaviors.Button
    onClick
    onElemState = @(s) stateFlags.set(s)
    color = stateFlags.get() & S_HOVER ? Color(255,255,255) : Color(150,250,200)
    fontSize = hdpx(40)
  }.__update(style ?? {})
}

let hflow = @(...) { flow = FLOW_HORIZONTAL children=flatten(vargv), gap = hdpx(10)}
let vflow = @(...) { flow = FLOW_VERTICAL children=flatten(vargv), gap = hdpx(5)}
let hudTxt  = @(text) {text, rendObj=ROBJ_TEXT, color = Color(150,250,150)}


let mkCorner = @(borderWidth, style=null) freeze({rendObj = ROBJ_FRAME borderWidth color = Color(105,105,105,105) size = [hdpx(10), hdpx(10)]}.__update(style ?? {}))
let ulCorner = mkCorner([1,0,0,1])
let urCorner = mkCorner([1,1,0,0], {hplace = ALIGN_RIGHT})
let drCorner = mkCorner([0,1,1,0], {hplace = ALIGN_RIGHT, vplace = ALIGN_BOTTOM})
let dlCorner = mkCorner([0,0,1,1], {vplace = ALIGN_BOTTOM})
let corners = freeze({children = [ulCorner, dlCorner, urCorner, drCorner] size = flex()})

let menuBtnTextColorNormal = Color(150,250,200)
let menuBtnTextColorHover = Color(255,255,255)
let menuBtnFillColorNormal = 0
let menuBtnFillColorHover = Color(50,50,50,60)

function menuBtn(text, onClick, style = null) {
  let stateFlags = Watched(0)
  let keyOnHover = {}
  let key = {}

  return @() {
    hotkeys = stateFlags.get() & S_HOVER ? [["Enter | Space"]] : null
    key = stateFlags.get() & S_HOVER ? keyOnHover : key
    watch = stateFlags
    behavior = Behaviors.Button
    onClick
    onElemState = @(s) stateFlags.set(s)
    rendObj = ROBJ_SOLID
    color = stateFlags.get() & S_HOVER ? menuBtnFillColorHover : menuBtnFillColorNormal
    children = [
      corners
      {
        padding = [hdpx(4), hdpx(10)]
        children = {
          rendObj = ROBJ_TEXT
          text
          color = stateFlags.get() & S_HOVER ? menuBtnTextColorHover : menuBtnTextColorNormal
          fontSize = hdpx(20)
        }
      }
    ]
  }.__update(style ?? {})
}

function input(text_state, state_flags, group, placeholder, fontSize) {
  let sf = state_flags.get()
  let setText = @(val) text_state.set(val)
  return @() {
    rendObj = ROBJ_BOX
    fillColor = (sf & S_KB_FOCUS) ? Color(10,10,10) : Color(0,0,0)
    borderWidth = hdpx(1)
    borderColor = (sf & S_HOVER) ? Color(195,195,195) : ((sf & S_KB_FOCUS) ? Color(155,155,155) : Color(40,40,40))
    group
    size = flex()
    padding = [hdpx(2), hdpx(5)]
    watch = [state_flags, text_state]
    valign = ALIGN_CENTER
    children = {
      rendObj = ROBJ_TEXT
      size = [flex(), SIZE_TO_CONTENT]
      group
      behavior = Behaviors.TextInput
      text = text_state.get()
      key = text_state
      onChange = setText
      fontSize = fontSize ?? hdpx(30)
      placeholder
      onEscape = setText
      color = (sf & S_KB_FOCUS) ? Color(255, 255, 255) : Color(160, 160, 160)
    }
  }
}
let textInput = kwarg(function(state, title=null, placeholder=null, width = hdpx(300), fontSize = hdpx(30)){
  let stateFlags = Watched(0)
  let group = ElemGroup()
  let titleCmp = title!=null ? {rendObj = ROBJ_TEXT text=title, fontSize} : null
  return function(){
    return {
      watch   = stateFlags
      group
      size = [width, fontSize*1.5]
      gap  = sh(2)
      fontSize
      flow = FLOW_HORIZONTAL
      valign = ALIGN_CENTER
      onElemState = @(sf) stateFlags.set(sf)
      children = [ titleCmp, input(state, stateFlags, group, placeholder, fontSize) ]
    }
  }
})
let headerTxt = @(text) freeze({rendObj = ROBJ_TEXT text fontSize = hdpx(70) color = Color(70,70,70,70)})

let itemGap = freeze({rendObj=ROBJ_FRAME size=[flex(),hdpx(1)] color=Color(90,90,90)})
function itemToOption(value){
  let t = type(value)
  let val = t!="table" ? value : value.value
  return {
    value = val
    text = t=="bool"
      ? (val ? "yes" : "no")
      : (t=="table" ? value.text : val)
  }
}

function listItem(text, onClick, isCurrent) {
  let stateFlags = Watched(0)
  return @() {
    rendObj = ROBJ_BOX
    onClick
    size = [flex(), SIZE_TO_CONTENT]
    minWidth = SIZE_TO_CONTENT
    behavior = Behaviors.Button
    watch = stateFlags
    onElemState = @(s) stateFlags.set(s)
    padding = [hdpx(5), hdpx(10)]
    fillColor = stateFlags.get() & S_HOVER ? menuBtnFillColorHover : menuBtnFillColorNormal
    children = {
      color = (stateFlags.get() & S_HOVER)
        ? menuBtnTextColorHover
        : isCurrent ? Color(255,255,255) : menuBtnTextColorNormal
      text
      rendObj = ROBJ_TEXT
    }
  }
}


function mkCombo(opt, _group=null, _xmbNode=null) {
  let watch = opt.var
  let values = opt.values
  let title = opt?.title
  let setValue = opt?.setValue ?? @(val) watch.set(val)
  let curVal = opt?.getValue ?? @() watch.get()
  let comboOpen = Watched(false)
  let close = @() comboOpen.set(false)
  local width = sw(3)

  function valueToOption(val) {
    let {value, text} = itemToOption(val)
    let cur = curVal()
    let curt = type(cur)
    let isCurrent = curt=="table" ? value == cur.value : value == cur
    function handler() {
      setValue(val)
      close()
    }
    let res = listItem(text, handler, isCurrent)
    width  = max(width, calc_comp_size(res)[0])
    return res
  }

  function list() {
    return {
      flow = FLOW_VERTICAL
      rendObj = ROBJ_SOLID
      color = Color(30,30,30)
      minWidth = width
      watch
      children = values.map(valueToOption)
      gap = itemGap
    }
  }
  let back = freeze({
    behavior = Behaviors.ComboPopup
    color = Color(0,0,0, 150)
    eventPassThrough = true
    onClick = close
    rendObj = ROBJ_SOLID pos = [-sw(101), -sh(101)] size = [sw(202), sh(202)]
  })

  function popup(){
    return {
      stopMouse = true
      size = [0,0]
      children = [back, list]
      zOrder = 1
    }
  }
  let stateFlags = Watched(0)
  let comboboxBtn = @() {
    watch = [watch, stateFlags]
    children = menuBtn(itemToOption(curVal()).text, @() comboOpen.set(true))
  }

  function comboBox(){
    return {
      watch = comboOpen
      children = [
        comboboxBtn
        comboOpen.get() ? popup : null
      ]
    }
  }
  return function() {
    return {
      flow = FLOW_HORIZONTAL
      size = [flex(), SIZE_TO_CONTENT]
      onDetach = close
      valign = ALIGN_CENTER
      gap = hdpx(10)
      children = [
        title == null ? null : {
          rendObj = ROBJ_TEXT
          text = title
        }
        comboBox
      ]
    }
  }
}

return {
  dtext
  headerTxt
  normalCursor
  mkWatchedText = @(watch) @(){watch, rendObj = ROBJ_TEXT, text=watch.get()}
  textBtn
  hflow
  vflow
  hudTxt
  textInput
  menuBtn
  mkCombo
  menuBtnTextColorNormal
  menuBtnTextColorHover
}