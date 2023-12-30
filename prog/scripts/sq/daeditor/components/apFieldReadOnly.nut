from "%darg/ui_imports.nut" import *
from "%sqstd/ecs.nut" import *

let {getValFromObj, compValToString} = require("attrUtil.nut")

let getCompVal = @(eid, comp_name, path) path!=null ? getValFromObj(eid, comp_name, path) : _dbg_get_comp_val_inspect(eid, comp_name)

function fieldReadOnly(params = {}) {
  let {path, eid, rawComponentName=null} = params
  let val = getCompVal(eid, rawComponentName, path)

  if (rawComponentName == "transform") {
    let valText = Watched(compValToString(val))
    function updateTransformFromEcs() {
      let updVal = getCompVal(eid, rawComponentName, path)
      valText(compValToString(updVal))
      gui_scene.setTimeout(0.25, updateTransformFromEcs)
    }
    gui_scene.setTimeout(0.25, updateTransformFromEcs)
    return @() {
      watch = valText
      rendObj = ROBJ_TEXT
      size = [flex(), SIZE_TO_CONTENT]
      text = valText.value
      margin = fsh(0.5)
    }
  }

  let valText = compValToString(val)
  return @() {
    rendObj = ROBJ_TEXT
    size = [flex(), SIZE_TO_CONTENT]
    text = valText
    margin = fsh(0.5)
  }
}


return fieldReadOnly
