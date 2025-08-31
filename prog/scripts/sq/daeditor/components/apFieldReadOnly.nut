from "%darg/ui_imports.nut" import *
from "%sqstd/ecs.nut" import *

let {getValFromObj, compValToString} = require("attrUtil.nut")

let getCompVal = @(eid, comp_name, path) path!=null ? getValFromObj(eid, comp_name, path) : _dbg_get_comp_val_inspect(eid, comp_name)

function fieldReadOnly(params = {}) {
  let {path, eid, rawComponentName=null} = params
  let val = getCompVal(eid, rawComponentName, path)

  if (rawComponentName == "transform") {
    let uniqueTimerKey = $"apFieldReadOnly, fieldReadOnly, {eid}, {rawComponentName}"
    let valText = Watched(compValToString(val))
    function updateTransformFromEcs() {
      let updVal = getCompVal(eid, rawComponentName, path)
      valText.set(compValToString(updVal))
    }
    gui_scene.clearTimer(uniqueTimerKey)
    gui_scene.setInterval(0.25, updateTransformFromEcs, uniqueTimerKey)
    return @() {
      watch = valText
      rendObj = ROBJ_TEXT
      size = FLEX_H
      text = valText.get()
      margin = fsh(0.5)
      onDetach = @() gui_scene.clearTimer(uniqueTimerKey)
    }
  }

  let valText = compValToString(val)
  // return value needs to be a function because we have a WARNING: w226 (return-different-types) Function can return different types.
  return @() {
    rendObj = ROBJ_TEXT
    size = FLEX_H
    text = valText
    margin = fsh(0.5)
  }
}


return fieldReadOnly
