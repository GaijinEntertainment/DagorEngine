from "%darg/ui_imports.nut" import *
from "%sqstd/ecs.nut" import *

let {colors, gridHeight, gridMargin} = require("style.nut")
let {compValToString, isValueTextValid, convertTextToVal, setValToObj, getValFromObj, isCompReadOnly} = require("attrUtil.nut")
let entity_editor = require("entity_editor")

let getCompVal = @(eid, comp_name, path) path!=null ? getValFromObj(eid, comp_name, path) : _dbg_get_comp_val_inspect(eid, comp_name)

let function fieldEditText_(params={}) {
  let {eid, comp_name, compVal, setVal, path, rawComponentName=null} = params
  local curRO = isCompReadOnly(eid, rawComponentName)

  let curText = Watched(compValToString(compVal))
  let isFocus = Watched(false)
  let group = ElemGroup()
  let compType = typeof compVal
  let stateFlags = Watched(0)
  let isValid = Computed(@() isValueTextValid(compType, curText.value))

  let function onChange(text){
    curText.update(text)
  }

  let function updateTextFromEcs() {
    let val = getCompVal(eid, rawComponentName, path)
    let compTextVal = compValToString(val)
    curText.update(compTextVal)
  }

  let function updateTextFromEcsTimeout() {
    updateTextFromEcs()
    if (!isFocus.value)
      gui_scene.resetTimeout(0.1, updateTextFromEcsTimeout)
  }

  let function updateComponentByTimer() {
    if (rawComponentName == "transform") {
      if (isFocus.value)
        gui_scene.clearTimer(updateTextFromEcsTimeout)
      else
        gui_scene.resetTimeout(0.1, updateTextFromEcsTimeout)
    }
  }

  isFocus.subscribe(@(_v) updateComponentByTimer())
  updateComponentByTimer()

  let function frame() {
    let frameColor = (stateFlags.value & S_KB_FOCUS) ? colors.FrameActive : colors.FrameDefault
    return {
      rendObj = ROBJ_FRAME group=group size = [flex(), gridHeight] color = frameColor watch = stateFlags
      onElemState = @(sf) stateFlags.update(sf)
    }
  }

  let function doApply() {
    if (curRO)
      return
    let checkVal = getCompVal(eid, rawComponentName, path)
    let checkValText = compValToString(checkVal)
    if (checkValText == curText.value)
      return
    if (isValid.value) {
      local val = null
      try {
        val = convertTextToVal(checkVal, compType, curText.value)
      } catch(e) {
        val = null
      }
      if (val != null && setVal(val)) {
        anim_start($"{comp_name}{"".join(path??[])}")
        gui_scene.clearTimer(updateTextFromEcsTimeout)
        gui_scene.resetTimeout(0.1, updateTextFromEcs) //do this in case when some es changes components
        return
      }
    }
    anim_start($"!{comp_name}{"".join(path??[])}")
  }

  let function textInput() {
    return {
      rendObj = ROBJ_TEXT
      size = [flex(), SIZE_TO_CONTENT]
      margin = gridMargin

      color = !isValid.value
                ? colors.TextError
                : curRO
                  ? colors.TextReadOnly
                  : colors.TextDefault

      text = curText.value
      behavior = curRO ? null : Behaviors.TextInput
      group = group
      watch = [curText, isValid]
      onChange
      function onReturn() {
        isFocus(false)
        doApply()
        set_kb_focus(null)
      }

      function onEscape() {
        isFocus(false)
        updateTextFromEcs()
        set_kb_focus(null)
      }

      function onFocus() {
        isFocus(true)
        updateTextFromEcs()
      }

      function onBlur() {
        isFocus(false)
        doApply()
      }
    }
  }


  return {
    key = $"{eid}:{comp_name}{"".join(path??[])}"
    size = [flex(), SIZE_TO_CONTENT]
    rendObj = ROBJ_SOLID
    color = colors.ControlBg

    animations = [
      { prop=AnimProp.color, from=colors.HighlightSuccess, duration=0.5, trigger=$"{comp_name}{"".join(path??[])}" }
      { prop=AnimProp.color, from=colors.HighlightFailure, duration=0.5, trigger=$"!{comp_name}{"".join(path??[])}" }
    ]

    children = {
      size = [flex(), SIZE_TO_CONTENT]
      children = [
        textInput
        frame
      ]
    }
  }
}

local function fieldEditText(params={}){
  let {eid, comp_name, rawComponentName, path=null, onChange=null} = params
  let function setVal(val) {
    if (path != null) {
      setValToObj(eid, rawComponentName, path, val)
      entity_editor.save_component(eid, rawComponentName)
      onChange?()
      return true
    }
    else {
      local ok = false
      try {
        obsolete_dbg_set_comp_val(eid, comp_name, val)
        onChange?()
        ok = true
      }
      catch (e) {
        ok = false
      }
      if (ok)
        entity_editor.save_component(eid, rawComponentName)
      return ok
    }
  }

  params = params.__merge({
    compVal = getCompVal(eid, rawComponentName, path)
    setVal = setVal
  })
  return fieldEditText_(params)
}

return fieldEditText
