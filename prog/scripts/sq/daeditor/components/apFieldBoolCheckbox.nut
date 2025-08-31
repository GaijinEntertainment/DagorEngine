from "%darg/ui_imports.nut" import *
from "%sqstd/ecs.nut" import *
from "style.nut" import colors

let entity_editor = require_optional("entity_editor")
let { setValToObj, getValFromObj, isCompReadOnly } = require("attrUtil.nut")

let getVal = @(eid, comp_name, path) path==null ? _dbg_get_comp_val_inspect(eid, comp_name) : getValFromObj(eid, comp_name, path)
function fieldBoolCheckbox(params = {}) {
  let {eid, comp_name, path, rawComponentName=null} = params
  local curRO = isCompReadOnly(eid, rawComponentName)
  local curVal = getVal(eid, rawComponentName, path)
  curVal = Watched(curVal)

  let group = ElemGroup()
  let stateFlags = Watched(0)
  let hoverFlag = Computed(@() stateFlags.get() & S_HOVER)

  function updateTextFromEcs() {
    let val = getVal(eid, rawComponentName, path)
    curVal.set(val)
  }
  function onClick() {
    if (curRO)
      return
    let val = !getVal(eid, rawComponentName, path)
    if (path!=null)
      setValToObj(eid, rawComponentName, path, val)
    else
      obsolete_dbg_set_comp_val(eid, comp_name, val)
    entity_editor?.save_component(eid, rawComponentName)
    params?.onChange?()

    curVal.set(val)
    let uniqueTimerKey = $"{eid}, {comp_name}, {path}, {rawComponentName}"
    gui_scene.resetTimeout(0.1, updateTextFromEcs, uniqueTimerKey) //do this in case when some es changes components
    return
  }


  return function () {
    local mark = null
    if (curVal.get()) {
      mark = {
        rendObj = ROBJ_SOLID
        color = curRO ? colors.ReadOnly : (hoverFlag.get() != 0) ? colors.Hover : colors.Interactive
        group
        size = static [pw(50), ph(50)]
        hplace = ALIGN_CENTER
        vplace = ALIGN_CENTER
      }
    }

    return {
      key = comp_name
      size = static [flex(), fontH(100)]
      halign = ALIGN_LEFT
      valign = ALIGN_CENTER

      watch = [curVal, hoverFlag]

      children = {
        size = static [fontH(80), fontH(80)]
        rendObj = ROBJ_SOLID
        color = colors.ControlBg

        behavior = Behaviors.Button
        group

        children = mark

        onElemState = @(sf) stateFlags.set(sf)

        onClick
      }
    }
  }
}

return fieldBoolCheckbox
