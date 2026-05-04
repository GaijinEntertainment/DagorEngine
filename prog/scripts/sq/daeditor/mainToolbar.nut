import "daEditorEmbedded" as daEditor
from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let { DE4_MODE_CREATE_ENTITY = null, get_instance = @() null } = require_optional("entity_editor")
let { LogsWindowId, EntitySelectWndId, SceneOutlinerWndId, propPanelVisible, propPanelClosed,
  showHelp, markedScenes, de4editMode, de4workMode, de4workModes, showUIinEditor, editorTimeStop,
  gizmoBasisType, gizmoBasisTypeNames, gizmoBasisTypeEditingDisabled, gizmoCenterType,
  gizmoCenterTypeNames, sceneIdMap, updateAllScenes } = require("state.nut")

let { sortScenesByLoadType } = require("components/sceneSorting.nut")

let pictureButton = require("components/pictureButton.nut")
let { addModalWindow, removeModalWindow } = require("%daeditor/components/modalWindows.nut")
let { makeVertScroll } = require("%daeditor/components/scrollbar.nut")
let { colors } = require("components/style.nut")
let textButton = require("components/textButton.nut")
let combobox = require("%daeditor/components/combobox.nut")
let cursors = require("%daeditor/components/cursors.nut")
let mkCheckBox = require("components/mkCheckBox.nut")
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
    checked = mode == getEditMode()
    imageMargin = fsh(0.5)
    onHover = @(on) cursors.setTooltip(on ? tooltip_text : null)
    action = function() {
      hideWindow(EntitySelectWndId)
      hideWindow(SceneOutlinerWndId)
      if (next_mode && mode==getEditMode())
        mode = next_mode
      daEditor.setEditMode(mode)
      if (next_action)
        next_action()
    }
  })
  return pictureButton(params)
}


