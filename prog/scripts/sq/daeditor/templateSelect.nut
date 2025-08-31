import "daEditorEmbedded" as daEditor
from "%darg/ui_imports.nut" import *

let entity_editor = require_optional("entity_editor")
let { showTemplateSelect, editorIsActive, showDebugButtons, selectedTemplatesGroup, addEntityCreatedCallback } = require("state.nut")
let { colors } = require("components/style.nut")
let txt = require("%daeditor/components/text.nut").dtext

let textButton = require("components/textButton.nut")
let closeButton = require("components/closeButton.nut")
let nameFilter = require("components/nameFilter.nut")
let combobox = require("%daeditor/components/combobox.nut")
let { makeVertScroll } = require("%daeditor/components/scrollbar.nut")
let { mkTemplateTooltip } = require("components/templateHelp.nut")

let { getSceneLoadTypeText } = require("%daeditor/daeditor_es.nut")
let { defaultScenesSortMode } = require("components/mkSortSceneModeButton.nut")
let {DE4_MODE_SELECT} = daEditor

const noSceneSelected = "UNKNOWN:0"
let allScenes = Watched([])
let allSceneTexts = Watched([noSceneSelected])
let selectedScene = Watched(noSceneSelected)
let selectedItem = Watched(null)
let filterText = Watched("")
let templatePostfixText = Watched("")

let scrollHandler = ScrollHandler()

addEntityCreatedCallback(@(_eid) set_kb_focus(null))

function scrollByName(text) {
  scrollHandler.scrollToChildren(function(desc) {
    return ("tpl_name" in desc) && desc.tpl_name.indexof(text)!=null
  }, 2, false, true)
}

function scrollBySelection() {
  scrollHandler.scrollToChildren(function(desc) {
    return ("tpl_name" in desc) && desc.tpl_name==selectedItem.get()
  }, 2, false, true)
}

function doSelectTemplate(tpl_name) {
  selectedItem.set(tpl_name)
  if (selectedItem.get()) {
    let finalTemplateName = selectedItem.get() + templatePostfixText.get()
    entity_editor?.get_instance().selectEcsTemplate(finalTemplateName)
  }
}

let filter = nameFilter(filterText, {
  placeholder = "Filter by name"

  function onChange(text) {
    filterText.set(text)

    if (selectedItem.get() && text.len()>0 && selectedItem.get().tolower().contains(text.tolower()))
      scrollBySelection()
    else if (text.len())
      scrollByName(text)
    else
      scrollBySelection()
  }

  function onEscape() {
    set_kb_focus(null)
  }

  function onReturn() {
    set_kb_focus(null)
  }

  function onClear() {
    filterText.set("")
    set_kb_focus(null)
  }
})

let templPostfix = nameFilter(templatePostfixText, {
  placeholder = "Template postfix"

  function onChange(text) {
    templatePostfixText.set(text)
  }

  function onEscape() {
    set_kb_focus(null)
  }

  function onReturn() {
    set_kb_focus(null)
  }

  function onClear() {
    templatePostfixText.set("")
    set_kb_focus(null)
  }
})

let templateTooltip = Watched(null)

function listRow(tpl_name, idx) {
  let stateFlags = Watched(0)

  return function() {
    let isSelected = selectedItem.get() == tpl_name

    local color
    if (isSelected) {
      color = colors.Active
    } else {
      color = (stateFlags.get() & S_TOP_HOVER) ? colors.GridRowHover : colors.GridBg[idx % colors.GridBg.len()]
    }

    return {
      rendObj = ROBJ_SOLID
      size = FLEX_H
      color = color
      behavior = Behaviors.Button
      tpl_name = tpl_name

      watch = stateFlags
      onHover = @(on) templateTooltip.set(on ? mkTemplateTooltip(tpl_name) : null)
      onClick = @() doSelectTemplate(tpl_name)
      onElemState = @(sf) stateFlags.set(sf & S_TOP_HOVER)

      children = {
        rendObj = ROBJ_TEXT
        text = tpl_name
        color = colors.TextDefault
        margin = fsh(0.5)
      }
    }
  }
}

