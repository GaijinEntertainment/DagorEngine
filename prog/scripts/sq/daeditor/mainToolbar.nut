import "daEditorEmbedded" as daEditor
from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let { DE4_MODE_CREATE_ENTITY = null, get_instance = @() null } = require_optional("entity_editor")
let { LogsWindowId, EntitySelectWndId, LoadedScenesWndId, propPanelVisible, propPanelClosed,
  showHelp, markedScenes, de4editMode, de4workMode, de4workModes, showUIinEditor, editorTimeStop,
  gizmoBasisType, gizmoBasisTypeNames, gizmoBasisTypeEditingDisabled, gizmoCenterType,
  gizmoCenterTypeNames } = require("state.nut")

let { getSceneIdOf, sceneGenerated, sceneSaved } = require("%daeditor/daeditor_es.nut")
let { defaultScenesSortMode } = require("components/mkSortSceneModeButton.nut")

let pictureButton = require("components/pictureButton.nut")
let { showMsgbox } = require("%daeditor/components/msgbox.nut")
let combobox = require("%daeditor/components/combobox.nut")
let cursors = require("%daeditor/components/cursors.nut")
let { hideWindow, toggleWindow, mkIsWindowVisible } = require("%daeditor/components/window.nut")
let { hasNewLogerr } = require("%daeditor/state/logsWindow.nut")

let {DE4_MODE_MOVE, DE4_MODE_ROTATE, DE4_MODE_SCALE, DE4_MODE_MOVE_SURF, DE4_MODE_SELECT,
     DE4_MODE_POINT_ACTION, getEditMode, setEditMode} = daEditor

let invert = @(v) !v
function toolbarButton(image, action, tooltip_text, checked=null, styles = {}) {
  function onHover(on) {
    cursors.setTooltip(on ? tooltip_text : null)
  }
  let defParams = {
    imageMargin = fsh(0.5)
    action
    onHover
    styles
  }
  let params = (type(image)=="table") ? defParams.__merge(image) : defParams.__merge({image})
  if (checked instanceof Watched) {
    return @(){ children = pictureButton(params.__update({checked=checked.get()})), watch = checked}
  }
  else return pictureButton(params.__update({checked}))
}

function modeButton(image, mode, tooltip_text, next_mode=null, next_action=null) {
  local params = (type(image)=="table") ? image : {image}
  params = params.__merge({
    checked = mode==getEditMode()
    imageMargin = fsh(0.5)
    function onHover(on) {
      cursors.setTooltip(on ? tooltip_text : null)
    }
    function action() {
      hideWindow(EntitySelectWndId)
      hideWindow(LoadedScenesWndId)
      if (next_mode && mode==getEditMode())
        mode = next_mode
      daEditor.setEditMode(mode)
      if (next_action)
        next_action()
    }
  })
  return pictureButton(params)
}


let separator = static {
  rendObj = ROBJ_SOLID
  color = Color(100, 100, 100, 100)
  size = [1, fsh(3)]
  margin = [0, fsh(0.5)]
}

let svg = @(name) {image = $"!%daeditor/images/{name}.svg"} //Atlas is not working %daeditor/editor#

function toggleEntitySelect() {
  if (getEditMode() == DE4_MODE_CREATE_ENTITY || getEditMode() == DE4_MODE_POINT_ACTION)
    setEditMode(DE4_MODE_SELECT)
  toggleWindow(EntitySelectWndId)
}
function toggleLoadedScenes() {
  if (getEditMode() == DE4_MODE_CREATE_ENTITY || getEditMode() == DE4_MODE_POINT_ACTION)
    setEditMode(DE4_MODE_SELECT)
  toggleWindow(LoadedScenesWndId)
}
function toggleLogsWindows() {
  toggleWindow(LogsWindowId)
}
function toggleCreateEntityMode() {
  hideWindow(EntitySelectWndId)
  hideWindow(LoadedScenesWndId)
  local mode = DE4_MODE_CREATE_ENTITY
  if (DE4_MODE_CREATE_ENTITY==getEditMode())
    mode = DE4_MODE_SELECT
  daEditor.setEditMode(mode)
}
let togglePropPanel = function() {
  propPanelClosed.set(propPanelVisible.get() && !showHelp.get())
  propPanelVisible.modify(invert)
}
let isVisibleEntitiesSelect = mkIsWindowVisible(EntitySelectWndId)
let isVisibleLoadedScenesWnd = mkIsWindowVisible(LoadedScenesWndId)
let isVisibleLogsWnd = mkIsWindowVisible(LogsWindowId)

