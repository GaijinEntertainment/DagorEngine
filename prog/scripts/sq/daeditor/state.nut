import "console" as console
import "%sqstd/ecs.nut" as ecs
from "%darg/ui_imports.nut" import *
from "%sqstd/frp.nut" import WatchedRo
from "ecs.computed" import mkEcsComputedEidMap
let { hideAllWindows } = require("%daeditor/components/window.nut")
let { mkCompPathKey } = require("%daeditor/components/attrUtil.nut")

let {setWorkMode=@(_) null, setEditMode=@(_) null, setPointActionPreview=@(_, __) null,
     DE4_MODE_POINT_ACTION=null, DE4_MODE_SELECT=null,
     DE4_MODE_MOVE=null, DE4_MODE_ROTATE=null, DE4_MODE_SCALE=null, DE4_MODE_MOVE_SURF=null,
     DE4_BASIS_WORLD=null, DE4_BASIS_LOCAL=null, DE4_BASIS_PARENT=null,
     DE4_CENTER_PIVOT=null, DE4_CENTER_SELECTION=null,
     workMode = WatchedRo(""), editMode = WatchedRo(null), gizmoBasisType = WatchedRo(null), gizmoCenterType = WatchedRo(null),
     editorIsActive = WatchedRo(false), editorFreeCam = WatchedRo(false)} = require_optional("daEditorEmbedded")
let {get_scene_filepath=@() null, set_start_work_mode=@(_) null, get_instance=@() null,
     DE4_MODE_CREATE_ENTITY=null} = require_optional("entity_editor")
let selectedEntities = mkEcsComputedEidMap({ comps = ["eid"], comps_rq = ["daeditor__selected"] })
// The one entity the attrPanel shows out of a multi-selection. The editor drops its
// selection focus on any selection change, so this follows it.
let focusedEntity = Watched(ecs.INVALID_ENTITY_ID)
selectedEntities.subscribe_with_nasty_disregard_of_frp_update(@(_) focusedEntity.set(ecs.INVALID_ENTITY_ID))
// The editor zooms to the focused entity, so it learns the pick too
function focusEntity(eid) {
  focusedEntity.set(eid)
  get_instance()?.setFocusedEntity(eid)
}
let selectedEntity = Computed(function() {
  let sel = selectedEntities.get()
  if (sel.len() == 1)
    return sel.keys()[0]
  let focused = focusedEntity.get()
  return focused in sel ? focused : ecs.INVALID_ENTITY_ID
})
// The picked attrPanel row: null, or { cname, path } with path null for the component itself
let selectedComp = Watched(null)
let deselectComp = @() selectedComp.set(null)
let selectedCompKey = Computed(function() {
  let sel = selectedComp.get()
  return sel == null ? null : mkCompPathKey(sel.cname, sel.path)
})
selectedEntity.subscribe_with_nasty_disregard_of_frp_update(@(_) deselectComp())
const SETTING_EDITOR_WORKMODE = "daEditor/workMode"
const SETTING_EDITOR_TPLGROUP = "daEditor/templatesGroup"
const SETTING_EDITOR_PROPS_ON_SELECT = "daEditor/showPropsOnSelect"
let { save_settings=null, get_setting_by_blk_path=null, set_setting_by_blk_path=null } = require_optional("settings")

let selectedTemplatesGroup = mkWatched(persist, "selectedTemplatesGroup", (get_setting_by_blk_path?(SETTING_EDITOR_TPLGROUP) ?? ""))
selectedTemplatesGroup.subscribe(function(v) { set_setting_by_blk_path?(SETTING_EDITOR_TPLGROUP, v ?? ""); save_settings?() })

let propPanelVisible = mkWatched(persist, "propPanelVisible", false)
let propPanelClosed  = mkWatched(persist, "propPanelClosed", (get_setting_by_blk_path?(SETTING_EDITOR_PROPS_ON_SELECT) ?? true)==false)
propPanelClosed.subscribe(function(v) { set_setting_by_blk_path?(SETTING_EDITOR_PROPS_ON_SELECT, (v ?? false)==false); save_settings?() })

let de4workMode = workMode
let de4workModes = Watched([""])
function selectWorkMode(mode) {
  mode = mode ?? ""
  if (mode == de4workMode.get())
    return
  set_start_work_mode?(mode)
  setWorkMode(mode)
  set_setting_by_blk_path?(SETTING_EDITOR_WORKMODE, mode)
  save_settings?()
}

function initWorkModes(modes, defMode=null) {
  modes = modes ?? [""]
  de4workModes.set(modes)
  let good_mode = modes.contains(defMode) ? defMode : modes?[0] ?? ""
  let last_mode = get_setting_by_blk_path?(SETTING_EDITOR_WORKMODE) ?? good_mode
  let mode_to_set = modes.contains(last_mode) ? last_mode : good_mode
  selectWorkMode(mode_to_set)
}

let de4editMode = editMode
let showTemplateSelect = Computed(@() DE4_MODE_CREATE_ENTITY != null && de4editMode.get() == DE4_MODE_CREATE_ENTITY)
let showPointAction = Computed(@() DE4_MODE_POINT_ACTION != null && de4editMode.get() == DE4_MODE_POINT_ACTION)

let gizmoEditModes = [DE4_MODE_MOVE, DE4_MODE_MOVE_SURF, DE4_MODE_ROTATE, DE4_MODE_SCALE]

let gizmoBasisTypeNames = [[DE4_BASIS_WORLD, "World"], [DE4_BASIS_LOCAL, "Local"], [DE4_BASIS_PARENT, "Parent"]]
let gizmoBasisTypeEditingDisabled = Computed(@() !gizmoEditModes.contains(de4editMode.get()))
let gizmoCenterTypeNames = [[DE4_CENTER_PIVOT, "Pivot"], [DE4_CENTER_SELECTION, "Selection"]]

