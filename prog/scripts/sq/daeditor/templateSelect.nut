from "%darg/ui_imports.nut" import *

let {showTemplateSelect, editorIsActive, showDebugButtons, selectedTemplatesGroup, addEntityCreatedCallback} = require("state.nut")
let {colors} = require("components/style.nut")
let txt = require("%daeditor/components/text.nut").dtext

let textButton = require("components/textButton.nut")
let closeButton = require("components/closeButton.nut")
let nameFilter = require("components/nameFilter.nut")
let combobox = require("%daeditor/components/combobox.nut")
let {makeVertScroll} = require("%daeditor/components/scrollbar.nut")
let {mkTemplateTooltip} = require("components/templateHelp.nut")

let entity_editor = require("entity_editor")
let daEditor = require("daEditorEmbedded")
let {DE4_MODE_SELECT} = daEditor

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
    return ("tpl_name" in desc) && desc.tpl_name==selectedItem.value
  }, 2, false, true)
}

function doSelectTemplate(tpl_name) {
  selectedItem(tpl_name)
  if (selectedItem.value) {
    let finalTemplateName = selectedItem.value + templatePostfixText.value
    entity_editor.get_instance().selectEcsTemplate(finalTemplateName)
  }
}

let filter = nameFilter(filterText, {
  placeholder = "Filter by name"

  function onChange(text) {
    filterText(text)

    if (selectedItem.value && text.len()>0 && selectedItem.value.tolower().contains(text.tolower()))
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
    filterText.update("")
    set_kb_focus(null)
  }
})

let templPostfix = nameFilter(templatePostfixText, {
  placeholder = "Template postfix"

  function onChange(text) {
    templatePostfixText(text)
  }

  function onEscape() {
    set_kb_focus(null)
  }

  function onReturn() {
    set_kb_focus(null)
  }

  function onClear() {
    templatePostfixText.update("")
    set_kb_focus(null)
  }
})

let templateTooltip = Watched(null)

function listRow(tpl_name, idx) {
  let stateFlags = Watched(0)

  return function() {
    let isSelected = selectedItem.value == tpl_name

    local color
    if (isSelected) {
      color = colors.Active
    } else {
      color = (stateFlags.value & S_TOP_HOVER) ? colors.GridRowHover : colors.GridBg[idx % colors.GridBg.len()]
    }

    return {
      rendObj = ROBJ_SOLID
      size = [flex(), SIZE_TO_CONTENT]
      color = color
      behavior = Behaviors.Button
      tpl_name = tpl_name

      watch = stateFlags
      onHover = @(on) templateTooltip(on ? mkTemplateTooltip(tpl_name) : null)
      onClick = @() doSelectTemplate(tpl_name)
      onElemState = @(sf) stateFlags.update(sf & S_TOP_HOVER)

      children = {
        rendObj = ROBJ_TEXT
        text = tpl_name
        margin = fsh(0.5)
      }
    }
  }
}

let selectedGroupTemplates = Computed(@() editorIsActive.value ? entity_editor.get_instance()?.getEcsTemplates(selectedTemplatesGroup.value) ?? [] : [])

let filteredTemplates = Computed(function() {
  let result = []
  foreach (tplName in selectedGroupTemplates.value) {
    if (filterText.value.len()==0 || tplName.tolower().contains(filterText.value.tolower())) {
      result.append(tplName)
    }
  }
  return result
})

let filteredTemplatesCount = Computed(@() filteredTemplates.value.len())
let selectedGroupTemplatesCount = Computed(@() selectedGroupTemplates.value.len())


local showWholeList = false
local showWholeListGroup = ""
filterText.subscribe(@(_) showWholeList = false)

function listMore() {
  return {
    rendObj = ROBJ_SOLID
    size = [flex(), SIZE_TO_CONTENT]
    color = colors.GridBg[0]
    behavior = Behaviors.Button

    onClick = function() {
      showWholeList = true
      selectedTemplatesGroup.trigger()
    }

    children = {
      rendObj = ROBJ_TEXT
      text = "... (show all)"
      margin = fsh(0.5)
    }
  }
}