let mkMessageboxSaveScenes = function(editableScenesCount) {
  local messageText = ""
  if (editableScenesCount > 1) {
    messageText = $"There are multiple ({editableScenesCount}) edited IMPORT scenes!"
  } else {
    messageText = $"There's an edited IMPORT scene!"
  }
  return {
    uid = "messagebox_save_scenes"
    text = messageText
    onClose = function() {}
    buttons = [
      {
        text = "Save MAIN and all edited IMPORT scenes"
        action = function() { get_instance()?.saveObjects("", true) }
        isCurrent = true
        isCancel = false
      }
      {
        text = "Save only the MAIN scene"
        action = function() { get_instance()?.saveObjects("", false) }
        isCurrent = false
        isCancel = false
      }
      {
        text = "Cancel"
        action = function() {}
        isCurrent = false
        isCancel = true
      }
    ]
  }
}

let markedSceneText = Computed(function() {
  local nMrk = 0
  local path = ""
  local scenes = get_instance()?.getSceneImports() ?? []
  scenes.sort(defaultScenesSortMode.func)
  foreach (scene in scenes) {
    local sceneId = getSceneIdOf(scene)
    local isMarked = markedScenes.get()?[sceneId] ?? false
    if (isMarked) {
      if (nMrk == 0) {
        path = scene.path
      }
      nMrk++
    }
  }
  local isMarkedSaved = markedScenes.get()?[getSceneIdOf(sceneSaved)] ?? false
  local isMarkedGenerated = markedScenes.get()?[getSceneIdOf(sceneGenerated)] ?? false
  local textSav = isMarkedSaved ? $"{sceneSaved.asText} " : ""
  local textGen = isMarkedGenerated ? $"{sceneGenerated.asText} " : ""
  if (nMrk > 0 || isMarkedSaved || isMarkedGenerated) {
    local andMore = nMrk > 1 ? $", and {nMrk - 1} more" : ""
    return $"Editing: {textGen}{textSav}{path}{andMore}"
  }
  return ""
})