let selectedGroupTemplates = Computed(@() editorIsActive.get()
  ? entity_editor?.get_instance().getEcsTemplates(selectedTemplatesGroup.get()) ?? [] : [])

let filteredTemplates = Computed(function() {
  let result = []
  foreach (tplName in selectedGroupTemplates.get()) {
    if (filterText.get().len()==0 || tplName.tolower().contains(filterText.get().tolower())) {
      result.append(tplName)
    }
  }
  return result
})

let filteredTemplatesCount = Computed(@() filteredTemplates.get().len())
let selectedGroupTemplatesCount = Computed(@() selectedGroupTemplates.get().len())


local showWholeList = false
local showWholeListGroup = ""
filterText.subscribe(@(_) showWholeList = false)

function listMore() {
  return {
    rendObj = ROBJ_SOLID
    size = FLEX_H
    color = colors.GridBg[0]
    behavior = Behaviors.Button

    onClick = function() {
      showWholeList = true
      selectedTemplatesGroup.trigger()
    }

    children = {
      rendObj = ROBJ_TEXT
      text = "... (show all)"
      color = colors.TextDarker
      margin = fsh(0.5)
    }
  }
}


local doRepeatValidateTemplates = @(_idx) null
function doValidateTemplates(idx) {
  const validateAfterName = ""
  local skipped = 0
  while (idx < selectedGroupTemplates.get().len()) {
    let tplName = selectedGroupTemplates.get()[idx]
    if (tplName > validateAfterName) {
      vlog($"Validating template {tplName}...")
      selectedItem.set(tplName)
      scrollBySelection()
      gui_scene.resetTimeout(0.01, function() {
        doSelectTemplate(tplName)
        doRepeatValidateTemplates(idx+1)
      })
      return
    }
    vlog($"Skipping template {tplName}...")
    if (++skipped > 50) {
      selectedItem.set(tplName)
      scrollBySelection()
      gui_scene.resetTimeout(0.01, @() doRepeatValidateTemplates(idx+1))
      return
    }
    idx += 1
  }
  vlog("Validation complete")
}
doRepeatValidateTemplates = doValidateTemplates

function sceneToText(scene) {
  if (scene.importDepth != 0) {
    local loadType = getSceneLoadTypeText(scene)
    return $"{loadType}:{scene.index}"
  }
  return "MAIN"
}

allScenes.subscribe_with_nasty_disregard_of_frp_update(function(v) {
  allSceneTexts.set(v.map(@(scene, _idx) sceneToText(scene)))
  allSceneTexts.get().append(noSceneSelected)
  local scene = entity_editor?.get_instance().getTargetScene()
  if (scene != null && ("loadType" in scene) && ("index" in scene)) {
    selectedScene.set(sceneToText(scene))
  } else {
    selectedScene.set(noSceneSelected)
  }
})

selectedScene.subscribe(function(v) {
  local loadType = 0
  local index = 0
  if (v != noSceneSelected) {
    local selectedSceneIndex = allSceneTexts.get().indexof(v)
    if (selectedSceneIndex != null) {
      local scene = allScenes.get()[selectedSceneIndex]
      loadType = scene.loadType
      index = scene.index
    }
  }
  entity_editor?.get_instance().setTargetScene(loadType, index)
})


