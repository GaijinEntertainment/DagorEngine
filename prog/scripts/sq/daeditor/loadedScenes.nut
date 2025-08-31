from "string" import format
from "%darg/ui_imports.nut" import *
from "%sqstd/ecs.nut" import *

let entity_editor = require_optional("entity_editor")
let { LoadedScenesWndId, selectedEntities, markedScenes, de4workMode } = require("state.nut")
let { colors } = require("components/style.nut")
let textButton = require("components/textButton.nut")
let nameFilter = require("components/nameFilter.nut")
let { makeVertScroll } = require("%daeditor/components/scrollbar.nut")
let { getSceneLoadTypeText, getSceneId, getSceneIdOf, getSceneIdLoadType, getSceneIdIndex, getSceneIndicies, loadTypeConst, sceneGenerated, sceneSaved, getNumMarkedScenes, matchSceneEntity, matchEntityByScene } = require("%daeditor/daeditor_es.nut")
let { defaultScenesSortMode, mkSceneSortModeButton } = require("components/mkSortSceneModeButton.nut")
let scrollHandler = ScrollHandler()
let markedStateScenes = mkWatched(persist, "markedStateScenes", {})
let editedStateScenes = mkWatched(persist, "editedStateScenes", {})
let filterString = mkWatched(persist, "filterString", "")
let filterScenesBySelectedEntities = mkWatched(persist, "filterScenesBySelectedEntities", true)
let allScenes = mkWatched(persist, "allScenes", [])
let allSceneIndices = mkWatched(persist, "allSceneIndices", [])
let selectionStateEntities = mkWatched(persist, "selectionStateEntities", {})
let allEntities = mkWatched(persist, "allEntities", [])
let filteredEntities = Watched([])

let sceneSortState = Watched(defaultScenesSortMode)
// for trigger filteredScenes computed only once
local sceneSortFuncCache = defaultScenesSortMode.func

let statusAnimTrigger = { lastN = null }

let selectedEntitiesSceneIds = Computed(function() {
  local res = [[], /* common */ [], /* client */ [], /* import */ [], /* const */ []]
  local generated = false
  local saved = false
  foreach (eid, _v in selectedEntities.get()) {
    local loadType = entity_editor?.get_instance().getEntityRecordLoadType(eid)
    if (loadType != 0) {
      local index = entity_editor?.get_instance().getEntityRecordIndex(eid)
      res[loadType].append(index)
    }
    if (!generated || !saved) {
      local isScene = entity_editor?.get_instance().isSceneEntity(eid)
      generated = generated || (isScene == false)
      saved = saved || (isScene == true)
    }
  }
  if (generated)
    res[loadTypeConst].append(sceneGenerated.index)
  if (saved)
    res[loadTypeConst].append(sceneSaved.index)
  return res
})

function getSelectedIdsCount(selectedIds) {
  local selectedIdsCount = 0
  foreach (_i, ids in selectedIds)
    selectedIdsCount += ids.len()
  return selectedIdsCount
}

function matchSceneBySelectedEntities(scene, selectedIds) {
  return selectedIds[scene.loadType].indexof(scene.index) != null
}

function sceneToText(scene) {
  if (scene.loadType == loadTypeConst) {
    return scene.asText
  }
  local sceneId = getSceneIdOf(scene)
  local edit = editedStateScenes.get()?[sceneId] ? "> " : "| "
  local loadType = "MAIN"
  local idSeparator = ""
  local index = ""
  if (scene.importDepth != 0) {
    loadType = getSceneLoadTypeText(scene)
    idSeparator = ":"
    index = scene.index
  }
  local path = scene.path
  local entityCount = scene.entityCount
  local order = scene.order
  local relation = ""
  if (sceneSortFuncCache == defaultScenesSortMode.func) {
    if (scene.hasParent) {
      local prefix = ""
      local loadTypeIndex = allSceneIndices.get()[scene.loadType]
      local parentScene = allScenes.get()[loadTypeIndex + scene.parent]
      while (parentScene.hasParent) {
        prefix = $"    {prefix}"
        parentScene = allScenes.get()[loadTypeIndex + parentScene.parent]
      }
      relation = $"{prefix}{scene.imports > 0 ? "+- " : "-- "}"
    }
  }
  return $"{edit}{loadType}{idSeparator}{index}  {relation}{path} - Entities: {entityCount}  (#{order})"
}

function matchSceneByText(scene, text) {
  if (text==null || text=="")
    return true
  if (sceneToText(scene).tolower().indexof(text.tolower()) != null)
    return true
  return false
}

function matchSceneByFilters(scene, selectedIds, selectedIdsCount) {
  if (selectedIdsCount > 0)
    if (matchSceneBySelectedEntities(scene, selectedIds))
      return true
  if (matchSceneByText(scene, filterString.get()))
    return true
  return false
}