local doRepeatValidateTemplates = @(_idx) null
function doValidateTemplates(idx) {
  const validateAfterName = ""
  local skipped = 0
  while (idx < selectedGroupTemplates.value.len()) {
    let tplName = selectedGroupTemplates.value[idx]
    if (tplName > validateAfterName) {
      vlog($"Validating template {tplName}...")
      selectedItem(tplName)
      scrollBySelection()
      gui_scene.resetTimeout(0.01, function() {
        doSelectTemplate(tplName)
        doRepeatValidateTemplates(idx+1)
      })
      return
    }
    vlog($"Skipping template {tplName}...")
    if (++skipped > 50) {
      selectedItem(tplName)
      scrollBySelection()
      gui_scene.resetTimeout(0.01, @() doRepeatValidateTemplates(idx+1))
      return
    }
    idx += 1
  }
  vlog("Validation complete")
}
doRepeatValidateTemplates = doValidateTemplates


function dialogRoot() {
  let templatesGroups = entity_editor.get_instance().getEcsTemplatesGroups()
  let maxTemplatesInList = 1000

  function listContent() {
    if (selectedTemplatesGroup.value != showWholeListGroup) {
      showWholeList = false
      showWholeListGroup = selectedTemplatesGroup.value
    }

    let rows = []
    let idx = 0
    foreach (tplName in filteredTemplates.value) {
      if (filterText.value.len()==0 || tplName.tolower().contains(filterText.value.tolower())) {
        rows.append(listRow(tplName, idx))
      }
      if (!showWholeList && rows.len() >= maxTemplatesInList) {
        rows.append(listMore())
        break
      }
    }

    return {
      watch = [filteredTemplates, selectedItem, filterText]
      size = [flex(), SIZE_TO_CONTENT]
      flow = FLOW_VERTICAL
      children = rows
      behavior = Behaviors.Button
    }
  }

  let scrollList = makeVertScroll(listContent, {
    scrollHandler
    rootBase = class {
      size = flex()
      function onAttach() {
        scrollBySelection()
      }
    }
  })


  function doClose() {
    showTemplateSelect(false)
    filterText("")
    daEditor.setEditMode(DE4_MODE_SELECT)
  }

  function doCancel() {
    if (selectedItem.value != null) {
      selectedItem(null)
      entity_editor.get_instance().selectEcsTemplate("")
    }
    else
      doClose()
  }

  return {
    size = [flex(), flex()]
    flow = FLOW_HORIZONTAL

    watch = [filteredTemplatesCount, selectedGroupTemplatesCount, showDebugButtons, templateTooltip]

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
            size = [flex(), SIZE_TO_CONTENT]
            children = [
              txt($"CREATE ENTITY ({filteredTemplatesCount.value}/{selectedGroupTemplatesCount.value})", {
                fontSize = hdpx(15)
                hplace = ALIGN_CENTER
                vplace = ALIGN_CENTER
                size = [flex(), SIZE_TO_CONTENT]
              })
              closeButton(doClose)
            ]
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
            size = [flex(), SIZE_TO_CONTENT]
            halign = ALIGN_CENTER
            valign = ALIGN_CENTER
            hotkeys = [["^Esc", doCancel]]
            children = [
              textButton("Close", doClose)
              showDebugButtons.value ? textButton("Validate", @() doValidateTemplates(0), {boxStyle={normal={borderColor=Color(50,50,50,50)}} textStyle={normal={color=Color(80,80,80,80) fontSize=hdpx(12)}}}) : null
            ]
          }
        ]
      }
      {
        size = [sw(17), sh(60)]
        hplace = ALIGN_LEFT
        vplace = ALIGN_CENTER
        children = templateTooltip.value
      }
    ]
  }
}


return dialogRoot