function mainToolbar() {
  let toggleTime = @() editorTimeStop.set(!editorTimeStop.get())
  let toggleHelp = @() showHelp.modify(@(v) !v)
  let save = function() {
    local editableScenesCount = get_instance()?.getEditableScenesCount() ?? 0
    local hasUnsavedChildScenes = get_instance()?.hasUnsavedChildScenes() ?? false
    if (editableScenesCount == 0 || !hasUnsavedChildScenes) {
      get_instance()?.saveObjects("", false)
    } else {
      showMsgbox(mkMessageboxSaveScenes(editableScenesCount))
    }
  }

  let markedSceneStyle = {
    fontSize = hdpx(15)
    color = Color(255,255,0,160)
  }
  let markedSceneShadowStyle = {
    fontFx = FFT_BLUR // FFT_GLOW
    fontFxColor = Color(0,0,0,160)
    fontFxFactor = hdpx(15)
    fontFxOffsY = hdpx(0.7)
    fontFxOffsX = hdpx(0.3)
  }

  return {
    flow = FLOW_VERTICAL
    watch = [de4editMode, de4workMode, de4workModes, markedScenes]
    size = flex()
    children = [
      {
        cursor = cursors.normal
        size = [sw(100), SIZE_TO_CONTENT]
        flow = FLOW_HORIZONTAL
        rendObj = ROBJ_WORLD_BLUR
        fillColor = Color(20, 20, 20, 155)
        valign = ALIGN_CENTER
        padding =hdpx(4)

        children = [
          // comp(Watch(scenePath), txt(scenePath.get(), {color = Color(170,170,170), padding=[0, hdpx(10)], maxWidth = sw(15)}), ClipChildren)
          toolbarButton(svg("select_by_name"), toggleEntitySelect, "Find entity (Tab)", isVisibleEntitiesSelect)
          separator
          toolbarButton(svg("scenes"), toggleLoadedScenes, "Loaded scenes (I)", isVisibleLoadedScenesWnd)
          separator
          modeButton(svg("select"), DE4_MODE_SELECT, "Select (Q)")
          modeButton(svg("move"),   DE4_MODE_MOVE,   "Move (W)")
          modeButton(svg("rotate"), DE4_MODE_ROTATE, "Rotate (E)") //rotate or mirror, make blue
          modeButton(svg("scale"),  DE4_MODE_SCALE,  "Scale (R)")
          modeButton(svg("create"), DE4_MODE_CREATE_ENTITY, "Create entity (T)", DE4_MODE_SELECT)
          separator
          toolbarButton(svg("delete"),     @() get_instance()?.deleteSelectedObjects(true), "Delete selected (Del)")
          separator
          modeButton(svg("surf"), DE4_MODE_MOVE_SURF, "Surf over ground (Ctrl+Alt+W)")
          toolbarButton(svg("drop"),         @() get_instance()?.dropObjects(),                     "Drop (Ctrl+Alt+D)")
          toolbarButton(svg("dropnormal"),   @() get_instance()?.dropObjectsNorm(),                 "Drop on normal (Ctrl+Alt+E)")
          toolbarButton(svg("resetscale"),   @() get_instance()?.resetScale(),                      "Reset scale (Ctrl+Alt+R)")
          toolbarButton(svg("resetrotate"),  @() get_instance()?.resetRotate(),                     "Reset rotation (Ctrl+Alt+T)")
          toolbarButton(svg("zoomcenter"),   @() get_instance()?.zoomAndCenter(),                   "Zoom and center (Z)")
          toolbarButton(svg("parent_set"),   @() get_instance()?.setParentForSelection(),           "Set parent (Ctrl+P)\n\nMake the last selected entity parent to the rest of the selection.")
          toolbarButton(svg("parent_clear"), @() get_instance()?.clearParentForSelection(),         "Clear parent (Alt+P)")
          toolbarButton(svg("parent_free"),  @() get_instance()?.toggleFreeTransformForSelection(), "Toggle between free and fixed transform in a child entity")
          separator
          toolbarButton(svg("properties"), togglePropPanel, "Property panel (P)", propPanelVisible)
          toolbarButton(svg("hide"), @() get_instance()?.hideSelectedTemplate(), "Hide")
          toolbarButton(svg("show"), @() get_instance()?.unhideAll(), "Unhide all")
          separator
          toolbarButton(svg("gui_toggle"), @() showUIinEditor.set(!showUIinEditor.get()), "Show UI", showUIinEditor.get())
          @() {watch = editorTimeStop children = toolbarButton(svg("time_toggle"), toggleTime, "Toggle time (Ctrl+T)", !editorTimeStop.get())}
          separator
          toolbarButton(svg("save"), save, "Save")
          @() { watch = hasNewLogerr children = toolbarButton(svg("logs"), toggleLogsWindows, "Show errors (Ctrl+L)", isVisibleLogsWnd, hasNewLogerr.get() ? { defColor=Color(251,78,58,250) } : {})}
          toolbarButton(svg("help"), toggleHelp, "Help (F1)", showHelp.get())

          de4workModes.get().len() <= 1 ? null : separator
          de4workModes.get().len() <= 1 ? null : {
            size = [hdpx(100),fontH(100)]
            children = combobox(de4workMode, de4workModes)
          }

          separator
          @() {
            watch = gizmoBasisTypeEditingDisabled
            size = static [hdpx(150), fontH(100)]
            children = combobox({value = gizmoBasisType, disable = gizmoBasisTypeEditingDisabled}, gizmoBasisTypeNames, gizmoBasisTypeEditingDisabled.get() ? "Set gizmo basis mode (X)\n\nEnabled when the move/rotate/scale/surf over ground edit mode is active." : "Set gizmo basis mode (X)")
          }
          @() {
            watch = gizmoBasisTypeEditingDisabled
            size = static [hdpx(150), fontH(100)]
            children = combobox({value = gizmoCenterType, disable = gizmoBasisTypeEditingDisabled}, gizmoCenterTypeNames, gizmoBasisTypeEditingDisabled.get() ? "Set gizmo transformation center mode (C)\n\nEnabled when the move/rotate/scale/surf over ground edit mode is active." : "Set gizmo transformation center mode (C)")
          }
        ]

        hotkeys = [
          ["L.Ctrl !L.Alt I", toggleLoadedScenes],
          ["L.Alt H", toggleEntitySelect],
          ["Tab", toggleEntitySelect],
          ["!L.Ctrl !L.Alt T", toggleCreateEntityMode],
          ["F1", toggleHelp],
          ["!L.Ctrl !L.Alt P", togglePropPanel],
          ["L.Ctrl !L.Alt P", @() get_instance()?.setParentForSelection()],
          ["!L.Ctrl L.Alt P", @() get_instance()?.clearParentForSelection()],
          ["L.Ctrl !L.Alt T", toggleTime],
          ["L.Ctrl !L.Alt S", { action = save, ignoreConsumerCallback = true }], // ignoreConsumerCallback so that freecamera moevement with s works
          ["L.Ctrl !L.Alt L", toggleLogsWindows],
          ["Esc", @() daEditor.setEditMode(DE4_MODE_SELECT)]
        ]
      }
      {
        flow = FLOW_HORIZONTAL
        margin = fsh(1.0)
        size = flex()
        children = [
          txt(markedSceneText.get()).__update(markedSceneStyle, markedSceneShadowStyle)
        ]
      }
    ]
  }
}


return mainToolbar
