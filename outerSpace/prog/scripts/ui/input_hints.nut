from "%darg/ui_imports.nut" import *
import "dainput2" as dainput

let {dtext} = require("%scripts/ui/widgets/simpleComponents.nut")
let format_ctrl_name = dainput.format_ctrl_name
let loc = @(text) text

let sticksAliases = {
  lx = ["J:L.Thumb.h", "J:Axis1"]
  ly = ["J:L.Thumb.v", "J:Axis2"]
  rx = ["J:R.Thumb.h", "J:Axis3"]
  ry = ["J:R.Thumb.v", "J:Axis4"]
}

function mkText(text, _params={}){
  if (text==null || text=="")
    return null
  return dtext(loc(text))
}

let validDevices = [dainput.DEV_kbd, dainput.DEV_pointing, dainput.DEV_gamepad, dainput.DEV_joy]
function isValidDevice(dev) {
  return validDevices.indexof(dev) != null
}

let ButtonAnd = class{
  function _tostring(){
    return "+"
  }
}
function buildModifiersList(binding) {
  let res = []
  let mods = binding.mod
  for (local i=0, n=binding.modCnt; i<n; ++i) {
    if (isValidDevice(mods[i].devId)) {
      res.append(format_ctrl_name(mods[i].devId, mods[i].btnId))
      res.append(ButtonAnd())
    }
  }
  return res
}

let inParents = @(locId) "({0})".subst(locId)
let eventTypeMap = {
  [dainput.BTN_pressed] = null,
  [dainput.BTN_pressed_long] = "controls/digital/mode/hold",
  [dainput.BTN_pressed2] = "controls/digital/multiClick/2",
  [dainput.BTN_pressed3] = "controls/digital/multiClick/3",
  [dainput.BTN_released] = "controls/digital/release",
  [dainput.BTN_released_short] = "controls/digital/release_short",
  [dainput.BTN_released_long] = "controls/digital/hold_release"
}

let eventTypeLabels = {
  [dainput.BTN_pressed] = "controls/digital/onPressed",
  [dainput.BTN_pressed_long] = "controls/digital/onHold",
  [dainput.BTN_pressed2] = "controls/digital/multiClick/2",
  [dainput.BTN_pressed3] = "controls/digital/multiClick/3",
  [dainput.BTN_released] = "controls/digital/checkReleased",
  [dainput.BTN_released_short] = "controls/digital/onClick",
  [dainput.BTN_released_long] = "controls/digital/onHoldReleased"
}
let notImportantEventsTexts = [dainput.BTN_released_short, dainput.BTN_released].map(@(v) eventTypeMap[v])

const axesSeparatorTxt = "/"
const axesGroupSeparatorTxt = ";"

function getSticksText(stickBinding) {
  let b = stickBinding
  let xaxis = format_ctrl_name(b.devId, b.axisXId, false)
  let yaxis = format_ctrl_name(b.devId, b.axisYId, false)
  if (sticksAliases.rx.indexof(xaxis)!=null && sticksAliases.ry.indexof(yaxis)!=null)
    return "J:R.Thumb.hv"
  if (sticksAliases.lx.indexof(xaxis)!=null && sticksAliases.ly.indexof(yaxis)!=null)
    return "J:L.Thumb.hv"
  if (xaxis == "M:X" && yaxis=="M:Y")
    return "M:XY"
  return null
}


function mkBuildDigitalBindingText(b, eventTypeToText=true) {
  let res = buildModifiersList(b).map(@(v) eventTypeToText? v.tostring() : v)
  res.append(format_ctrl_name(b.devId, b.ctrlId, b.btnCtrl))
  let etype = eventTypeToText ? eventTypeMap?[b.eventType] : b.eventType
  if (etype)
    res.append(etype)
  return res
}
let buildDigitalBindingText = @(b) mkBuildDigitalBindingText(b)