let filteredScenes = Computed(function() {
  local scenes = allScenes.get().map(@(scene) scene) // get a copy to avoid sorting allScenes
  scenes = [sceneGenerated, sceneSaved].extend(scenes)
  if (filterScenesBySelectedEntities.get()) {
    local selectedIds = selectedEntitiesSceneIds.get()
    local selectedIdsCount = getSelectedIdsCount(selectedIds)
    if (selectedIdsCount > 0)
      scenes = scenes.filter(@(scene) matchSceneBySelectedEntities(scene, selectedIds))
  }
  if (filterString.get() != "")
    scenes = scenes.filter(@(scene) matchSceneByText(scene, filterString.get()))
  if (sceneSortFuncCache != null)
    scenes.sort(sceneSortFuncCache)
  return scenes
})

let filteredScenesCount = Computed(@() filteredScenes.get().len())

let filteredScenesEntityCount = Computed(function() {
  local eCount = 0
  foreach (scene in filteredScenes.get()) {
    eCount += scene.entityCount
  }
  return eCount
})

function filterEntities() {
  local entities = allEntities.get()

  if (getNumMarkedScenes() > 0) {
    local savedMarked = markedStateScenes.get()?[sceneSaved.id]
    local generatedMarked = markedStateScenes.get()?[sceneGenerated.id]
    entities = entities.filter(@(eid) matchEntityByScene(eid, savedMarked, generatedMarked))
  }

  filteredEntities.set(entities)

  entity_editor?.get_instance().unhideAll()
  entity_editor?.get_instance().hideUnmarkedEntities(filteredEntities.get())
}

let persistMarkedScenes = function(v) {
  if (v) {
    local scenes = markedScenes.get()
    foreach (sceneId, marked in v)
      scenes[sceneId] <- marked
  }
}
markedScenes.whiteListMutatorClosure(persistMarkedScenes)
markedStateScenes.subscribe_with_nasty_disregard_of_frp_update(function(v) {
  persistMarkedScenes(v)
  markedScenes.trigger()
  filterEntities()
})
editedStateScenes.subscribe_with_nasty_disregard_of_frp_update(@(_v) filterEntities())

function calculateSceneEntityCount(saved, generated) {
  local entities = allEntities.get()
  entities = entities.filter(@(eid) matchSceneEntity(eid, saved, generated))
  return entities.len()
}

let numMarkedScenesEntityCount = Computed(function() {
  local nMrkSaved = 0
  local nMrkGenerated = 0
  local nSaved = 0
  local nGenerated = 0
  foreach (scene in filteredScenes.get()) {
    local sceneId = getSceneIdOf(scene)
    if (markedStateScenes.get()?[sceneId]) {
      if (scene.loadType == loadTypeConst) {
        if (scene == sceneSaved)
          nSaved = calculateSceneEntityCount(true, false)
        if (scene == sceneGenerated)
          nGenerated = calculateSceneEntityCount(false, true)
      } else {
        if (scene.importDepth == 0 || editedStateScenes.get()?[sceneId])
          nMrkSaved += scene.entityCount
        else
          nMrkGenerated += scene.entityCount
      }
    }
  }
  return (nSaved > 0 ? nSaved : nMrkSaved) + (nGenerated > 0 ? nGenerated : nMrkGenerated)
})

function markScene(cb) {
  markedStateScenes.mutate(function(value) {
    foreach (k, v in value)
      value[k] = cb(k, v)
  })
}

// use of filteredScenes here would be more correct here, but reapplying name check should be faster than
// linear search in array (O(N) vs O(N^2))
// + the selected eid checks are precalculated to not slow this down
let markAllFiltered = function() {
  local selectedIds = selectedEntitiesSceneIds.get()
  local selectedIdsCount = getSelectedIdsCount(selectedIds)
  markScene(@(scene, _cur) matchSceneByFilters(scene, selectedIds, selectedIdsCount))
}

let markSceneNone = @() markScene(@(_scene, _cur) false)

// invert filtered, deselect unfiltered
let markScenesInvert = function() {
  local selectedIds = selectedEntitiesSceneIds.get()
  local selectedIdsCount = getSelectedIdsCount(selectedIds)
  markScene(@(scene, cur) matchSceneByFilters(scene, selectedIds, selectedIdsCount) ? !cur : false)
}

let toggleEditing = function() {
  editedStateScenes.mutate(function(value) {
    foreach (sceneId, edited in value) {
      if (markedStateScenes.get()?[sceneId]) {
        local loadType = getSceneIdLoadType(sceneId)
        local index = getSceneIdIndex(sceneId)
        if (entity_editor?.get_instance().isChildScene(loadType, index)) {
          entity_editor?.get_instance().setChildSceneEditable(loadType, index, !edited)
          value[sceneId] = !edited
        }
      }
    }
  })
}

