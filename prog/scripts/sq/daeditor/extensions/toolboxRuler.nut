from "%darg/ui_imports.nut" import *
import "%sqstd/ecs.nut" as ecs

let {addToolboxOption, getToolboxState, runToolboxCmd} = require("panelElem_buttonToolbox.nut")

let {showPointAction, namePointAction, setPointActionMode, resetPointActionMode} = require("%daeditor/state.nut")

let {CmdDebugRulerAddOrRemovePoint=@(...) null,
     CmdDebugRulerClearPoints=@(...) null} = require_optional("dasevents")

const POINTACTION_MODE_RULER_DIRECT = "Direct distance meter"
const POINTACTION_MODE_RULER_NAVMESH = "NavMesh distance meter"
const RULER_MODE_DIRECT = 0
const RULER_MODE_NAVMESH = 1

let isRulerDirectMode = Computed(@() showPointAction.value && namePointAction.value == POINTACTION_MODE_RULER_DIRECT)
let isRulerNavmeshMode = Computed(@() showPointAction.value && namePointAction.value == POINTACTION_MODE_RULER_NAVMESH)
let isRulerMode = Computed(@() isRulerDirectMode.value || isRulerNavmeshMode.value)


local rulerDisableCmd = function() {}
function callRulerDisableFn() {
  rulerDisableCmd()
  rulerDisableCmd = function() {}
}


function onRulerAction(action) {
  if (!isRulerMode.value)
    return

  if (action.op == "action" && action.pos != null) {
      ecs.g_entity_mgr.broadcastEvent(CmdDebugRulerAddOrRemovePoint({pos=action.pos, findClosestPoint=true}))
  }
  else if (action.op == "context") {
    ecs.g_entity_mgr.broadcastEvent(CmdDebugRulerClearPoints())
  }
  else if (action.op == "finish") {
    callRulerDisableFn()
  }
}


function toggleRuler(enabled, mode, toggle_cmd_fn) {
  if (!enabled) {
    callRulerDisableFn()
    return
  }

  let pointActionMode = mode == RULER_MODE_DIRECT ? POINTACTION_MODE_RULER_DIRECT : POINTACTION_MODE_RULER_NAVMESH
  setPointActionMode("point_action", pointActionMode, onRulerAction)
  toggle_cmd_fn(true)

  rulerDisableCmd = function() {
    resetPointActionMode()
    toggle_cmd_fn(false)
  }
}

function toolboxCmd_toggleRuler(mode) {
  let newEnabled = getToolboxState("ruler")?.mode == mode
                 ? !(getToolboxState("ruler")?.enabled ?? false)
                 : true
  let cmd = $"distance.meter {mode}"
  toggleRuler(newEnabled, mode, @(enabled) runToolboxCmd(cmd, null, "ruler", { enabled mode }))
}

function addToolboxOptions_Ruler() {
  addToolboxOption(@() isRulerDirectMode.value,  "ruler", { enabled = false, mode = 0 }, "Ruler",           @(_) toolboxCmd_toggleRuler(RULER_MODE_DIRECT),  null, "Toggle direct ruler")
  addToolboxOption(@() isRulerNavmeshMode.value, null, null,                             "Ruler Navmesh",   @(_) toolboxCmd_toggleRuler(RULER_MODE_NAVMESH), null, "Toggle navmesh ruler")
}

return {
  addToolboxOptions_Ruler
}