function textListFromAction(action_name, column, eventTypeToText=true) {
  let ah = dainput.get_action_handle(action_name, 0xFFFF)
  let digitalBinding = dainput.get_digital_action_binding(ah, column)
  let axisBinding = dainput.get_analog_axis_action_binding(ah, column)
  let stickBinding = dainput.get_analog_stick_action_binding(ah, column)

  local res
  if (digitalBinding) {
    res = mkBuildDigitalBindingText(digitalBinding, eventTypeToText)
  }
  else if (axisBinding) {
    let b = axisBinding
    res = buildModifiersList(b)
    if (isValidDevice(b.devId)) {
      let text = format_ctrl_name(b.devId, b.axisId, false)
      res.append(text)
    }

    if (dainput.get_action_type(ah) == dainput.TYPE_STEERWHEEL) {
      if (isValidDevice(b.minBtn.devId) || isValidDevice(b.maxBtn.devId)) {
        res.append(format_ctrl_name(b.minBtn.devId, b.minBtn.btnId, true))
        res.append(axesSeparatorTxt)
        res.append(format_ctrl_name(b.maxBtn.devId, b.maxBtn.btnId, true))
      }
    }
    else {
      if (dainput.is_action_stateful(ah)){
        local oneGroupExist = false
        if (isValidDevice(b.incBtn.devId) || isValidDevice(b.decBtn.devId)) {
          res.append(format_ctrl_name(b.decBtn.devId, b.decBtn.btnId, true))
          res.append(axesSeparatorTxt)
          res.append(format_ctrl_name(b.incBtn.devId, b.incBtn.btnId, true))
          oneGroupExist = true
        }
        if (isValidDevice(b.minBtn.devId) || isValidDevice(b.maxBtn.devId)) {
          if (oneGroupExist)
            res.append(axesGroupSeparatorTxt)
          res.append(format_ctrl_name(b.minBtn.devId, b.minBtn.btnId, true))
          res.append(axesSeparatorTxt)
          res.append(format_ctrl_name(b.maxBtn.devId, b.maxBtn.btnId, true))
        }
      }
      else {
        if (isValidDevice(b.maxBtn.devId)) {
          res.append(format_ctrl_name(b.maxBtn.devId, b.maxBtn.btnId, true))
        }
      }
    }
  }
  else if (stickBinding) {
    let b = stickBinding
    res = buildModifiersList(b)
    if (isValidDevice(b.devId)) {
      let sticksText = getSticksText(b)
      if (sticksText != null) {
        res.append(sticksText)
      }
      else {
        res.append(format_ctrl_name(b.devId, b.axisXId, false))
        res.append(axesSeparatorTxt)
        res.append(format_ctrl_name(b.devId, b.axisYId, false))
      }
    }
    if (b.maxYBtn.devId)
      res.append(format_ctrl_name(b.maxYBtn.devId, b.maxYBtn.btnId, true))
    if (b.minXBtn.devId)
      res.append(format_ctrl_name(b.minXBtn.devId, b.minXBtn.btnId, true))
    if (b.minYBtn.devId)
      res.append(format_ctrl_name(b.minYBtn.devId, b.minYBtn.btnId, true))
    if (b.maxXBtn.devId)
      res.append(format_ctrl_name(b.maxXBtn.devId, b.maxXBtn.btnId, true))
  }
  else
    res = []
  return res
}
let eventTypeValues = eventTypeMap.values()

function buildElemsFromListOfAction(textlist, params = {imgFunc=null, textFunc=mkText, eventTextFunc = null, compact=false}){
  let textFunc = params?.textFunc ?? mkText
  let eventTextFunc = params?.eventTextFunc ?? textFunc
  let compact = params?.compact
  if (compact) {
    textlist = textlist.filter(@(text) notImportantEventsTexts.indexof(text)==null)
  }
  let elems = textlist.map(function(text){
    return function(){
      if (eventTypeValues.indexof(text)!=null)
        return eventTextFunc?(inParents(loc(text)))

      else if(type(text)=="string")
        return textFunc(loc(text))
      else
        return null
    }
  })
  return elems
}

function hasBinding(actionName, column=0){
  return dainput.is_action_binding_set(dainput.get_action_handle(actionName, 0xFFFF), column)
}

let getActionTags = memoize(@(action_handler)
  dainput.get_group_tag_str_for_action(action_handler).split(",").map(@(v) v.replace(" ","")).filter(@(v) v!=""))

function getControlsByGroupTag(groupTag, column = 0){
  let res = []
  let total = dainput.get_actions_count()
  for (local i = 0; i < total; i++) {
    let ah = dainput.get_action_handle_by_ord(i)
    if (!dainput.is_action_internal(ah)){
      let tags = getActionTags(ah)
      if (!tags.contains(groupTag))
        continue
      let actionName = dainput.get_action_name(ah)
      res.append([actionName, textListFromAction(actionName, column)])
    }
  }
  return res
}

let hflow = @(children) {children, flow = FLOW_HORIZONTAL}
function mkActionText(v){
  let [name, actionList] = v
  return {
    flow = FLOW_HORIZONTAL children = [dtext(name, {color = Color(80,220,120)}), dtext(": ", {color = Color(80,220,120)}), hflow(buildElemsFromListOfAction(actionList))]
  }
}

return {
  buildModifiersList
  buildDigitalBindingText
  notImportantEventsTexts
  textListFromAction
  eventTypeLabels
  getSticksText
  hasBinding
  buildElemsFromListOfAction
  getControlsByGroupTag
  mkActionText
}
