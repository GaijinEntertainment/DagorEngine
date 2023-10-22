from "%darg/ui_imports.nut" import *

let {mkPanelElemsButton} = require("panelElem.nut")

let editorState = require_optional("%daeditor/state.nut")
let proceedWithSavingUnsavedChanges = editorState?.proceedWithSavingUnsavedChanges ?? @(...) false

let entity_editor = require_optional("entity_editor")

let {get_scene_filepath=null} = entity_editor
let {switch_scene=null} = require_optional("app")

return @(showMsgBox) mkPanelElemsButton("Restart", function() {
  if (get_scene_filepath && switch_scene && showMsgBox)
    proceedWithSavingUnsavedChanges(showMsgBox, @() switch_scene(get_scene_filepath()), true)
})
