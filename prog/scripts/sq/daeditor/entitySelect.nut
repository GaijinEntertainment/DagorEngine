from "%darg/ui_imports.nut" import *

from "%sqstd/ecs.nut" import *

let {showEntitySelect, selectedEntities, de4workMode} = require("state.nut")
let {colors} = require("components/style.nut")
let textButton = require("components/textButton.nut")
let closeButton = require("components/closeButton.nut")
let mkWindow = require("components/window.nut")
let nameFilter = require("components/nameFilter.nut")
let combobox = require("%daeditor/components/combobox.nut")
let scrollbar = require("%daeditor/components/scrollbar.nut")
let {getEntityExtraName} = require("%daeditor/daeditor_es.nut")
let { format } = require("string")
let entity_editor = require("entity_editor")
let mkSortModeButton = require("components/mkSortModeButton.nut")
let selectedGroup = Watched("")
let selectionState = mkWatched(persist, "selectionState", {})
let filterString = mkWatched(persist, "filterString", "")
let scrollHandler = ScrollHandler()
let allEntities = mkWatched(persist, "allEntities", [])

let statusAnimTrigger = { lastN = null }
local locateOnDoubleClick = false

let entitySortState = Watched({})
// for trigger filteredEntites computed only once
local entitySortFuncCache = null

let numSelectedEntities = Computed(function() {
  local nSel = 0
  foreach (v in selectionState.value) {
    if (v)
      ++nSel
  }
  return nSel
})


let function matchEntityByText(eid, text) {
  if (text==null || text=="" || eid.tostring().indexof(text)!=null)
    return true
  let tplName = g_entity_mgr.getEntityTemplateName(eid)
  if (tplName==null)
    return false
  if (tplName.tolower().contains(text.tolower()))
    return true
  let riExtraName = getEntityExtraName(eid)
  if (riExtraName != null && riExtraName.tolower().contains(text.tolower()))
    return true
  return false
}

let filteredEntites = Computed(function() {
  local entities = allEntities.value
  if (filterString.value != "")
    entities = entities.filter(@(eid) matchEntityByText(eid, filterString.value))
  if (entitySortFuncCache != null)
    entities.sort(entitySortFuncCache)
  return entities
})

let filteredEntitiesCount = Computed(@() filteredEntites.value.len())

let function applySelection(cb) {
  selectionState.mutate(function(value) {
    foreach (k, v in value)
      value[k] = cb(k, v)
  })
}

// use of filteredEntites here would be more correct here, but reapplying name check should faster than
// linear search in array (O(N) vs O(N^2))
let selectAllFiltered = @() applySelection(@(eid, _cur) matchEntityByText(eid, filterString.value))

let selectNone = @() applySelection(@(_eid, _cur) false)

// invert filtered, deselect unfiltered
let selectInvert = @() applySelection(@(eid, cur) matchEntityByText(eid, filterString.value) ? !cur : false)


let function scrollBySelection() {
  scrollHandler.scrollToChildren(function(desc) {
    return ("eid" in desc) && selectionState.value?[desc.eid]
  }, 2, false, true)
}


let function doSelect() {
  let eids = []
  foreach (k, v in selectionState.value) if (v) eids.append(k)
  entity_editor.get_instance().selectEntities(eids)
  gui_scene.resetTimeout(0.1, function() {
    selectedEntities.trigger()
    selectionState.trigger()
  })
//  filterString.update("")
}

let function doLocate() {
  let eids = []
  foreach (k, v in selectionState.value) if (v) eids.append(k)
  entity_editor.get_instance().selectEntities(eids)
  entity_editor.get_instance().zoomAndCenter()
  //showEntitySelect(false)
}


let function doCancel() {
  showEntitySelect(false)
//  filterString.update("")
}