function dialogRoot() {
  let templatesGroups = entity_editor?.get_instance().getEcsTemplatesGroups()
  let maxTemplatesInList = 1000

  local scenes = entity_editor?.get_instance().getSceneImports() ?? []
  scenes.sort(defaultScenesSortMode.func)
  allScenes.set(scenes)
  let selectedSceneIndex = selectedScene.get() != noSceneSelected ? allSceneTexts.get().indexof(selectedScene.get()) : null
  let sceneInfo = selectedSceneIndex != null ? scenes[selectedSceneIndex] : null
  let sceneTitleStyle = { fontSize = hdpx(17), color=Color(150,150,150,120) }
  let sceneInfoStyle = { fontSize = hdpx(17), color=Color(180,180,180,120) }
  let sceneTooltip = function() {
    return {
      rendObj = ROBJ_BOX
      fillColor = Color(30, 30, 30, 220)
      borderColor = Color(50, 50, 50, 110)
      size = SIZE_TO_CONTENT
      borderWidth = hdpx(1)
      padding = fsh(1)
      flow = FLOW_VERTICAL
      children = [
        txt("Select target scene to create entity in", sceneTitleStyle)
        txt(selectedScene.get(), sceneInfoStyle)
        sceneInfo != null ? txt($"{sceneInfo.path}", sceneInfoStyle) : null
      ]

    }
  }

  function listContent() {
    if (selectedTemplatesGroup.get() != showWholeListGroup) {
      showWholeList = false
      showWholeListGroup = selectedTemplatesGroup.get()
    }

    let rows = []
    let idx = 0
    foreach (tplName in filteredTemplates.get()) {
      if (filterText.get().len()==0 || tplName.tolower().contains(filterText.get().tolower())) {
        rows.append(listRow(tplName, idx))
      }
      if (!showWholeList && rows.len() >= maxTemplatesInList) {
        rows.append(listMore())
        break
      }
    }

    return {
      watch = [filteredTemplates, selectedItem, filterText]
      size = FLEX_H
      flow = FLOW_VERTICAL
      children = rows
      behavior = Behaviors.Button
    }
  }

  let scrollList = makeVertScroll(listContent, {
    scrollHandler
    rootBase = {
      size = flex()
      function onAttach() {
        scrollBySelection()
      }
    }
  })


  function doClose() {
    showTemplateSelect(false)
    filterText.set("")
    daEditor.setEditMode(DE4_MODE_SELECT)
  }

  function doCancel() {
    if (selectedItem.get() != null) {
      selectedItem.set(null)
      entity_editor?.get_instance().selectEcsTemplate("")
    }
    else
      doClose()
  }

  return {
    size = [flex(), flex()]
    flow = FLOW_HORIZONTAL

    watch = [filteredTemplatesCount, selectedGroupTemplatesCount, showDebugButtons, templateTooltip, selectedScene]

    children = [
      {
        size = [sw(17), sh(75)]
        hplace = ALIGN_LEFT
        vplace = ALIGN_CENTER
        rendObj = ROBJ_SOLID
        color = colors.ControlBg
        flow = FLOW_VERTICAL
        halign = ALIGN_CENTER
        behavior = Behaviors.Button
        key = "template_select"
        padding = fsh(0.5)
        gap = fsh(0.5)

        children = [
          {
            flow = FLOW_HORIZONTAL
            size = FLEX_H
            children = [
              txt($"CREATE ENTITY ({filteredTemplatesCount.get()}/{selectedGroupTemplatesCount.get()})", {
                fontSize = hdpx(15)
                hplace = ALIGN_CENTER
                vplace = ALIGN_CENTER
                size = FLEX_H
              })
              closeButton(doClose)
            ]
          }

          {
            size = [flex(),fontH(100)]
            children = combobox(selectedScene, allSceneTexts, sceneTooltip)
          }

          {
            size = [flex(),fontH(100)]
            children = combobox(selectedTemplatesGroup, templatesGroups)
          }
          filter
          {
            size = flex()
            children = scrollList
          }
          templPostfix
          {
            flow = FLOW_HORIZONTAL
            size = FLEX_H
            halign = ALIGN_CENTER
            valign = ALIGN_CENTER
            hotkeys = [["^Esc", doCancel]]
            children = [
              textButton("Close", doClose)
              showDebugButtons.get() ? textButton("Validate", @() doValidateTemplates(0), {boxStyle={normal={borderColor=Color(50,50,50,50)}} textStyle={normal={color=Color(80,80,80,80) fontSize=hdpx(12)}}}) : null
            ]
          }
        ]
      }
      {
        size = [sw(17), sh(60)]
        hplace = ALIGN_LEFT
        vplace = ALIGN_CENTER
        children = templateTooltip.get()
      }
    ]
  }
}


return dialogRoot