function scrollScenesBySelection() {
  scrollHandler.scrollToChildren(function(desc) {
    return ("scene" in desc) && markedStateScenes.get()?[getSceneIdOf(desc.scene)]
  }, 2, false, true)
}

function scnTxt(count) { return count==1 ? "scene" : "scenes" }
function entTxt(count) { return count==1 ?  "entity" : "entities" }
function statusText(count, textFunc) { return format("%d %s", count, textFunc(count)) }

function statusLineScenes() {
  let sMrk = getNumMarkedScenes()
  let eMrk = numMarkedScenesEntityCount.get()
  let eRec = filteredScenesEntityCount.get()

  if (statusAnimTrigger.lastN != null && statusAnimTrigger.lastN != sMrk)
    anim_start(statusAnimTrigger)
  statusAnimTrigger.lastN = sMrk

  return {
    watch = [numMarkedScenesEntityCount, filteredScenesCount, filteredScenesEntityCount, markedStateScenes, selectedEntities, editedStateScenes]
    size = FLEX_H
    flow = FLOW_HORIZONTAL
    children = [
      {
        rendObj = ROBJ_TEXT
        size = FLEX_H
        text = format(" %s, with %s, marked", statusText(sMrk, scnTxt), statusText(eMrk, entTxt))
        animations = [
          { prop=AnimProp.color, from=colors.HighlightSuccess, duration=0.5, trigger=statusAnimTrigger }
        ]
      }
      {
        rendObj = ROBJ_TEXT
        halign = ALIGN_RIGHT
        size = FLEX_H
        text = format(" %s, with %s, listed", statusText(filteredScenesCount.get(), scnTxt), statusText(eRec, entTxt))
        color = Color(170,170,170)
      }
    ]
  }
}

let filter = nameFilter(filterString, {
  placeholder = "Filter by load-type/path/entities"

  function onChange(text) {
    filterString.set(text)
  }

  function onEscape() {
    set_kb_focus(null)
  }

  function onReturn() {
    set_kb_focus(null)
  }

  function onClear() {
    filterString.set("")
    set_kb_focus(null)
  }
})