function proceedWithSavingUnsavedChanges(showMsgbox, callback, unsavedText=null, proceedText=null) {
  if (unsavedText == true) { unsavedText = null; proceedText = true; }
  local hasUnsavedChanges = (get_instance() != null && (get_instance().hasUnsavedChanges() ?? false))
  if (!hasUnsavedChanges && proceedText==null) { callback(); return }
  if (proceedText == true) proceedText = null;
  showMsgbox({
    text = hasUnsavedChanges ? (unsavedText!=null ? unsavedText : "You have unsaved changes. How do you want to proceed?")
                             : (proceedText!=null ? proceedText : "No unsaved changes. Proceed?")
    buttons = hasUnsavedChanges ? [
      { text = "Save changes",  isCurrent = true, action = function() { get_instance()?.saveDirtyScenes(); callback() }}
      { text = "Ignore changes" action = callback }
      { text = "Cancel", isCancel = true }
    ] : [
      { text = "Proceed" action = callback }
      { text = "Cancel", isCancel = true }
    ]
  })
}

let editorTimeStop = mkWatched(persist, "editorTimeStop", false)
editorTimeStop.subscribe(function(v) {
  if (v == true)
    console?.command($"app.timeSpeed 0")
  else if (v == false)
    console?.command($"app.timeSpeed 1")
})

let editorUnpauseData = {timerRunning = false}
function editorUnpauseEnd() {
  editorUnpauseData.timerRunning = false
  editorTimeStop.set(true)
}
function editorUnpause(time) {
  if (time <= 0) {
    editorTimeStop.set(false)
    gui_scene.clearTimer(editorUnpauseEnd)
    editorUnpauseData.timerRunning = false
    return
  }
  if (editorTimeStop.get() || editorUnpauseData.timerRunning) {
    editorTimeStop.set(false)
    editorUnpauseData.timerRunning = true
    gui_scene.resetTimeout(time, editorUnpauseEnd)
  }
}

let typePointAction = mkWatched(persist, "typePointAction", "")
let namePointAction = mkWatched(persist, "namePointAction", "")
let edObjectFlagsUpdateTrigger = mkWatched(persist, "edObjectFlagsUpdateTrigger", 0)
// Names the trigger as a Computed source where the editor state behind it is read by native calls.
let DEPENDS_ON = @(...) null

local funcPointAction = null
function setPointActionMode(actionType, actionName, cb) {
  hideAllWindows()
  setEditMode(DE4_MODE_POINT_ACTION)
  setPointActionPreview("", 0.0) // default
  typePointAction.set(actionType)
  namePointAction.set(actionName)
  funcPointAction = cb
}
function updatePointActionPreview(shape, param) {
  setPointActionPreview(shape, param)
}
function callPointActionCallback(action) {
  funcPointAction?(action)
}
function resetPointActionMode() {
  local funcFinish = funcPointAction
  if (de4editMode.get() == DE4_MODE_POINT_ACTION)
    setEditMode(DE4_MODE_SELECT)
  setPointActionPreview("", 0.0)
  typePointAction.set("")
  namePointAction.set("")
  funcPointAction = null
  if (funcFinish != null)
    funcFinish({ op = "finish" })
}
showPointAction.subscribe_with_nasty_disregard_of_frp_update(function(on) {
  if (!on)
    resetPointActionMode()
})

let funcsEntityCreated = []
function addEntityCreatedCallback(cb) {
  funcsEntityCreated.append(cb)
}
function handleEntityCreated(eid) {
  foreach (func in funcsEntityCreated) {
    func?(eid)
  }
}

let funcsEntityRemoved = []
function addEntityRemovedCallback(cb) {
  funcsEntityRemoved.append(cb)
}
function handleEntityRemoved(eid) {
  foreach (func in funcsEntityRemoved) {
    func?(eid)
  }
}

let funcsEntityMoved = []
function addEntityMovedCallback(cb) {
  funcsEntityMoved.append(cb)
}
function handleEntityMoved(eid) {
  foreach (func in funcsEntityMoved) {
    func?(eid)
  }
}

return {
  EntitySelectWndId = "entity_select"
  SceneOutlinerWndId = "scene_outliner"
  LogsWindowId = "log_window"

  showUIinEditor = mkWatched(persist, "showUIinEditor", false)
  editorIsActive
  editorFreeCam
  selectedEntity
  selectedEntities
  focusEntity
  selectedTemplatesGroup
  scenePath = Watched(get_scene_filepath?())
  propPanelVisible
  propPanelClosed
  filterString = mkWatched(persist, "filterString", "")
  selectedComp
  selectedCompKey
  deselectComp
  showTemplateSelect
  showHelp = mkWatched(persist, "showHelp", false)
  edObjectFlagsUpdateTrigger
  DEPENDS_ON
  de4editMode
  extraPropPanelCtors = Watched([])
  de4workMode
  de4workModes
  initWorkModes
  selectWorkMode
  gizmoBasisType
  gizmoBasisTypeNames
  gizmoBasisTypeEditingDisabled
  gizmoCenterType
  gizmoCenterTypeNames
  proceedWithSavingUnsavedChanges
  showDebugButtons = Watched(true)

  editorTimeStop
  editorUnpause

  showPointAction
  typePointAction
  namePointAction
  setPointActionMode
  updatePointActionPreview
  callPointActionCallback
  resetPointActionMode

  addEntityCreatedCallback
  addEntityRemovedCallback
  addEntityMovedCallback
  handleEntityCreated
  handleEntityRemoved
  handleEntityMoved

  wantOpenRISelect = Watched(false)
}