let function statusLine() {
  let nMrk = numSelectedEntities.value
  let nSel = selectedEntities.value.len()

  if (statusAnimTrigger.lastN != null && statusAnimTrigger.lastN != nSel)
    anim_start(statusAnimTrigger)
  statusAnimTrigger.lastN = nSel

  return {
    watch = [numSelectedEntities, filteredEntitiesCount, selectedEntities]
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_HORIZONTAL
    children = [
      {
         rendObj = ROBJ_TEXT
         size = [flex(), SIZE_TO_CONTENT]
         text = format(" %d %s marked, %d selected", nMrk, nMrk==1 ? "entity" : "entities", nSel)
         animations = [
           { prop=AnimProp.color, from=colors.HighlightSuccess, duration=0.5, trigger=statusAnimTrigger }
         ]
      }
      {
        rendObj = ROBJ_TEXT
        halign = ALIGN_RIGHT
        size = [flex(), SIZE_TO_CONTENT]
        text = format("%d listed   ", filteredEntitiesCount.value)
        color = Color(170,170,170)
     }
    ]
  }
}


let filter = nameFilter(filterString, {
  placeholder = "Filter by name"

  function onChange(text) {
    filterString(text)
  }

  function onEscape() {
    set_kb_focus(null)
  }

  function onReturn() {
    set_kb_focus(null)
  }

  function onClear() {
    filterString.update("")
    set_kb_focus(null)
  }
})

let function doSelectEid(eid, mod) {
  let eids = []
  local found = false
  foreach (k, _v in selectedEntities.value) {
    if (k == eid)
      found = true
    else if (mod)
      eids.append(k)
  }
  if (!found)
    eids.append(eid)
  entity_editor.get_instance().selectEntities(eids)
  gui_scene.resetTimeout(0.1, @() selectionState.trigger())
}

let removeSelectedByEditorTemplate = @(tname) tname.replace("+daeditor_selected+","+").replace("+daeditor_selected","").replace("daeditor_selected+","")

let function listRow(eid, idx) {
  return watchElemState(function(sf) {
    let isSelected = selectionState.value?[eid]
    let color = isSelected ? colors.Active
      : sf & S_TOP_HOVER ? colors.GridRowHover
      : colors.GridBg[idx % colors.GridBg.len()]

    let extraName = getEntityExtraName(eid)
    let extra = (extraName != null) ? $"/ {extraName}" : ""

    local tplName = g_entity_mgr.getEntityTemplateName(eid) ?? ""
    let name = removeSelectedByEditorTemplate(tplName)
    let div = (tplName != name) ? "•" : "|"

    return {
      rendObj = ROBJ_SOLID
      size = [flex(), SIZE_TO_CONTENT]
      color
      eid
      behavior = Behaviors.Button

      function onClick(evt) {
        if (evt.shiftKey) {
          local selCount = 0
          foreach (_k, v in selectionState.value) {
            if (v)
              ++selCount
          }
          if (selCount > 0) {
            local idx1 = -1
            local idx2 = -1
            foreach (i, filteredEid in filteredEntites.value) {
              if (eid == filteredEid) {
                idx1 = i
                idx2 = i
              }
            }
            foreach (i, filteredEid in filteredEntites.value) {
              if (selectionState.value?[filteredEid]) {
                if (idx1 > i)
                  idx1 = i
                if (idx2 < i)
                  idx2 = i
              }
            }
            if (idx1 >= 0 && idx2 >= 0) {
              if (idx1 > idx2) {
                let tmp = idx1
                idx1 = idx2
                idx2 = tmp
              }
              selectionState.mutate(function(value) {
                for (local i = idx1; i <= idx2; i++) {
                  let filteredEid = filteredEntites.value[i]
                  value[filteredEid] <- !evt.ctrlKey
                }
              })
            }
          }
        }
        else if (evt.ctrlKey) {
          selectionState.mutate(function(value) {
            value[eid] <- !value?[eid]
          })
        }
        else {
          applySelection(@(eid_, _cur) eid_==eid)
        }
      }

      onDoubleClick = function(evt) {
        if (locateOnDoubleClick) { doLocate(); return }
        locateOnDoubleClick = true
        gui_scene.resetTimeout(0.3, @() locateOnDoubleClick = false)
        doSelectEid(eid, evt.ctrlKey)
      }

      children = {
        rendObj = ROBJ_TEXT
        text = $"{eid}  {div}  {name} {extra}"
        margin = fsh(0.5)
      }
    }
  })
}