function listSceneRow(scene, idx) {
  return watchElemState(function(sf) {
    let sceneId = getSceneIdOf(scene)
    let isMarked = markedStateScenes.get()?[sceneId]
    let textColor = isMarked ? colors.TextDefault : colors.TextDarker
    let color = isMarked ? colors.Active
    : sf & S_TOP_HOVER ? colors.GridRowHover
    : colors.GridBg[idx % colors.GridBg.len()]

    return {
      rendObj = ROBJ_SOLID
      size = FLEX_H
      color
      scene
      behavior = Behaviors.Button

      function onClick(evt) {
        if (evt.shiftKey) {
          local selCount = 0
          foreach (_k, v in markedStateScenes.get()) {
            if (v)
              ++selCount
          }
          if (selCount > 0) {
            local idx1 = -1
            local idx2 = -1
            foreach (i, filteredScene in filteredScenes.get()) {
              if (scene == filteredScene) {
                idx1 = i
                idx2 = i
              }
            }
            foreach (i, filteredScene in filteredScenes.get()) {
              if (markedStateScenes.get()?[getSceneIdOf(filteredScene)]) {
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
              markedStateScenes.mutate(function(value) {
                for (local i = idx1; i <= idx2; i++) {
                  let filteredScene = filteredScenes.get()[i]
                  value[getSceneIdOf(filteredScene)] <- !evt.ctrlKey
                }
              })
            }
          }
        }
        else if (evt.ctrlKey) {
          markedStateScenes.mutate(function(value) {
            value[sceneId] <- !value?[sceneId]
          })
        }
        else {
          local wasMarked = markedStateScenes.get()?[sceneId]
          markSceneNone()
          if (!wasMarked) {
            markedStateScenes.mutate(function(value) {
              value[sceneId] <- true
            })
          }
        }
      }

      children = {
        rendObj = ROBJ_TEXT
        text = sceneToText(scene)
        color = textColor
        margin = fsh(0.5)
      }
    }
  })
}

function listRowMoreLeft(num, idx) {
  return watchElemState(function(sf) {
    let color = (sf & S_TOP_HOVER) ? colors.GridRowHover : colors.GridBg[idx % colors.GridBg.len()]
    return {
      rendObj = ROBJ_SOLID
      size = FLEX_H
      color
      children = {
        rendObj = ROBJ_TEXT
        text = $"{num} more ..."
        color = colors.TextReadOnly
        margin = fsh(0.5)
      }
    }
  })
}


function initScenesList() {
  local scenes = entity_editor?.get_instance().getSceneImports() ?? []
  foreach (scene in scenes) {
    local sceneId = getSceneId(scene.loadType, scene.index)
    local isMarked = markedScenes.get()?[sceneId] ?? false
    markedStateScenes.get()[sceneId] <- isMarked
    editedStateScenes.get()[sceneId] <- false
  }
  allScenes.set(scenes)
  allSceneIndices.set(getSceneIndicies(scenes))
  markedStateScenes.trigger()
}

sceneSortState.subscribe_with_nasty_disregard_of_frp_update(function(v) {
  sceneSortFuncCache = v?.func
  selectedEntities.trigger()
  markedStateScenes.trigger()
  initScenesList()
})

de4workMode.subscribe(@(_) gui_scene.resetTimeout(0.1, initScenesList))

function initEntitiesList() {
  let entities = entity_editor?.get_instance().getEntities("") ?? []
  foreach (eid in entities) {
    let isSelected = selectedEntities.get()?[eid] ?? false
    selectionStateEntities.get()[eid] <- isSelected
  }
  allEntities.set(entities)
  selectionStateEntities.trigger()
}

function initLists() {
  initScenesList();
  initEntitiesList();
}

function sceneFilterCheckbox() {
  let group = ElemGroup()
  let stateFlags = Watched(0)
  let hoverFlag = Computed(@() stateFlags.get() & S_HOVER)

  function onClick() {
    filterScenesBySelectedEntities.set(!filterScenesBySelectedEntities.get())
    return
  }

  return function () {
    local mark = null
    if (filterScenesBySelectedEntities.get()) {
      mark = {
        rendObj = ROBJ_SOLID
        color = (hoverFlag.get() != 0) ? colors.Hover : colors.Interactive
        group
        size = [pw(50), ph(50)]
        hplace = ALIGN_CENTER
        vplace = ALIGN_CENTER
      }
    }

    return {
      size = FLEX_H
      flow = FLOW_HORIZONTAL
      halign = ALIGN_LEFT
      valign = ALIGN_CENTER

      watch = [filterScenesBySelectedEntities]

      children = [
        {
          size = [fontH(80), fontH(80)]
          rendObj = ROBJ_SOLID
          color = colors.ControlBg

          behavior = Behaviors.Button
          group

          children = mark

          onElemState = @(sf) stateFlags.set(sf)

          onClick
        }
        {
          rendObj = ROBJ_TEXT
          size = FLEX_H
          text = "Pre-filter based on selected entities"
          color = colors.TextDefault
          margin = fsh(0.5)
        }
      ]
    }
  }
}

function mkScenesList() {

  function listSceneContent() {
    const maxVisibleItems = 250
    local sRows = filteredScenes.get().slice(0, maxVisibleItems).map(@(scene, idx) listSceneRow(scene, idx))
    if (sRows.len() < filteredScenes.get().len())
      sRows.append(listRowMoreLeft(filteredScenes.get().len() - sRows.len(), sRows.len()))

    return {
      watch = [selectedEntities, markedStateScenes, filteredScenes, editedStateScenes]
      size = FLEX_H
      flow = FLOW_VERTICAL
      children = sRows
      behavior = Behaviors.Button
    }
  }

  let scrollListScenes = makeVertScroll(listSceneContent, {
    scrollHandler
    rootBase = {
      size = flex()
      function onAttach() {
        scrollScenesBySelection()
      }
    }
  })

  return  @() {
    flow = FLOW_VERTICAL
    gap = fsh(0.5)
    watch = [allScenes, filteredScenes, markedStateScenes, allEntities, filteredEntities, selectionStateEntities]
    size = flex()
    children = [
      {
        size = FLEX_H
        flow = FLOW_HORIZONTAL
        children = [
          mkSceneSortModeButton(sceneSortState)
          static { size = [sw(0.2), SIZE_TO_CONTENT] }
          filter
          static { size = [fsh(0.2), 0] }
        ]
      }
      {
        flow = FLOW_HORIZONTAL
        size = FLEX_H
        children = sceneFilterCheckbox()
      }
      {
        size = flex()
        children = scrollListScenes
      }
      statusLineScenes
      {
        flow = FLOW_HORIZONTAL
        size = FLEX_H
        halign = ALIGN_CENTER
        children = [
          textButton("All filtered", markAllFiltered)
          textButton("None", markSceneNone)
          textButton("Invert", markScenesInvert)
        ]
      }
      {
        flow = FLOW_HORIZONTAL
        size = FLEX_H
        halign = ALIGN_CENTER
        children = [
          textButton("Toggle editing", toggleEditing)
        ]
      }
    ]
  }
}

return {
  id = LoadedScenesWndId
  onAttach = initLists
  mkContent = mkScenesList
  saveState=true
}
