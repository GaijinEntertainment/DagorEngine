let {sh, sw, set_kb_focus} = require("daRg")
require("interop.nut")
require("daeditor_es.nut")

let {showHelp, editorIsActive, editorFreeCam, showEntitySelect, showTemplateSelect, propPanelVisible,
     showPointAction, typePointAction, entitiesListUpdateTrigger} = require("state.nut")

editorIsActive.subscribe(function(v){ if(v == false) set_kb_focus(null) })
editorFreeCam.subscribe(function(v){ if(v == true) set_kb_focus(null) })
propPanelVisible.subscribe(function(v){ if (v == false) set_kb_focus(null) })

let mainToolbar = require("mainToolbar.nut")
let entitySelect = require("entitySelect.nut")
let {showLogsWindow} = require("%daeditor/state/logsWindow.nut")
let logsWindow = require("logsWindow.nut")
let templateSelect = require("templateSelect.nut")
let attrPanel = require("attrPanel.nut")
let help = require("components/help.nut")(showHelp)
let cursors = require("components/cursors.nut")
let {modalWindowsComponent} = require("components/modalWindows.nut")
let {msgboxComponent} = require("editor_msgbox.nut")

let function getActionCursor(actionType) {
  if (actionType=="point_action")
    return cursors.actionCircle
  if (actionType=="pick_action")
    return cursors.actionPick
  return cursors.normal
}

return function() {
  if (!editorIsActive.value) {
    return {
      watch = editorIsActive
    }
  }

  return {
    size = [sw(100), sh(100)]
    watch = [
      editorIsActive
      showEntitySelect
      showTemplateSelect
      showPointAction
      typePointAction
      showHelp
      entitiesListUpdateTrigger
      showLogsWindow
      editorFreeCam
    ]

    cursor = editorFreeCam.value ? null
      : showPointAction.value ? getActionCursor(typePointAction.value)
      : cursors.normal

    children = [
      mainToolbar,
      (showEntitySelect.value ? entitySelect : null),
      (showTemplateSelect.value ? templateSelect : null),
      attrPanel,
      (showHelp.value ? help : null),
      modalWindowsComponent,
      msgboxComponent,
      (showLogsWindow.value ? logsWindow : null),
    ]
  }
}