let function listRowMoreLeft(num, idx) {
  return watchElemState(function(sf) {
    let color = (sf & S_TOP_HOVER) ? colors.GridRowHover : colors.GridBg[idx % colors.GridBg.len()]
    return {
      rendObj = ROBJ_SOLID
      size = [flex(), SIZE_TO_CONTENT]
      color
      children = {
        rendObj = ROBJ_TEXT
        text = $"{num} more ..."
        margin = fsh(0.5)
        color = Color(160,160,160,160)
      }
    }
  })
}


let function initEntitiesList() {
  let entities = entity_editor.get_instance()?.getEntities(selectedGroup.value) ?? []
  foreach (eid in entities) {
    let isSelected = selectedEntities.value?[eid] ?? false
    selectionState.value[eid] <- isSelected
  }
  allEntities(entities)
  selectionState.trigger()
}

entitySortState.subscribe(function(v) {
  entitySortFuncCache = v?.func
  selectedEntities.trigger()
  selectionState.trigger()
  initEntitiesList()
})

selectedGroup.subscribe(@(_) initEntitiesList())
de4workMode.subscribe(@(_) gui_scene.resetTimeout(0.1, initEntitiesList))

let function entitySelectRoot() {
  let templatesGroups = ["(all workset entities)"].extend(entity_editor.get_instance().getEcsTemplatesGroups())

  let function listContent() {
    const maxVisibleItems = 500
    let rows = filteredEntites.value.slice(0, maxVisibleItems).map(@(eid, idx) listRow(eid, idx))
    if (rows.len() < filteredEntites.value.len())
      rows.append(listRowMoreLeft(filteredEntites.value.len() - rows.len(), rows.len()))

    return {
      watch = [selectionState, filteredEntites]
      size = [flex(), SIZE_TO_CONTENT]
      flow = FLOW_VERTICAL
      children = rows
      behavior = Behaviors.Button
    }
  }


  let scrollList = scrollbar.makeVertScroll(listContent, {
    scrollHandler
    rootBase = class {
      size = flex()
      function onAttach() {
        scrollBySelection()
      }
    }
  })

  let content = @() {
    flow = FLOW_VERTICAL
    gap = fsh(0.5)
    watch = [allEntities, selectedEntities]
    size = flex()
    children = [
      {
        size = [flex(), SIZE_TO_CONTENT]
        flow = FLOW_HORIZONTAL
        children = [
          mkSortModeButton(entitySortState)
          { size = [sw(0.2), SIZE_TO_CONTENT] }
          filter
          {
            size = [sw(11), sh(2.7)]
            children = combobox(selectedGroup, templatesGroups)
          }
          closeButton(doCancel)
        ]
      }
      {
        size = flex()
        children = scrollList
      }
      statusLine
      {
        flow = FLOW_HORIZONTAL
        size = [flex(), SIZE_TO_CONTENT]
        halign = ALIGN_CENTER
        children = [
          textButton("All filtered", selectAllFiltered)
          textButton("None",   selectNone)
          textButton("Invert", selectInvert)
        ]
      }
      {
        flow = FLOW_HORIZONTAL
        size = [flex(), SIZE_TO_CONTENT]
        halign = ALIGN_CENTER
        children = [
          textButton("Select", doSelect, {hotkeys=["^Enter"]})
          textButton("Locate", doLocate, {hotkeys=["^Z"]})
          textButton("Close",  doCancel, {hotkeys=["^Esc"]})
        ]
      }
    ]
  }
  return mkWindow({
    onAttach = initEntitiesList
    id = "entity_select"
    content
    saveState = true
  })()
}


return entitySelectRoot
