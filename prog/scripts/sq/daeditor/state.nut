from "%darg/ui_imports.nut" import *
import "%sqstd/ecs.nut" as ecs

let console = require("console")
let { mkFrameIncrementObservable } = require("%daeditor/ec_to_watched.nut")

let {getEditMode=@() null, isFreeCamMode=@() false, setWorkMode=@(_) null,
     setEditMode=@(_) null, setPointActionPreview=@(_, __) null, DE4_MODE_POINT_ACTION=null, DE4_MODE_SELECT=null} = require_optional("daEditorEmbedded")
let {is_editor_activated=@() false, get_scene_filepath=@() null, set_start_work_mode=@(_) null, get_instance=@() null} = require_optional("entity_editor")
let selectedEntity = Watched(ecs.INVALID_ENTITY_ID)
let { selectedEntities, selectedEntitiesSetKeyVal, selectedEntitiesDeleteKey } = mkFrameIncrementObservable({}, "selectedEntities")
let showEntitySelect = mkWatched(persist, "showEntitySelect", false)

const SETTING_EDITOR_WORKMODE = "daEditor/workMode"
const SETTING_EDITOR_TPLGROUP = "daEditor/templatesGroup"
const SETTING_EDITOR_PROPS_ON_SELECT = "daEditor/showPropsOnSelect"
let { save_settings=null, get_setting_by_blk_path=null, set_setting_by_blk_path=null } = require_optional("settings")

let selectedTemplatesGroup = mkWatched(persist, "selectedTemplatesGroup", (get_setting_by_blk_path?(SETTING_EDITOR_TPLGROUP) ?? ""))
selectedTemplatesGroup.subscribe(function(v) { set_setting_by_blk_path?(SETTING_EDITOR_TPLGROUP, v ?? ""); save_settings?() })

let propPanelVisible = mkWatched(persist, "propPanelVisible", false)
let propPanelClosed  = mkWatched(persist, "propPanelClosed", (get_setting_by_blk_path?(SETTING_EDITOR_PROPS_ON_SELECT) ?? true)==false)
propPanelClosed.subscribe(function(v) { set_setting_by_blk_path?(SETTING_EDITOR_PROPS_ON_SELECT, (v ?? false)==false); save_settings?() })

let de4workMode = Watched("")
let de4workModes = Watched([""])
de4workMode.subscribe(function(v) {
  set_start_work_mode?(v ?? "")
  setWorkMode?(v ?? "")
  set_setting_by_blk_path?(SETTING_EDITOR_WORKMODE, v ?? "")
  save_settings?()
})

let initWorkModes = function(modes, defMode=null) {
  de4workModes(modes ?? [""])
  let good_mode = modes.contains(defMode) ? defMode : modes?[0] ?? ""
  let last_mode = get_setting_by_blk_path?(SETTING_EDITOR_WORKMODE) ?? good_mode
  let mode_to_set = modes.contains(last_mode) ? last_mode : good_mode
  de4workMode(mode_to_set)
}

let proceedWithSavingUnsavedChanges = function(showMsgbox, callback, unsavedText=null, proceedText=null) {
  if (unsavedText == true) { unsavedText = null; proceedText = true; }
  local hasUnsavedChanges = (get_instance() != null && (get_instance().hasUnsavedChanges() ?? false))
  if (!hasUnsavedChanges && proceedText==null) { callback(); return }
  if (proceedText == true) proceedText = null;
  showMsgbox({
    text = hasUnsavedChanges ? (unsavedText!=null ? unsavedText : "You have unsaved changes. How do you want to proceed?")
                             : (proceedText!=null ? proceedText : "No unsaved changes. Proceed?")
    buttons = hasUnsavedChanges ? [
      { text = "Save changes",  isCurrent = true, action = function() { get_instance?().saveObjects(""); callback() }}
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
  editorTimeStop(true)
}
function editorUnpause(time) {
  if (time <= 0) {
    editorTimeStop(false)
    gui_scene.clearTimer(editorUnpauseEnd)
    editorUnpauseData.timerRunning = false
    return
  }
  if (editorTimeStop.value || editorUnpauseData.timerRunning) {
    editorTimeStop(false)
    editorUnpauseData.timerRunning = true
    gui_scene.resetTimeout(time, editorUnpauseEnd)
  }
}

let showPointAction = mkWatched(persist, "showPointAction", false)
let typePointAction = mkWatched(persist, "typePointAction", "")
let namePointAction = mkWatched(persist, "namePointAction", "")
local funcPointAction = null
function setPointActionMode(actionType, actionName, cb) {
  showEntitySelect(false)
  setEditMode(DE4_MODE_POINT_ACTION)
  setPointActionPreview("", 0.0) // default
  typePointAction(actionType)
  namePointAction(actionName)
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
  if (getEditMode() == DE4_MODE_POINT_ACTION)
    setEditMode(DE4_MODE_SELECT)
  setPointActionPreview("", 0.0)
  typePointAction("")
  namePointAction("")
  funcPointAction = null
  if (funcFinish != null)
    funcFinish({ op = "finish" })
}

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
  showUIinEditor = mkWatched(persist, "showUIinEditor", false)
  editorIsActive = Watched(is_editor_activated?())
  editorFreeCam = Watched(isFreeCamMode?())
  selectedEntity
  selectedEntities
  selectedEntitiesSetKeyVal
  selectedEntitiesDeleteKey
  selectedTemplatesGroup
  scenePath = Watched(get_scene_filepath?())
  propPanelVisible
  propPanelClosed
  filterString = mkWatched(persist, "filterString", "")
  selectedCompName = Watched()
  showEntitySelect
  showTemplateSelect = mkWatched(persist, "showTemplateSelect", false)
  showHelp = mkWatched(persist, "showHelp", false)
  entitiesListUpdateTrigger = mkWatched(persist, "entitiesListUpdateTrigger", 0)
  de4editMode = Watched(getEditMode?())
  extraPropPanelCtors = Watched([])
  de4workMode
  de4workModes
  initWorkModes
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

