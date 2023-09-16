let {editorIsActive, editorFreeCam, entitiesListUpdateTrigger, showTemplateSelect,
     showPointAction, callPointActionCallback, resetPointActionMode,
     handleEntityCreated, handleEntityRemoved, handleEntityMoved,
     de4editMode, de4workMode} = require("state.nut")

let daEditor4 = require("daEditor4")
let entity_editor = require("entity_editor")

let eventbus = require("eventbus")

let {DE4_MODE_POINT_ACTION, isFreeCamMode=null} = daEditor4
let {DE4_MODE_CREATE_ENTITY, get_point_action_op} = entity_editor


eventbus.subscribe("daEditor4.onDe4SetWorkMode", function onDe4SetWorkMode(mode) {
  de4workMode(mode)
})

eventbus.subscribe("daEditor4.onDe4SetEditMode", function onDe4SetEditMode(mode) {
  de4editMode(mode)
  showTemplateSelect(mode == DE4_MODE_CREATE_ENTITY)

  showPointAction(mode == DE4_MODE_POINT_ACTION)
  if (!showPointAction.value)
    resetPointActionMode()
})

eventbus.subscribe("entity_editor.onEditorActivated", function onEditorActivated(on) {
  editorIsActive.update(on)
})

eventbus.subscribe("entity_editor.onEditorChanged", function onEditorChanged(_) {
  editorFreeCam.update(isFreeCamMode?() ?? false)

  local paOp = get_point_action_op()
  if (paOp != "") {
    let mod      = entity_editor.get_point_action_mod()
    let has_pos  = entity_editor.get_point_action_has_pos()
    let pos      = entity_editor.get_point_action_pos()
    let ext_id   = entity_editor.get_point_action_ext_id()
    let ext_name = entity_editor.get_point_action_ext_name()
    let ext_mtx  = entity_editor.get_point_action_ext_mtx()
    let ext_sph  = entity_editor.get_point_action_ext_sph()
    let ext_eid  = entity_editor.get_point_action_ext_eid()
    local ev = {
      op = paOp
      mod
      pos = has_pos ? pos : null
      ext_id
      ext_name
      ext_mtx
      ext_sph
      ext_eid
    }
    callPointActionCallback(ev)
  }
})

eventbus.subscribe("entity_editor.onEntityAdded", function onEntityAdded(_eid) {
  entitiesListUpdateTrigger(entitiesListUpdateTrigger.value+1)
})

eventbus.subscribe("entity_editor.onEntityRemoved", function onEntityRemoved(eid) {
  entitiesListUpdateTrigger(entitiesListUpdateTrigger.value+1)
  handleEntityRemoved(eid)
})

eventbus.subscribe("entity_editor.onEntityNewBySample", function onEntityNewBySample(eid) {
  handleEntityCreated(eid)
})

eventbus.subscribe("entity_editor.onEntityMoved", function onEntityMoved(eid) {
  handleEntityMoved(eid)
})
