from "daRg" import sh, sw, set_kb_focus
require("interop.nut")
require("daeditor_es.nut")

let { showHelp, editorIsActive, editorFreeCam, showTemplateSelect, propPanelVisible, showPointAction, typePointAction, entitiesListUpdateTrigger } = require("state.nut")

editorIsActive.subscribe(function(v){ if(v == false) set_kb_focus(null) })
editorFreeCam.subscribe(function(v){ if(v == true) set_kb_focus(null) })
propPanelVisible.subscribe(function(v){ if (v == false) set_kb_focus(null) })

let mainToolbar = require("mainToolbar.nut")
let templateSelect = require("templateSelect.nut")
let attrPanel = require("attrPanel.nut")
let help = require("components/help.nut")(showHelp)
let cursors = require("components/cursors.nut")
let { modalWindowsComponent } = require("components/modalWindows.nut")
let { msgboxComponent } = require("%daeditor/components/msgbox.nut")
let { windowsManager, registerWindow } = require("%daeditor/components/window.nut")

registerWindow(require("entitySelect.nut"))
registerWindow(require("loadedScenes.nut"))
registerWindow(require("logsWindow.nut"))

function getActionCursor(actionType) {
  if (actionType=="point_action")
    return cursors.actionCircle
  if (actionType=="pick_action")
    return cursors.actionPick
  return cursors.normal
}

return function() {
  if (!editorIsActive.get()) {
    return {
      watch = editorIsActive
    }
  }

  return {
    size = [sw(100), sh(100)]
    watch = [
      editorIsActive
      showTemplateSelect
      showPointAction
      typePointAction
      showHelp
      entitiesListUpdateTrigger
      editorFreeCam
    ]

    cursor = editorFreeCam.get() ? null
      : showPointAction.get() ? getActionCursor(typePointAction.get())
      : cursors.normal

    children = [
      mainToolbar,
      windowsManager,
      (showTemplateSelect.get() ? templateSelect : null), //fixme
      attrPanel, //fixme
      (showHelp.get() ? help : null),
      modalWindowsComponent,
      msgboxComponent,
    ]
  }
}
