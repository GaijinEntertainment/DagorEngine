from "%darg/ui_imports.nut" import *
from "%sqstd/ecs.nut" import *
from "style.nut" import colors, gridHeight, gridMargin

let entity_editor = require_optional("entity_editor")
let { compValToString, isValueTextValid, convertTextToVal, setValToObj, getValFromObj, isCompReadOnly } = require("attrUtil.nut")

let getCompVal = @(eid, comp_name, path) path!=null ? getValFromObj(eid, comp_name, path) : _dbg_get_comp_val_inspect(eid, comp_name)

function fieldEditText_(params={}) {
  let {eid, comp_name, compVal, setVal, path, rawComponentName=null} = params
  local curRO = isCompReadOnly(eid, rawComponentName)

  let curText = Watched(compValToString(compVal))
  let isFocus = Watched(false)
  let group = ElemGroup()
  let compType = typeof compVal
  let stateFlags = Watched(0)
  let isValid = Computed(@() isValueTextValid(compType, curText.get()))

  function onChange(text){
    curText.set(text)
  }

  function updateTextFromEcs() {
    let val = getCompVal(eid, rawComponentName, path)
    let compTextVal = compValToString(val)
    curText.set(compTextVal)
  }

  let uniqueTimerKey = $"{eid}, {comp_name}, {path}, {rawComponentName}"
  function updateTextFromEcsTimeout() {
    updateTextFromEcs()
    if (!isFocus.get())
      gui_scene.resetTimeout(0.1, updateTextFromEcsTimeout, uniqueTimerKey)
  }

  function updateComponentByTimer() {
    if (rawComponentName == "transform") {
      if (isFocus.get())
        gui_scene.clearTimer(uniqueTimerKey)
      else
        gui_scene.resetTimeout(0.1, updateTextFromEcsTimeout, uniqueTimerKey)
    }
  }

  isFocus.subscribe(@(_v) updateComponentByTimer())
  updateComponentByTimer()

  function frame() {
    let frameColor = (stateFlags.get() & S_KB_FOCUS) ? colors.FrameActive : colors.FrameDefault
    return {
      rendObj = ROBJ_FRAME group=group size = [flex(), gridHeight] color = frameColor watch = stateFlags
      onElemState = @(sf) stateFlags.set(sf)
    }
  }

  function doApply() {
    if (curRO)
      return
    let checkVal = getCompVal(eid, rawComponentName, path)
    let checkValText = compValToString(checkVal)
    if (checkValText == curText.get())
      return
    if (isValid.get()) {
      local val = null
      try {
        val = convertTextToVal(checkVal, compType, curText.get())
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

  function textInput() {
    return {
      rendObj = ROBJ_TEXT
      size = FLEX_H
      margin = gridMargin

      color = !isValid.get()
                ? colors.TextError
                : curRO
                  ? colors.TextReadOnly
                  : colors.TextDefault

      text = curText.get()
      behavior = curRO ? null : Behaviors.TextInput
      group = group
      watch = [curText, isValid]
      onChange
      function onReturn() {
        isFocus.set(false)
        doApply()
        set_kb_focus(null)
      }

      function onEscape() {
        isFocus.set(false)
        updateTextFromEcs()
        set_kb_focus(null)
      }

      function onFocus() {
        isFocus.set(true)
        updateTextFromEcs()
      }

      function onBlur() {
        isFocus.set(false)
        doApply()
      }
    }
  }


  return {
    key = $"{eid}:{comp_name}{"".join(path??[])}"
    size = FLEX_H
    rendObj = ROBJ_SOLID
    color = colors.ControlBg

    animations = [
      { prop=AnimProp.color, from=colors.HighlightSuccess, duration=0.5, trigger=$"{comp_name}{"".join(path??[])}" }
      { prop=AnimProp.color, from=colors.HighlightFailure, duration=0.5, trigger=$"!{comp_name}{"".join(path??[])}" }
    ]

    children = {
      size = FLEX_H
      children = [
        textInput
        frame
      ]
    }
  }
}

function fieldEditText(params={}){
  let {eid, comp_name, rawComponentName, path=null, onChange=null} = params
  function setVal(val) {
    if (path != null) {
      setValToObj(eid, rawComponentName, path, val)
      entity_editor?.save_component(eid, rawComponentName)
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
        entity_editor?.save_component(eid, rawComponentName)
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