let separator = const {
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
function toggleSceneOutliner() {
  if (getEditMode() == DE4_MODE_CREATE_ENTITY || getEditMode() == DE4_MODE_POINT_ACTION)
    setEditMode(DE4_MODE_SELECT)
  toggleWindow(SceneOutlinerWndId)
}
function toggleLogsWindows() {
  toggleWindow(LogsWindowId)
}
function toggleCreateEntityMode() {
  hideWindow(EntitySelectWndId)
  hideWindow(SceneOutlinerWndId)
  local mode = DE4_MODE_CREATE_ENTITY
  if (DE4_MODE_CREATE_ENTITY==getEditMode())
    mode = DE4_MODE_SELECT
  daEditor.setEditMode(mode)
}
function togglePropPanel() {
  propPanelClosed.set(propPanelVisible.get() && !showHelp.get())
  propPanelVisible.modify(invert)
}
let isVisibleEntitiesSelect = mkIsWindowVisible(EntitySelectWndId)
let isVisibleSceneOutlinerWnd = mkIsWindowVisible(SceneOutlinerWndId)
let isVisibleLogsWnd = mkIsWindowVisible(LogsWindowId)

const saveScenesWindowUID = "save_scenes_modal_window"

function showMessageboxSaveScenes(modifiedSceneIds) {
  let close = @() removeModalWindow(saveScenesWindowUID)
  let selectionsState = Watched([])
  selectionsState.get().resize(modifiedSceneIds.len(), false)

  let isAnySceneSelected = Computed(function () {
    foreach (selected in selectionsState.get()) {
      if (selected) {
        return true
      }
    }

    return false
  })

  let rows = modifiedSceneIds.map(function (sceneId, index) {
    return watchElemState(function (_sf) {
      return {
        rendObj = ROBJ_SOLID
        size = [flex(), SIZE_TO_CONTENT]
        color = colors.GridBg[index % colors.GridBg.len()]
        children = {
          flow = FLOW_HORIZONTAL
          valign = ALIGN_CENTER
          gap = fsh(0.5)
          margin = fsh(0.5)
          children = [
            mkCheckBox(Computed(@() selectionsState.get()[index]), function() {
              selectionsState.mutate(function(value) {
                value[index] = !value[index]
              })
            })
            {
              rendObj = ROBJ_TEXT
              text = sceneIdMap?.get()[sceneId].path
            }
          ]
        }
      }
    })
  })

  addModalWindow({
    key = saveScenesWindowUID
    children = {
      behavior = Behaviors.Button
      gap = fsh(0.5)
      flow = FLOW_VERTICAL
      rendObj = ROBJ_SOLID
      size = const [hdpx(700), hdpx(400)]
      color = Color(50,50,50)
      hplace = ALIGN_CENTER
      vplace = ALIGN_CENTER
      padding = hdpx(10)
      children = [
        {
          children = txt("Do you want to save scenes?")
        }
        {
          size = flex()
          children = makeVertScroll(function() {
            return {
              size = FLEX_H
              flow = FLOW_VERTICAL
              children = rows
              behavior = Behaviors.Button
            }
          })
        }
        {
          flow = FLOW_HORIZONTAL
          size = [flex(), SIZE_TO_CONTENT]
          children = [
            hflow(
              Size(SIZE_TO_CONTENT, SIZE_TO_CONTENT)
              textButton("Save all", function () {
                get_instance()?.saveDirtyScenes()
                close()
              })
              @() { watch = isAnySceneSelected, children = textButton("Save selected", function () {
                  let selectedSceneIds = []
                  for (local i = 0; i < modifiedSceneIds.len(); ++i) {
                    if (selectionsState.get()[i]) {
                      selectedSceneIds.append(modifiedSceneIds[i])
                    }
                  }

                  get_instance()?.saveScenes(selectedSceneIds)
                  close()
                },
                { off = !isAnySceneSelected.get(), disabled = Computed(@() !isAnySceneSelected.get() )}) }
            )
            hflow(
              Size(flex(), SIZE_TO_CONTENT)
              HARight
              textButton("Cancel", close, {hotkeys=[["Esc"]]})
            )
          ]
        }
      ]
    }
  })
}

let markedSceneText = Computed(function() {
  local nMrk = 0
  local path = ""
  local scenes = get_instance()?.getSceneImports().map(function (item, ind) {
      item.index <- ind
      return item
      }) ?? []
  scenes.sort(sortScenesByLoadType)
  foreach (scene in scenes) {
    local isMarked = markedScenes.get()?[scene?.id] ?? false
    if (isMarked) {
      if (nMrk == 0) {
        path = scene.path
      }
      nMrk++
    }
  }

  if (nMrk > 0) {
    local andMore = nMrk > 1 ? $", and {nMrk - 1} more" : ""
    return $"Editing: {path}{andMore}"
  }
  return ""
})

function mainToolbar() {
  let toggleTime = @() editorTimeStop.set(!editorTimeStop.get())
  let toggleHelp = @() showHelp.modify(@(v) !v)
  function isMainScene(id) {
    let scene = sceneIdMap?.get()[id]
    return scene != null && scene.importDepth == 0 && !scene.hasParent
  }
  function save() {
    let modifiedSceneIds = get_instance()?.getModifiedSceneIds() ?? []
    if (modifiedSceneIds.len() != 0) {
      updateAllScenes()

      if (modifiedSceneIds.len() == 1 && isMainScene(modifiedSceneIds[0])) {
        get_instance()?.saveMainScene()
      }
      else {
        showMessageboxSaveScenes(modifiedSceneIds)
      }
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
          toolbarButton(svg("scenes"), toggleSceneOutliner, "Scene Outliner (I)", isVisibleSceneOutlinerWnd)
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
            size = [hdpx(150), fontH(100)]
            children = combobox({value = gizmoBasisType, disable = gizmoBasisTypeEditingDisabled}, gizmoBasisTypeNames, gizmoBasisTypeEditingDisabled.get() ? "Set gizmo basis mode (X)\n\nEnabled when the move/rotate/scale/surf over ground edit mode is active." : "Set gizmo basis mode (X)")
          }
          @() {
            watch = gizmoBasisTypeEditingDisabled
            size = [hdpx(150), fontH(100)]
            children = combobox({value = gizmoCenterType, disable = gizmoBasisTypeEditingDisabled}, gizmoCenterTypeNames, gizmoBasisTypeEditingDisabled.get() ? "Set gizmo transformation center mode (C)\n\nEnabled when the move/rotate/scale/surf over ground edit mode is active." : "Set gizmo transformation center mode (C)")
          }
        ]

        hotkeys = [
          ["L.Ctrl !L.Alt I", toggleSceneOutliner],
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
