from "string" import format
from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let entity_editor = require_optional("entity_editor")
let { SceneOutlinerWndId, selectedEntities, markedScenes, de4workMode, allScenesWatcher, updateAllScenes,
  sceneIdMap, sceneListUpdateTrigger, addEntityCreatedCallback, addEntityRemovedCallback, entitiesListUpdateTrigger
  edObjectFlagsUpdateTrigger } = require("state.nut")
let { colors } = require("components/style.nut")
let textButton = require("components/textButton.nut")
let nameFilter = require("components/nameFilter.nut")
let mkCheckBox = require("components/mkCheckBox.nut")
let { makeVertScroll } = require("%daeditor/components/scrollbar.nut")
let textInput = require("%daeditor/components/textInput.nut")
let { getSceneLoadTypeText, getNumMarkedScenes, getScenePrettyName, canSceneBeModified } = require("%daeditor/daeditor_es.nut")
let { sortScenesByLoadType } = require("components/sceneSorting.nut")
let { addModalWindow, removeModalWindow } = require("%daeditor/components/modalWindows.nut")
let { Point3 } = require("dagor.math")
let { isStringFloat } = require("%sqstd/string.nut")
let { fileName } = require("%sqstd/path.nut")
let { addImportDialog } = require("%daeditor/addImportDialog.nut")
let { deferOnce } = require("dagor.workcycle")
let { setTooltip } = require("components/cursors.nut")
let ecs = require("%sqstd/ecs.nut")

const TREE_CONTROL_CONTROL_WIDTH = 20
const DROP_BORDER_SIZE = 0.2
const TREE_CONNECTIONS_COLOR = Color(255, 255, 255)
const MAX_FREE_ENTITIES = 100

let filterSelectedEntities = Watched(false)
let showEntities = mkWatched(persist, "showEntities", true)
let showImportScenes = mkWatched(persist, "showImportScenes", true)
let showClientScenes = mkWatched(persist, "showClientScenes", true)
let showCommonScenes = mkWatched(persist, "showCommonScenes", true)
let showOtherScenes = mkWatched(persist, "showOtherScenes", true)
let itemDragData = Watched(null)
let dragDestData = Watched(null)
let markedStateScenes = mkWatched(persist, "markedStateScenes", {})
let expandedStateScenes = mkWatched(persist, "expandedStateScenes", {})
let filterString = mkWatched(persist, "filterString", "")
let selectionStateEntities = mkWatched(persist, "selectionStateEntities", {})
let allEntities = mkWatched(persist, "allEntities", {})
let scrollHandler = ScrollHandler()

let statusAnimTrigger = { lastN = null }

let UNUSED = @(...) null

function sceneToText(scene) {
  local prettyName = getScenePrettyName(scene.id)
  local strippedPath = fileName(scene.path)
  local sceneName = prettyName.len() == 0 ? strippedPath : $"{prettyName} ({strippedPath})"

  return $"{scene.importDepth == 0 ? "*** " : ""}{sceneName}"
}

let removeSelectedByEditorTemplate = @(tname) tname.replace("+daeditor_selected+","+").replace("+daeditor_selected","").replace("daeditor_selected+","")

function entityToTxt(eid) {
  local tplName = ecs.g_entity_mgr.getEntityTemplateName(eid) ?? ""
  let name = removeSelectedByEditorTemplate(tplName)
  return $"{eid} - {name}"
}

let isFilteringEnabled = Computed(function (){
  return (filterSelectedEntities?.get() ?? false) || (filterString?.get() != null && filterString.get().len() != 0) || !showEntities.get()
})

let fakeScene = {
  loadType = 3
  entityCount = 0
  path = "Entities without scene"
  importDepth = 1
  parent = ecs.INVALID_SCENE_ID
  hasParent = false
  imports = 0
  id = ecs.INVALID_SCENE_ID
  hasChildren = false
}

let isFakeSceneHidden = Computed(function() {
  UNUSED(edObjectFlagsUpdateTrigger)

  let entities = allEntities?.get()[ecs.INVALID_SCENE_ID] ?? []
  foreach (id in entities) {
    if (!entity_editor?.get_instance().isEntityHidden(id)) {
      return false
    }
  }
  return true
})

let isFakeSceneLocked = Computed(function() {
  UNUSED(edObjectFlagsUpdateTrigger)

  let entities = allEntities?.get()[ecs.INVALID_SCENE_ID] ?? []
  foreach (id in entities) {
    if (!entity_editor?.get_instance().isEntityLocked(id)) {
      return false
    }
  }

  return true
})

function getScene(id) {
  return id != ecs.INVALID_SCENE_ID ? sceneIdMap?.get()[id] : fakeScene
}

function filterItem(item) {
  if (item?.entity != null && !showEntities.get()) {
    return false
  }

  if (filterSelectedEntities?.get()) {
    if (item?.entity == null) {
      return false
    }

    if (!selectedEntities?.get()[item.entity.id]) {
      return false
    }
  }

  let filterStr = filterString.get()
  if (filterStr != null && filterStr.len() != 0) {
    let itemStr = item?.scene != null ? sceneToText(item.scene) : entityToTxt(item.entity.id)

    return itemStr.tolower().contains(filterStr.tolower())
  }
  return true
}

function createFakeSceneItems(order, expandedStateCb) {
  local sceneItem = {}
  let entities = allEntities?.get()[ecs.INVALID_SCENE_ID] ?? []

  sceneItem.scene <- fakeScene
  sceneItem.order <- order
  sceneItem.scene.entityCount = entities.len()
  sceneItem.scene.hasChildren = false
  sceneItem.depth <- 0

  let fakeItems = []
  local isAnythingFilteredIn = filterItem(sceneItem)

  foreach (eid in entities) {
    if (fakeItems.len() >= MAX_FREE_ENTITIES) {
      local textItem = {}
      textItem.text <- "More entities ...";
      textItem.parentSceneId <- ecs.INVALID_SCENE_ID
      textItem.depth <- 1

      fakeItems.append(textItem)
      break
    }

    local entityItem = {}
    entityItem.entity <- {}
    entityItem.entity.id <- eid
    entityItem.entity.parentSceneId <- ecs.INVALID_SCENE_ID
    entityItem.order <- fakeItems.len()
    entityItem.depth <- 1

    let isExpanded = expandedStateCb(fakeScene)

    if (filterItem(entityItem)) {
      isAnythingFilteredIn = true
      sceneItem.scene.hasChildren = true
      if (isExpanded) {
        fakeItems.append(entityItem)
      }
    }
  }

  if (isAnythingFilteredIn) {
    fakeItems.insert(0, sceneItem)
  }

  return fakeItems
}

function gatherSceneItems(scene, order, isParentExpanded, depth, items, expandedStateCb) {
  if (scene.id != fakeScene.id) {
    if ((scene.loadType == 1 && !showCommonScenes.get()) ||
      (scene.loadType == 2 && !showClientScenes.get()) ||
      (scene.loadType == 3 && !showImportScenes.get())) {
      return false;
    }
  }
  else if (!showOtherScenes.get()) {
    return false
  }

  if (scene.id == fakeScene.id) {
    items.extend(createFakeSceneItems(order, expandedStateCb))
    return
  }

  local sceneItem = {}
  sceneItem.scene <- scene
  sceneItem.order <- order
  sceneItem.scene.hasChildren <- false
  sceneItem.depth <- depth

  local insertionPos = items.len()
  local isAnythingFilteredIn = filterItem(sceneItem)
  let isExpanded = isParentExpanded && expandedStateCb(scene)

  local entries = entity_editor?.get_instance().getSceneOrderedEntries(scene.id) ?? []
  local i = 0
  foreach (entry in entries) {
    if (entry.isEntity) {
      local entityItem = {}
      entityItem.entity <- {}
      entityItem.entity.id <- entry.eid
      entityItem.entity.parentSceneId <- scene.id
      entityItem.order <- i
      entityItem.depth <- depth + 1

      if (filterItem(entityItem)) {
        isAnythingFilteredIn = true
        sceneItem.scene.hasChildren = true

        if (isExpanded) {
          items.append(entityItem)
          ++i
        }
      }
    }
    else {
      local filteredIn = gatherSceneItems(getScene(entry.sid), i, isExpanded, depth + 1, items, expandedStateCb)
      if (filteredIn) {
        ++i
        isAnythingFilteredIn = true
        sceneItem.scene.hasChildren = true
      }
    }
  }

  if (isAnythingFilteredIn && isParentExpanded) {
    items.insert(insertionPos, sceneItem)
  }

  return isAnythingFilteredIn
}

function getEntityCount(scene) {
  if (scene.id == fakeScene.id) {
    return allEntities.get()?[scene.id].len()
  }
  else {
    return entity_editor?.get_instance().getSceneEntityCount(scene.id)
  }
}

let filteredItems = Computed(function() {
  UNUSED(showEntities)
  UNUSED(showClientScenes)
  UNUSED(showCommonScenes)
  UNUSED(showImportScenes)
  UNUSED(showOtherScenes)
  UNUSED(filterSelectedEntities)
  UNUSED(selectedEntities)
  UNUSED(filterString)
  UNUSED(allEntities)
  UNUSED(expandedStateScenes)

  local scenes = allScenesWatcher.get()?.map(function (item, ind) {
    item.index <- ind
    return item
    }) ?? [] // get a copy to avoid sorting allScenes
  local items = []

  local rootScenes = scenes.filter(function (scene) {
    return !scene.hasParent
  })

  rootScenes.sort(sortScenesByLoadType)

  local i = 0
  foreach (scene in rootScenes) {
    if (gatherSceneItems(scene, i, true, 0, items, @(val) expandedStateScenes?.get()?[val.id] ?? false)) {
      ++i
    }
  }

  gatherSceneItems(fakeScene, i, true, 0, items, @(val) expandedStateScenes?.get()?[val.id] ?? false)

  return items
})

let filteredScenesCount = Computed(@() filteredItems.get().filter(@(item) item?.scene != null && item.scene.id != ecs.INVALID_SCENE_ID).len())

let filteredScenesEntityCount = Computed(function() {
  local eCount = 0
  foreach (item in filteredItems.get()) {
    if (item?.scene != null) {
      eCount += getEntityCount(item.scene)
    }
  }
  return eCount
})

function persistMarkedScenes(v) {
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
})

let numMarkedScenesEntityCount = Computed(function() {
  local nSaved = 0
  local nGenerated = 0
  foreach (item in filteredItems.get()) {
    if (item?.scene) {
      local scene = item.scene
      if (markedStateScenes.get()?[scene.id]) {
        nSaved += getEntityCount(scene)
      }
    }
  }
  return nSaved + nGenerated
})

function clearSelection() {
  markedStateScenes.mutate(function(value) {
    foreach (k, _v in value)
      value[k] = false
  })

  selectionStateEntities.mutate(function(value) {
    foreach (k, _v in value)
      value[k] = false
  })
}

function scrollScenesBySelection() {
  scrollHandler.scrollToChildren(function(desc) {
    return ("item" in desc) && (markedStateScenes.get()?[desc.item?.scene.id] || selectionStateEntities.get()?[desc.item?.entity.id])
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
    watch = [numMarkedScenesEntityCount, filteredScenesCount, filteredScenesEntityCount, markedStateScenes, selectedEntities]
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

function markAllObjects() {
  clearSelection()
  foreach (item in filteredItems.get()) {
    if (item?.scene != null) {
      markedStateScenes.get()[item.scene.id] <- true
    }
    else if (item?.entity != null) {
      selectionStateEntities.get()[item.entity.id] <- true
    }
  }

  markedStateScenes.trigger()
  selectionStateEntities.trigger()
}

function markObject(object, flag = true) {
  if (object?.scene != null) {
    markedStateScenes.mutate(function(value) {
      value[object.scene.id] <- flag
    })
  }
  else if (object?.entity != null) {
    selectionStateEntities.mutate(function(value) {
      value[object.entity.id] <- flag
    })
  }
}

function getIcon(name) {
  return $"!%daeditor/images/{name}"
}

function mkIconButton(icon, onClick = null, visible = true, parentHovered = false, opacity = 0.9, tooltip = null) {
  return watchElemState( @(sf) {
    rendObj = ROBJ_BOX
    behavior = (onClick != null && visible) ? Behaviors.Button : Behaviors.TrackMouse
    onClick = onClick
    size = SIZE_TO_CONTENT
    onHover = @(on) setTooltip(on ? tooltip : null)
    children = @() {
      rendObj = ROBJ_IMAGE
      image = !visible ? null : ((sf & S_HOVER) || parentHovered ? Picture(icon) : null)
      size = [hdpx(20), hdpx(20)]
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      color = onClick != null && (sf & S_HOVER) ? Color(66, 176, 255) : Color(255, 255, 255, 255)
      opacity = opacity
    }
  })
}

function gatherItemsAndModifySelection(op) {
  let additionalSelection = []
  foreach (item in filteredItems.get()) {
    if (item.depth == 0) {
      let sceneCopy = clone item.scene
      gatherSceneItems(sceneCopy, 0, true, 0, additionalSelection, @(_scene) true)
    }
  }

  foreach (selected in additionalSelection) {
    markObject(selected, op)
  }
}

let hasFilteredItems = Computed(@() isFilteringEnabled.get() && filteredItems.get().len() != 0)

let filter = nameFilter(filterString, {
  placeholder = "Filter and search"
  onChange = @(text) filterString.set(text)
  onEscape = @() set_kb_focus(null)
  onReturn = @() set_kb_focus(null)
  onClear = function() {
    filterString.set("")
    set_kb_focus(null)
  }
  additionalChildren = [
    @() {
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      size = SIZE_TO_CONTENT
      pad = fsh(0.5)
      watch = [isFilteringEnabled, filteredItems]
      children = mkIconButton(getIcon("filter_selected"),
        hasFilteredItems.get() ? @() gatherItemsAndModifySelection(true) : null,
        true, true, hasFilteredItems.get() ? 1.0 : 0.5, hasFilteredItems.get()
          ? "Select all currently filtered items"
          : "There are no matches for selection")
    }
    @() {
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      size = SIZE_TO_CONTENT
      pad = fsh(0.5)
      watch = [isFilteringEnabled, filteredItems]
      children = mkIconButton(getIcon("filter_unselected"),
        hasFilteredItems.get() ? @() gatherItemsAndModifySelection(false) : null,
        true, true, hasFilteredItems.get() ? 0.9 : 0.5, hasFilteredItems.get()
          ? "Unselect all currently filtered items"
          : "There are no matches for unselection")
    }
  ]
})

function getSelectedScenesIndicies(scenes) {
  return scenes.get()?.filter(@(marked, _sceneId) marked).keys()
}

function getAllSelectedItems() {
  let selection = []
  selection.extend(markedStateScenes?.get().filter(@(marked, _sceneId) marked).keys().map(function (value) {
    let item = {}
    item.id <- value
    item.isEntity <- false
    item.loadType <- sceneIdMap?.get()[value].loadType ?? 0
    return item
  }))

  selection.extend(selectionStateEntities?.get().filter(@(selected, _eid) selected).keys().map(function (value) {
    let item = {}
    item.id <- value
    item.isEntity <- true
    return item
  }))

  return selection
}

function selectObjects() {
  entity_editor?.get_instance().selectObjects(getAllSelectedItems())
}

function initScenesList() {
  foreach (scene in allScenesWatcher?.get() ?? []) {
    local isMarked = markedScenes.get()?[scene.id] ?? false
    local isExpanded = expandedStateScenes.get()?[scene.id] ?? false

    markedStateScenes.get()[scene.id] <- isMarked
    expandedStateScenes.get()[scene.id] <- isExpanded
  }

  local wasFakeSceneExpanded = expandedStateScenes.get()?[ecs.INVALID_SCENE_ID] ?? false
  markedStateScenes.modify(@(v) v.filter(@(_, key) key in sceneIdMap.get()))
  expandedStateScenes.modify(@(v) v.filter(@(_, key) key in sceneIdMap.get()))

  expandedStateScenes.get()[ecs.INVALID_SCENE_ID] <- wasFakeSceneExpanded

  markedStateScenes.trigger()
  expandedStateScenes.trigger()
}

function initEntitiesList() {
  let entities = entity_editor?.get_instance().getEntities("") ?? []
  foreach (eid in entities) {
    let isSelected = selectionStateEntities.get()?[eid] ?? false
    selectionStateEntities.get()[eid] <- isSelected
  }
  allEntities.set({})
  allEntities.mutate(function(value) {
    foreach (eid in entities) {
      local sceneId = entity_editor?.get_instance().getEntityRecordSceneId(eid) ?? ecs.INVALID_SCENE_ID

      let entries = value?[sceneId]

      if (entries == null) {
        value[sceneId] <- [eid]
      }
      else {
        value[sceneId].append(eid)
      }
    }
  })
  selectionStateEntities.trigger()
}

function initLists() {
  initScenesList();
  initEntitiesList();
}

sceneListUpdateTrigger.subscribe_with_nasty_disregard_of_frp_update(@(_v) deferOnce(updateAllScenes))
allScenesWatcher.subscribe_with_nasty_disregard_of_frp_update(@(_v) deferOnce(initLists))

entitiesListUpdateTrigger.subscribe_with_nasty_disregard_of_frp_update(@(_v) deferOnce(initEntitiesList))
addEntityCreatedCallback(@(_eid) deferOnce(initEntitiesList))
addEntityRemovedCallback(@(_eid) deferOnce(initEntitiesList))

const setSceneNameUID = "set_scene_name_modal_window"

function setScenePrettyName(sceneId) {
  let sceneName = Watched(entity_editor?.get_instance().getScenePrettyName(sceneId))
  let close = @() removeModalWindow(setSceneNameUID)

  function doSetSceneName() {
    local confirmationUID = "clear_scene_name_modal_window"

    function applySceneName() {
        entity_editor?.get_instance().setScenePrettyName(sceneId, sceneName.get())
        updateAllScenes()
    }

    if (sceneName.get().len() == 0) {
      addModalWindow({
        key = confirmationUID
        children = vflow(
          Button
          Gap(fsh(0.5))
          RendObj(ROBJ_SOLID)
          Padding(hdpx(10))
          Colr(20,20,20,255)
          Size(hdpx(330), SIZE_TO_CONTENT)
          vflow(
            HCenter
            txt("Are you sure you want to clear scene name?"))
          hflow(
            HCenter
            textButton("Cancel", @() removeModalWindow(confirmationUID), {hotkeys=[["Esc"]]})
            textButton("Ok", function () {
              applySceneName()
              removeModalWindow(confirmationUID)
            }, {hotkeys=[["Enter"]]})
          )
        )
      })
    }
    else {
      applySceneName()
    }
  }

  addModalWindow({
    key = setSceneNameUID
    children = vflow(
      Button
      Gap(fsh(0.5))
      RendObj(ROBJ_SOLID)
      Padding(hdpx(10))
      Colr(20,20,20,255)
      vflow(Size(flex(), SIZE_TO_CONTENT), txt("Enter scene name:"))
      textInput(sceneName)
      hflow(
        textButton("Cancel", close, {hotkeys=[["Esc"]]})
          @() {
            children = textButton("Apply", function() {
              doSetSceneName()
              close()
            })
          }
      )
    )
  })
}

function isItemSelected(item) {
  if (item?.text != null) {
    return false
  }

  return item?.scene != null ? markedStateScenes?.get()[item.scene.id] : selectionStateEntities?.get()[item.entity.id]
}

function getItemParentScene(item) {
  return item?.scene != null ? item.scene.parent : (item?.entity != null ? item.entity.parentSceneId : item.parentSceneId)
}

function getTreeControl(item, ind) {
  function getCurrentScene() {
    if (item?.scene != null) {
      return getScene(item.scene.id)
    }
    else if (item?.entity != null) {
      return getScene(item.entity.parentSceneId)
    }
    else {
      return item.parentSceneId
    }
  }

  let isScene = item?.scene != null
  local offset = isScene ? 0 : 1;
  local currentScene = getCurrentScene()

  while (currentScene?.hasParent) {
    ++offset
    currentScene = sceneIdMap.get()?[currentScene.parent]
  }

  let objs = []
  local currentOffset = 0
  while (currentOffset < offset - 1) {
    objs.append(
      {
        size = [hdpx(TREE_CONTROL_CONTROL_WIDTH), flex()]
        valign = ALIGN_CENTER
        halign = ALIGN_CENTER
        children = {
          rendObj = ROBJ_SOLID
          size = [1, flex()]
          color = TREE_CONNECTIONS_COLOR
        }
      }
    )

    ++currentOffset
  }

  if ((isScene && !(item.scene?.importDepth == 0 || (item.scene?.importDepth != 0 && !entity_editor?.get_instance().isChildScene(item.scene.id))))
    || !isScene) {
    if (ind + 1 == filteredItems.get().len() || item.depth > filteredItems.get()[ind + 1].depth) {
      objs.append(
        {
          size = [hdpx(TREE_CONTROL_CONTROL_WIDTH), flex()]
          valign = ALIGN_CENTER
          halign = ALIGN_RIGHT
          flow = FLOW_HORIZONTAL
          children = [
            {
              size = [1, flex()]
              flow = FLOW_VERTICAL
              children = [
                {
                  rendObj = ROBJ_SOLID
                  valign = ALIGN_TOP
                  size = [1, flex()]
                  color = TREE_CONNECTIONS_COLOR
                }
                {
                  valign = ALIGN_TOP
                  size = [1, flex()]
                }
              ]
            }
            {
              halign = ALIGN_RIGHT
              rendObj = ROBJ_SOLID
              size = [hdpx(TREE_CONTROL_CONTROL_WIDTH) / 2, 1]
              color = TREE_CONNECTIONS_COLOR
            }
          ]
        }
      )
    }
    else {
      objs.append(
        {
          size = [hdpx(TREE_CONTROL_CONTROL_WIDTH), flex()]
          valign = ALIGN_CENTER
          halign = ALIGN_RIGHT
          flow = FLOW_HORIZONTAL
          children = [
            {
              rendObj = ROBJ_SOLID
              size = [1, flex()]
              color = TREE_CONNECTIONS_COLOR
            }
            {
              halign = ALIGN_RIGHT
              rendObj = ROBJ_SOLID
              size = [hdpx(TREE_CONTROL_CONTROL_WIDTH) / 2, 1]
              color = TREE_CONNECTIONS_COLOR
            }
          ]
        }
      )
    }
  }

  if (isScene && item.scene.hasChildren) {
    objs.append({
      size = [hdpx(TREE_CONTROL_CONTROL_WIDTH), flex()]
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      children = {
        rendObj = ROBJ_SOLID
        behavior = Behaviors.Button
        color = Color(224, 224, 224)
        size = const [ hdpx(14), hdpx(14) ]
        halign = ALIGN_CENTER
        valign = ALIGN_CENTER
        children = {
          halign = ALIGN_CENTER
          valign = ALIGN_CENTER
          rendObj = ROBJ_TEXT
          text = expandedStateScenes?.get()[item.scene.id] ? "-" : "+"
          color = Color(0, 0, 0)
        }

        onClick = function () {
          expandedStateScenes.mutate(function(value) {
            value[item.scene.id] <- !value?[item.scene.id]
          })
        }
      }
    })
  }

  return objs
}

function getRowText(item) {
  if (item?.scene != null) {
    return sceneToText(item.scene)
  }
  else if (item?.entity != null) {
    return entityToTxt(item.entity.id)
  }
  else {
    return item.text
  }
}

function mkTag(icon, bgColor, color, text, maxTextString, tooltip = null) {
  return {
    rendObj = ROBJ_BOX
    fillColor = bgColor
    borderRadius = hdpx(45)
    flow = FLOW_HORIZONTAL
    valign = ALIGN_CENTER
    padding = [0, fsh(0.5), 0, fsh(0.5)]
    gap = fsh(0.25)
    behavior = Behaviors.TrackMouse
    onHover = @(on) setTooltip(on ? tooltip : null)
    children = [
      {
        halign = icon != null ? ALIGN_RIGHT : ALIGN_CENTER
        rendObj = ROBJ_TEXT
        text = text
        color = color
        size = [calc_str_box({rendObj = ROBJ_TEXT, text = maxTextString})[0], SIZE_TO_CONTENT]
      }
      icon != null
        ? {
          halign = ALIGN_RIGHT
          rendObj = ROBJ_IMAGE
          image = Picture(icon)
          size = [hdpx(20), hdpx(20)]
          color = color
        }
        : null
    ]
  }
}

function mkDataRow(item, textColor, hovered) {
  let isRealScene = item?.scene != null && item.scene.id != fakeScene.id
  let isEntity = item?.entity != null
  let isScene = item?.scene != null
  let isOtherItem = item?.text != null

  function getTagWidth(hasIcon, maxTextString) {
    return calc_comp_size(mkTag(hasIcon ? getIcon("select_none") : null, Color(0, 0, 0), Color(0, 0, 0), "", maxTextString))[0]
  }

  function canBeModified() {
    if (isScene) {
      return canSceneBeModified(item.scene)
    }
    else if (isEntity) {
      return item.entity.parentSceneId == fakeScene.id || canSceneBeModified(sceneIdMap.get()[item.entity.parentSceneId])
    }

    return false
  }

  function canBeRemoved() {
    return (item?.scene.importDepth ?? 1) != 0 && canBeModified()
  }

  function isTransformable() {
    if (isScene) {
      if (!canBeModified() || !isRealScene) {
        return false
      }

      return entity_editor?.get_instance().isSceneInTransformableHierarchy(item.scene.id)
    }
    else if (isEntity) {
      return entity_editor?.get_instance().isEntityTransformable(item.entity.id)
    }

    return false
  }

  function getTransformableIconOpacity() {
    if (isScene) {
      if (!canBeModified() || !isRealScene) {
        return 0.5
      }

      return isTransformable() ? 0.9 : 0.75
    }
    else if (isEntity) {
      return isTransformable() ? 0.9 : 0.5
    }
    else {
      return 0
    }
  }

  function isHidden() {
    if (isScene) {
      if (item.scene.id == fakeScene.id) {
        return isFakeSceneHidden.get()
      }
      else {
        return entity_editor?.get_instance().isSceneHidden(item.scene.id)
      }
    }
    else if (isEntity) {
      return entity_editor?.get_instance().isEntityHidden(item.entity.id)
    }
    else {
      return true
    }
  }

  function isLocked() {
    if (isScene) {
      if (item.scene.id == fakeScene.id) {
       return isFakeSceneLocked.get()
      }
      else {
        return entity_editor?.get_instance().isSceneLocked(item.scene.id)
      }
    }
    else if (isEntity) {
      return entity_editor?.get_instance().isEntityLocked(item.entity.id)
    }
    else {
      return true
    }
  }

  function itemToObjectData() {
    let data = {}
    data.isEntity <- item?.entity != null
    data.id <- item?.entity != null ? item.entity.id : item.scene.id
    if (item?.scene != null) {
      data.loadType <- item.scene.loadType
    }

    return data
  }

  function getSceneTypeTagText(scene) {
    return scene.id != fakeScene.id ? getSceneLoadTypeText(scene) : "OTHER"
  }

  return {
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_HORIZONTAL
    gap = fsh(0.5)
    children = [
      isScene
        ? {
          rendObj = ROBJ_TEXT
          text = item.scene.id != fakeScene.id ? item.scene.id : ""
          color = textColor
          halign = ALIGN_LEFT
          valign = ALIGN_CENTER
          size = [calc_str_box({rendObj = ROBJ_TEXT, text = "888"})[0], flex()]
        }
        : null
      {
        rendObj = ROBJ_TEXT
        text = getRowText(item)
        size = [flex(), SIZE_TO_CONTENT]
        color = textColor
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
      }
      {
        halign = ALIGN_RIGHT
        valign = ALIGN_CENTER
        children = mkIconButton(
          getIcon("rename"),
          isRealScene && canSceneBeModified(item.scene) ? @() setScenePrettyName(item.scene.id) : null,
          isRealScene && canSceneBeModified(item.scene)
          hovered, 0.9, "Rename alias")
      }
      isScene
        ? mkTag(null, Color(67, 67, 67), Color(154, 154, 154),
          getSceneTypeTagText(item.scene), "COMMON", $"{getSceneTypeTagText(item.scene)} scene")
        : { rendObj = ROBJ_BOX, size = [ getTagWidth(false, "COMMON"), flex() ] }
      isScene
        ? mkTag(getIcon("select_none"), Color(75, 51, 49), Color(174, 143, 109), getEntityCount(item.scene), "88888",
          $"{getEntityCount(item.scene)} entities")
        : { rendObj = ROBJ_BOX, size = [ getTagWidth(true, "88888"), flex() ] }
      {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        children = mkIconButton(getIcon("move"), null, true, true, getTransformableIconOpacity(),
          isTransformable() ? "Transformable" : "Not transformable")
      }
      {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        children = mkIconButton(getIcon("zoom_and_center"), function() {
          clearSelection()
          markObject(item)
          selectObjects()
          entity_editor?.get_instance().zoomAndCenter()
        }, !isOtherItem, hovered, 0.9, "Zoom in at center in the viewport")
      }
      {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        children = mkIconButton(getIcon("layer_entity"), function() {
          local wasSelected = isItemSelected(item)
          if (isScene) {
            let newSelectionItems = []
            gatherSceneItems(item.scene, 0, true, 0, newSelectionItems, @(_scene) true)
            foreach (selected in newSelectionItems) {
              markObject(selected, !wasSelected)
            }
          }
          else if (isEntity) {
            markObject(item, !wasSelected)
          }
        }, !isOtherItem, hovered, 0.9, Computed(function() {
          UNUSED(markedStateScenes)
          UNUSED(selectionStateEntities)
          return isItemSelected(item)
            ? "Selected\nClick to unselect this item and all nested items in the viewport"
            : "Not selected\nClick to select this item and all nested items in the viewport"
        }))
      }
      @() {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        watch = edObjectFlagsUpdateTrigger
        children = mkIconButton(isHidden() ? getIcon("eye_hide") : getIcon("eye_show"), function() {
          if (isHidden()) {
            entity_editor?.get_instance().unhideObjects([itemToObjectData()])
          }
          else {
            entity_editor?.get_instance().hideObjects([itemToObjectData()])
          }
        }, !isOtherItem, hovered || isHidden(), 0.9, Computed(function() {
          UNUSED(edObjectFlagsUpdateTrigger)
          return isHidden()
            ? "Hidden\nClick to show debug visualization in the viewport"
            : "Visible\nClick to hide debug visualization in the viewport"
        }))
      }
      @() {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        watch = edObjectFlagsUpdateTrigger
        children = mkIconButton(isLocked() ? getIcon("lock_close") : getIcon("lock_open"), function() {
          if (isLocked()) {
            entity_editor?.get_instance().unlockObjects([itemToObjectData()])
          }
          else {
            entity_editor?.get_instance().lockObjects([itemToObjectData()])
          }
        },
        !isOtherItem,
        hovered || isLocked(), 0.9, Computed(function() {
          UNUSED(edObjectFlagsUpdateTrigger)
          return isLocked()
            ? "Locked\nClick to unlock editing"
            : "Unlocked\nClick to lock editing"
        }))
      }
      {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        children = mkIconButton(getIcon("delete"), function() {
          entity_editor?.get_instance().removeObjects([itemToObjectData()])
        }, !isOtherItem, hovered, canBeRemoved() ? 0.9 : 0.5, canBeRemoved()
          ? "Remove item"
          : "This item cannot be removed")
      }
    ]
  }
}

let sceneRowItemStateWatchers = {}
markedStateScenes.subscribe_with_nasty_disregard_of_frp_update(function(v) {
  foreach (item, stateWatcher in sceneRowItemStateWatchers) {
    if (item?.scene != null) {
      let state = stateWatcher.get()
      if (state.isMarked != v?[item.scene.id]) {
        state.isMarked = v?[item.scene.id] ?? false
        stateWatcher.set(state)
        stateWatcher.trigger()
      }
    }
  }
})

selectionStateEntities.subscribe_with_nasty_disregard_of_frp_update(function(v) {
  foreach (item, stateWatcher in sceneRowItemStateWatchers) {
    if (item?.entity != null) {
      let state = stateWatcher.get()
      if (state.isMarked != v?[item.entity.id]) {
        state.isMarked = v[item.entity.id]
        stateWatcher.set(state)
        stateWatcher.trigger()
      }
    }
  }
})

dragDestData.subscribe_with_nasty_disregard_of_frp_update(function(v) {
  if (v == null) {
    foreach (_item, stateWatcher in sceneRowItemStateWatchers) {
      let state = stateWatcher.get()
      if (state.dropPosition != null) {
        state.dropPosition = null
        stateWatcher.trigger()
      }
    }
  }
  else {
    function getDropPosForItem(item, itemIndex) {
      if (v.dropPosition == null) {
        return item == v.item ? 0 : null
      }
      else {
        local destPos = v.dropPosition > 0 ? 0 : -1
        if (itemIndex - 1 == v.index + destPos) {
          return -1
        }
        else if (itemIndex == v.index + destPos) {
          return 1
        }
      }

      return null
    }

    foreach (item, stateWatcher in sceneRowItemStateWatchers) {
      let state = stateWatcher.get()
      let thisDropPosition = getDropPosForItem(item, state.index)
      if (state.dropPosition != thisDropPosition) {
        state.dropPosition = thisDropPosition
        stateWatcher.trigger()
      }
    }
  }
})

function updateDropPosition(newPosition) {
  let dragDest = dragDestData.get()
  if (dragDest != null && dragDest.dropPosition != newPosition) {
    dragDest.dropPosition = newPosition
    dragDestData.set(dragDest)
    dragDestData.trigger()
  }
}

function listSceneRow(item, idx) {
  let stateData = { index = idx, isMarked = isItemSelected(item), dropPosition = null }
  let stateWatcher = Watched(stateData)
  sceneRowItemStateWatchers[item] <- stateWatcher

  return watchElemState(function(sf) {
    let isScene = item?.scene != null

    let isMarked = Computed(@() stateWatcher.get().isMarked)
    let textColor = Computed(@() isMarked.get() ? colors.TextDefault : (isScene ? Color(50, 166, 168) : colors.TextDarker))
    let rowColor = Computed(function () {
      return isMarked.get() ? colors.Active
        : sf & S_TOP_HOVER ? colors.GridRowHover
        : colors.GridBg[idx % colors.GridBg.len()]
    })

    let canBeDropped = Computed(function () {
      if (itemDragData.get() == null || dragDestData.get() == null) {
        return false
      }

      let dragDest = dragDestData.get()
      if (dragDest.item?.entity != null && dragDest.dropPosition == null) {
        return false
      }

      if (dragDest.dropPosition != null && isFilteringEnabled.get()) {
        return false
      }

      let dragDestScene = dragDest.item?.scene != null
        ? dragDest.item.scene
        : getScene(entity_editor?.get_instance().getEntityRecordSceneId(dragDest.item.entity.id))
      if (dragDestScene == null || (!canSceneBeModified(dragDestScene) && dragDestScene.id != ecs.INVALID_SCENE_ID)) {
        return false
      }

      function isInHierarchy(sceneId) {
        local destScene = dragDest.item?.scene != null ? dragDest.item.scene : sceneIdMap?.get()[getItemParentScene(dragDest.item)]
        while (destScene != null) {
          if (destScene.id == sceneId) {
            return true
          }

          destScene = sceneIdMap?.get()[destScene.parent]
        }

        return false
      }

      function notImport(draggedItem) {
        local scene = sceneIdMap?.get()[draggedItem.isEntity ? entity_editor?.get_instance().getEntityRecordSceneId(draggedItem.id) : draggedItem.id]
        return !canSceneBeModified(scene)
      }

      foreach (draggedItem in itemDragData.get()) {
        if (!draggedItem.isEntity) {
          if (isInHierarchy(draggedItem.id) || draggedItem.id == ecs.INVALID_SCENE_ID || dragDestScene.id == ecs.INVALID_SCENE_ID) {
            return false
          }
        }

        if (notImport(draggedItem)) {
          return false
        }
      }

      return true
    })

    let dragColor = Computed(function () {
      return (sf & S_DRAG) ? Color(255,255,0) : (!canBeDropped.get() ? Color(255,0,0) : Color(255, 255, 255))
    })

    function getSeparatorColor(state, position) {
      if (state.dropPosition != null && state.dropPosition != 0 && state.dropPosition == position) {
        if (canBeDropped.get()) {
          return Color(255, 255, 255)
        }
        else {
          return Color(255, 0, 0)
        }
      }

      return Color(0, 0, 0)
    }

    return {
      rendObj = ROBJ_SOLID
      size = FLEX_H
      color = rowColor.get()
      item
      watch = [stateWatcher, expandedStateScenes]
      behavior = item?.text == null ? [Behaviors.TrackMouse, Behaviors.DragAndDrop] : []
      flow = FLOW_HORIZONTAL
      eventPassThrough = true
      dropData = item

      canDrop = function(_data) {
        dragDestData.set({item, index = idx, dropPosition = dragDestData?.get().dropPosition })
        return canBeDropped.get()
      }

      onDrop = function(_data) {
        if (dragDestData.get() == null || itemDragData.get() == null) {
          return
        }

        let dragDest = dragDestData.get()
        if (dragDest.dropPosition == null) {
          if (dragDest.item?.entity != null) {
            return
          }

          entity_editor?.get_instance().setSceneNewParent(dragDest.item.scene.id, itemDragData.get())
        }
        else {
          entity_editor?.get_instance().setSceneNewParentAndOrder(getItemParentScene(dragDest.item),
            dragDest.dropPosition < 0 ? dragDest.item.order : dragDest.item.order + 1, itemDragData.get())
        }
      }

      onMouseMove = function (event) {
        if (sf & S_DRAG) {
          dragDestData.set(null)
        }

        if (dragDestData.get()) {
          let rect = event.targetRect
          let elemH = rect.b - rect.t
          let borderSize = elemH * DROP_BORDER_SIZE
          let relY = (event.screenY - rect.t)

          if (relY < borderSize) {
            updateDropPosition(-1)
          }
          else if (relY > elemH - borderSize) {
            updateDropPosition(1)
          }
          else {
            updateDropPosition(null)
          }
        }
      }

      onDragMode = function(on, _val) {
        let selectedIds = getAllSelectedItems()

        if (isMarked.get()) {
          itemDragData.set(on ? selectedIds : null)
        }
        else {
          let dragItem = {}
          dragItem.id <- isScene ? item.scene.id : item.entity.id
          dragItem.isEntity <- !isScene
          if (isScene) {
            dragItem.loadType <- item.scene.loadType ?? 0
          }
          itemDragData.set(on ? [dragItem] : null)
        }

        if (!on) {
          dragDestData.set(null)
        }
      }

      onDoubleClick = function(_evt) {
        clearSelection()
        if (isScene) {
          markedStateScenes.mutate(function(value) {
            value[item.scene.id] <- true
          })
        }
        else {
          selectionStateEntities.mutate(function(value) {
            value[item.entity.id] <- true
          })
        }
        selectObjects()
      }

      onClick = function(evt) {
        if (evt.shiftKey) {
          local selCount = 0
          foreach (_k, v in markedStateScenes.get()) {
            if (v)
              ++selCount
          }
          foreach (_k, v in selectionStateEntities.get()) {
            if (v)
              ++selCount
          }
          if (selCount > 0) {
            local idx1 = -1
            local idx2 = -1
            foreach (i, filteredItem in filteredItems.get()) {
              if (item == filteredItem) {
                idx1 = i
                idx2 = i
              }
            }
            foreach (i, filteredItem in filteredItems.get()) {
              if ((filteredItem?.scene != null && markedStateScenes.get()?[filteredItem.scene.id])
                || (filteredItem?.entity != null && selectionStateEntities.get()?[filteredItem.entity.id])) {
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
                  let filteredItem = filteredItems.get()[i]
                  if (filteredItem?.scene != null) {
                    value[filteredItem.scene.id] <- !evt.ctrlKey
                  }
                }
              })
              selectionStateEntities.mutate(function(value) {
                for (local i = idx1; i <= idx2; i++) {
                  let filteredItem = filteredItems.get()[i]
                  if (filteredItem?.entity != null) {
                    value[filteredItem.entity.id] <- !evt.ctrlKey
                  }
                }
              })
            }
          }
        }
        else if (evt.ctrlKey) {
          if (isScene) {
            markedStateScenes.mutate(function(value) {
              value[item.scene.id] <- !value?[item.scene.id]
            })
          }
          else {
            selectionStateEntities.mutate(function(value) {
              value[item.entity.id] <- !value?[item.entity.id]
            })
          }
        }
        else {
          local wasMarked = isItemSelected(item)
          clearSelection()
          if (!wasMarked) {
            markObject(item)
          }
        }
      }

      children = [
        {
          flow = FLOW_VERTICAL
          size = [flex(), SIZE_TO_CONTENT]
          children = [
            @() {
              rendObj = ROBJ_SOLID
              watch = [stateWatcher]
              size = [flex(), 1]
              color = getSeparatorColor(stateWatcher.get(), -1)
            }
            {
              flow = FLOW_HORIZONTAL
              size = [ flex(), SIZE_TO_CONTENT ]
              children = [
                {
                  halign = ALIGN_CENTER
                  valign = ALIGN_CENTER
                  flow = FLOW_HORIZONTAL
                  size = [ SIZE_TO_CONTENT, flex() ]
                  padding = [0, fsh(0.5), 0, fsh(0.5)]
                  children = getTreeControl(item, idx)
                }
                @() {
                  size = [ flex(), SIZE_TO_CONTENT ]
                  watch = [stateWatcher]
                  padding = [fsh(0.5), fsh(0.5), fsh(0.5), 0]
                  children = mkDataRow(item, (stateWatcher?.get().dropPosition == 0) || (sf & S_DRAG) ? dragColor.get() : textColor.get(),
                    sf & S_HOVER)
                }
              ]
            }
            @() {
              rendObj = ROBJ_SOLID
              watch = [stateWatcher]
              size = [flex(), 1]
              color = getSeparatorColor(stateWatcher.get(), 1)
            }
          ]
        }
      ]
    }
  })
}

de4workMode.subscribe(@(_) gui_scene.resetTimeout(0.1, initLists))

function isAnyEntitySelected() {
  return selectionStateEntities?.get().filter(@(val, _) val).len() != 0
}

function createSelectButton() {
  return textButton("Select", selectObjects, {
    boxStyle = {
      normal = {
        margin = 0
      }},
    onHover = @(on) setTooltip(on ? "Select items in the viewport" : null) })
}

let canAddImport = Computed(function() {
  let selectedSceneIds = getSelectedScenesIndicies(markedStateScenes)
  if (selectedSceneIds?.len() != 1 || (selectionStateEntities.get() != null && isAnyEntitySelected())) {
    return false
  }

  local scene = sceneIdMap?.get()[selectedSceneIds[0]]
  if (scene?.loadType != 3) {
    return false
  }

  return canSceneBeModified(scene)
})

function createRemoveObjectsButton() {
  let canRemoveObjects = Computed(function() {
    UNUSED(edObjectFlagsUpdateTrigger)

    let selectedItems = getAllSelectedItems()
    if (selectedItems.len() == 0) {
      return false;
    }

    foreach (item in selectedItems) {
      if (item.isEntity) {
        let parentSceneId = entity_editor?.get_instance().getEntityRecordSceneId(item.id)
        if (entity_editor?.get_instance()?.isEntityLocked(item.id) || (parentSceneId != ecs.INVALID_SCENE_ID && !canSceneBeModified(sceneIdMap?.get()[parentSceneId]))) {
          return false
        }
      }
      else {
        let scene = sceneIdMap?.get()[item.id]
        if (!canSceneBeModified(scene) || scene?.importDepth == 0) {
          return false
        }
      }
    }

    return true
  })

  return @() {
    watch = [edObjectFlagsUpdateTrigger, canRemoveObjects]
    children = textButton("Remove", function() {
      entity_editor?.get_instance().removeObjects(getAllSelectedItems())
      },
      {
        off = !canRemoveObjects.get()
        disabled = Computed(@() !canRemoveObjects.get() )
        onHover = @(on) setTooltip(on ? Computed(@() canRemoveObjects.get() ? "Remove selected items" : "First, select the target items") : null)
        boxStyle = {
          normal = {
            margin = 0
          }
      }})
  }
}

function createHideObjectsButton() {
  let canHideObjects = Computed(function() {
    UNUSED(markedStateScenes)
    UNUSED(selectionStateEntities)

    let selectedItems = getAllSelectedItems()
    if (selectedItems.len() == 0) {
      return false;
    }

    return true
  })

  let isSelectionHidden = Computed(function() {
    UNUSED(markedStateScenes)
    UNUSED(selectionStateEntities)
    UNUSED(edObjectFlagsUpdateTrigger)

    let selectedItems = getAllSelectedItems()
    foreach (item in selectedItems) {
      if (item.isEntity) {
        if (!entity_editor?.get_instance().isEntityHidden(item.id)) {
          return false
        }
      }
      else {
        if (item.id == fakeScene.id) {
          if (!isFakeSceneHidden.get()) {
            return false
          }
        }
        else if (!entity_editor?.get_instance().isSceneHidden(item.id)) {
          return false
        }
      }
    }

    return true
  })

  return @() {
    watch = [isSelectionHidden, canHideObjects]
    children = textButton(isSelectionHidden.get() && canHideObjects.get() ? "Unhide" : "Hide", function() {
      if (isSelectionHidden.get()) {
        entity_editor?.get_instance().unhideObjects(getAllSelectedItems())
      }
      else {
        entity_editor?.get_instance().hideObjects(getAllSelectedItems())
      }
    }, {
      off = !canHideObjects.get(),
      disabled = Computed(@() !canHideObjects.get() ),
      onHover = @(on) setTooltip(on ? Computed(function() {
        if (!canHideObjects.get()) {
          return "First, select the target items"
        }

        return isSelectionHidden.get() ? "Show debug visualization for selected items in the viewport" : "Hide debug visualization for selected items in the viewport"
      }) : null),
      boxStyle = {
        normal = {
          margin = 0
        }
      }})
  }
}

function createLockObjectsButton() {
  let canLockObjects = Computed(function() {
    UNUSED(markedStateScenes)
    UNUSED(selectionStateEntities)

    let selectedItems = getAllSelectedItems()
    if (selectedItems.len() == 0) {
      return false;
    }

    return true
  })

  let isSelectionLocked = Computed(function() {
    UNUSED(markedStateScenes)
    UNUSED(selectionStateEntities)
    UNUSED(edObjectFlagsUpdateTrigger)

    let selectedItems = getAllSelectedItems()
    foreach (item in selectedItems) {
      if (item.isEntity) {
        if (!entity_editor?.get_instance().isEntityLocked(item.id)) {
          return false
        }
      }
      else {
        if (item.id == fakeScene.id) {
          if (!isFakeSceneLocked.get()) {
            return false
          }
        }
        else if (!entity_editor?.get_instance().isSceneLocked(item.id)) {
          return false
        }
      }
    }

    return true
  })

  return @() {
    watch = [isSelectionLocked, canLockObjects]
    children = textButton(isSelectionLocked.get() && canLockObjects.get() ? "Unlock" : "Lock", function() {
      if (isSelectionLocked.get()) {
        entity_editor?.get_instance().unlockObjects(getAllSelectedItems())
      }
      else {
        entity_editor?.get_instance().lockObjects(getAllSelectedItems())
      }
    }, {
      off = !canLockObjects.get(),
      disabled = Computed(@() !canLockObjects.get() )
      onHover = @(on) setTooltip(on ? Computed(function() {
        if (!canLockObjects.get()) {
          return "First, select the target items"
        }

        return isSelectionLocked.get() ? "Locked\nClick to unlock selected items" : "Unlocked\nClick to lock selected items"
      }) : null),
      boxStyle = {
        normal = {
          margin = 0
        }
      }})
  }
}

function createScenePropertiesControl() {
  let sceneIndex = Computed(function() {
    let selectedSceneIds = markedStateScenes.get()?.filter(@(marked, _sceneId) marked).keys()
    if (selectedSceneIds == null || selectedSceneIds.len() != 1) {
      return -1
    }
    let sceneId = selectedSceneIds[0]
    if (sceneIdMap.get()?[sceneId].loadType != 3) {
      return -1
    }

    return sceneId;
  })

  function getIsTransformable() {
    return sceneIndex.get() != -1 ? entity_editor?.get_instance().isSceneTransformable(sceneIndex.get()) : false
  }

  let isTransformable = Watched(getIsTransformable())
  let pivot = entity_editor?.get_instance().getScenePivot(sceneIndex.get())
  let pivotX = Watched(pivot ? pivot.x : "")
  let pivotY = Watched(pivot ? pivot.y : "")
  let pivotZ = Watched(pivot ? pivot.z : "")

  function onPivotXChanged(val) {
    if (isStringFloat(val)) {
      entity_editor?.get_instance().setScenePivot(sceneIndex.get(),
        Point3(val.tofloat(), pivotY.get().tofloat(), pivotZ.get().tofloat()))
    }
  }

  function onPivotYChanged(val) {
    if (isStringFloat(val)) {
      entity_editor?.get_instance().setScenePivot(sceneIndex.get(),
        Point3(pivotX.get().tofloat(), val.tofloat(), pivotZ.get().tofloat()))
    }
  }

  function onPivotZChanged(val) {
    if (isStringFloat(val)) {
      entity_editor?.get_instance().setScenePivot(sceneIndex.get(),
        Point3(pivotX.get().tofloat(), pivotY.get().tofloat(), val.tofloat()))
    }
  }

  function getPropertiesControls() {
    return [
      {
        flow = FLOW_HORIZONTAL
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        children = [
          {
              rendObj = ROBJ_TEXT
              text = "Transformable"
              color = colors.TextDefault
              margin = fsh(0.5)
          }
          mkCheckBox(isTransformable, function() {
            if (sceneIndex.get() != -1) {
              let currVal = isTransformable.get()
              entity_editor?.get_instance().setSceneTransformable(sceneIndex.get(), !currVal)
              isTransformable.set(!currVal)
            }
          })
        ]
      }
      {
        flow = FLOW_HORIZONTAL
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        children = [
          {
            rendObj = ROBJ_TEXT
            text = "Pivot"
            color = colors.TextDefault
            margin = fsh(0.5)
          }
          {
            flow = FLOW_HORIZONTAL
            halign = ALIGN_CENTER
            size = const [ hdpx(300), SIZE_TO_CONTENT ]
            children = [
              textInput(pivotX, { textmargin = [sh(0), sh(0)], valignText = ALIGN_CENTER, onChange = onPivotXChanged })
              textInput(pivotY, { textmargin = [sh(0), sh(0)], valignText = ALIGN_CENTER, onChange = onPivotYChanged })
              textInput(pivotZ, { textmargin = [sh(0), sh(0)], valignText = ALIGN_CENTER, onChange = onPivotZChanged })
            ]
          }
        ]
      }
    ]
  }

  return @() {
    flow = FLOW_HORIZONTAL
    halign = ALIGN_LEFT
    valign = ALIGN_CENTER
    watch = [sceneIndex]
    children = sceneIndex.get() != -1 ? getPropertiesControls() : []
  }
}

function createFilterControls() {
  let stateFlags = Watched(0)
  let toolTipText = Computed(function() {
    if (filterSelectedEntities.get()) {
      return "Only selected items are shown\nClick to show all items"
    }
    else {
      return "All items are shown\nClick to show only selected items"
    }
  })

  return @() {
    size = FLEX_H
    flow = FLOW_HORIZONTAL
    halign = ALIGN_LEFT
    valign = ALIGN_CENTER

    children = @() {
      rendObj = ROBJ_BOX
      fillColor = Color(0,0,0,64)
      borderRadius = hdpx(5)
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      flow = FLOW_HORIZONTAL
      watch = [stateFlags, filterSelectedEntities]
      children = [
        {
          rendObj = ROBJ_TEXT
          text = "All"
          color = colors.TextDefault
          margin = fsh(0.5)
        }
        {
          rendObj = ROBJ_SOLID
          size = [hdpx(46), hdpx(25)]
          color = colors.ControlBgOpaque
          behavior = Behaviors.Button
          onClick = @() filterSelectedEntities.set(!filterSelectedEntities.get())
          onElemState = @(nsf) stateFlags.set(nsf)

          onHover = @(on) setTooltip(on ? toolTipText : null)
          children = {
            flow = FLOW_HORIZONTAL
            margin = hdpx(3)
            size = flex()
            children = [
              {
                rendObj = ROBJ_SOLID
                size = flex()
                color = !filterSelectedEntities.get()
                  ? ((stateFlags.get() & S_HOVER) ? colors.Hover : Color(115, 115, 115))
                  : colors.ControlBgOpaque
              }
              {
                rendObj = ROBJ_SOLID
                size = flex()
                color = filterSelectedEntities.get()
                  ? ((stateFlags.get() & S_HOVER) ? colors.Hover : Color(115, 115, 115))
                  : colors.ControlBgOpaque
              }
            ]
          }
        }
        {
          rendObj = ROBJ_TEXT
          text = "Selected"
          color = colors.TextDefault
          margin = fsh(0.5)
        }
      ]
    }
  }
}

function mkImportButton() {
  return @() {
    size = [SIZE_TO_CONTENT, flex()]
    watch = [canAddImport]
    children = textButton("Import", function() {
        addImportDialog(function(path) {
          let selectedSceneIds = getSelectedScenesIndicies(markedStateScenes)
          if (selectedSceneIds?.len() == 1 && !isAnyEntitySelected()) {
            entity_editor?.get_instance().addImportScene(selectedSceneIds[0], path)
            expandedStateScenes?.mutate(function(value) {
              value[selectedSceneIds[0]] = true
            })
            updateAllScenes()
          }
        })
      },
      {
        off = !canAddImport.get(),
        disabled = Computed(@() !canAddImport.get() ),
        onHover = function(on) {
          local text = null
          if (on) {
            if (canAddImport.get()) {
              let selectedSceneIds = getSelectedScenesIndicies(markedStateScenes)
              text = $"Add a new scene or import an existing one into {fileName(sceneIdMap?.get()[selectedSceneIds[0]].path)}"
            }
            else {
              text = "Add a new scene or import an existing one\nFirst, select the target scene to import"
            }
          }
          setTooltip(text)
        },
        boxStyle = {
          normal = {
            size = [SIZE_TO_CONTENT, flex()]
            margin = 0
            padding = [hdpx(1),hdpx(10)]
          }
        }
        textStyle = {
          normal = {
            size = [SIZE_TO_CONTENT, flex()]
            valign = ALIGN_CENTER
            halign = ALIGN_CENTER
          }
      }})
  }
}

function mkFilterOptionsButton() {
  let dropDownOpen = Watched(false)
  let toggleDropDown = @() dropDownOpen.get() ? dropDownOpen.set(false) : dropDownOpen.set(true)
  let doHideDropDown = @() dropDownOpen.set(false)

  local popupLayer = 1
  if ("Layers" in getconsttable()) {
    popupLayer = getconsttable()?["Layers"].ComboPopup ?? 1
  }

  let isAnyFilterOptionEnabled = Computed(function() {
    return !showEntities.get() || !showClientScenes.get() || !showCommonScenes.get() || !showImportScenes.get() || !showOtherScenes.get()
  })

  function dropdownBgOverlay(onClick) {
    return {
      pos = [-9000, -9000]
      size = [19999, 19999]
      behavior = Behaviors.ComboPopup
      eventPassThrough = true
      onClick
    }
  }

  function popupWrapper(popupContent) {
    let children = [
      {size = [flex(), ph(100)]}
      {size = [flex(), hdpx(2)]}
      popupContent
    ]

    return {
      size = flex()
      flow = FLOW_VERTICAL
      vplace = ALIGN_TOP
      valign = ALIGN_TOP
      children
    }
  }

  function mkDropDownEntry(watched, text) {
    return {
      flow = FLOW_HORIZONTAL
      size = SIZE_TO_CONTENT
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      eventPassThrough = false
      gap = fsh(0.5)
      children = [
        mkCheckBox(watched, @() watched.set(!watched.get()))
        {
          size = SIZE_TO_CONTENT
          rendObj = ROBJ_TEXT
          text
          color = colors.TextDefault
        }
      ]
    }
  }

  function dropDownList() {
    let content = {
      size = SIZE_TO_CONTENT
      rendObj = ROBJ_BOX
      fillColor = const Color(50,50,50)
      borderColor = Color(80,80,80)
      borderWidth = 1
      stopMouse = true
      clipChildren = true
      flow = FLOW_VERTICAL
      padding = fsh(1.0)
      gap = fsh(0.5)
      children = [
        {
          size = SIZE_TO_CONTENT
          rendObj = ROBJ_TEXT
          text = "Show:"
          color = colors.TextDefault
        }
        mkDropDownEntry(showEntities, "Entities")
        mkDropDownEntry(showImportScenes, "Import scenes")
        mkDropDownEntry(showClientScenes, "Client scenes")
        mkDropDownEntry(showCommonScenes, "Common scenes")
        mkDropDownEntry(showOtherScenes, "Other scenes")
      ]
      hplace = ALIGN_RIGHT
    }

    return {
      zOrder = popupLayer
      size = flex()
      children = [
        dropdownBgOverlay(doHideDropDown)
        {
          size = flex()
          behavior = Behaviors.Button
          onClick = @() doHideDropDown()
          rendObj = ROBJ_FRAME
        }
        popupWrapper(content)
      ]
      hplace = ALIGN_RIGHT
    }
  }

  return watchElemState( @(sf) {
    rendObj = ROBJ_BOX
    behavior = Behaviors.Button
    size = [SIZE_TO_CONTENT, flex()]
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    watch = [dropDownOpen]
    fillColor = (sf & S_HOVER) ? Color(255, 255, 255, 255) : Color(0,0,0,64)
    onClick = @() toggleDropDown()
    onHover = @(on) setTooltip(on ? "Outliner filter settings" : null)

    children = @() {
      rendObj = ROBJ_IMAGE
      image = Picture(getIcon("filter_default"))
      size = [hdpx(26), hdpx(26)]
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      watch = [dropDownOpen, isAnyFilterOptionEnabled]
      color = (sf & S_HOVER) || isAnyFilterOptionEnabled.get() ? Color(66, 176, 255) : Color(255, 255, 255, 255)
      children = dropDownOpen.get() ? dropDownList : null
    }
  })
}

function mkScenesList() {

  function listSceneContent() {
    sceneRowItemStateWatchers.clear()

    local sRows = filteredItems.get().map(@(item, idx) listSceneRow(item, idx))

    return {
      watch = [allScenesWatcher, filteredItems]
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
      onAttach = @() scrollScenesBySelection()
    }
  })

  return  @() {
    flow = FLOW_VERTICAL
    gap = fsh(0.5)
    watch = [allScenesWatcher, allEntities, selectionStateEntities, selectedEntities, sceneListUpdateTrigger]
    size = flex()
    children = [
      {
        size = FLEX_H
        flow = FLOW_HORIZONTAL
        gap = fsh(0.5)
        children = [
          mkImportButton()
          filter
          mkFilterOptionsButton()
        ]
      }
      {
        flow = FLOW_HORIZONTAL
        size = [flex(), SIZE_TO_CONTENT]
        children = [
          createFilterControls()
          hflow(
            HARight
            textButton("Expand all", function () {
              expandedStateScenes.mutate(function (value) {
                foreach (id, _state in value) {
                  value[id] = true
                }
              })
            }, { onHover = @(on) setTooltip(on ? "Expand the entire hierarchy" : null)})
            textButton("Collapse all", function () {
              expandedStateScenes.mutate(function (value) {
                foreach (id, _state in value) {
                  value[id] = false
                }
              })
            }, { onHover = @(on) setTooltip(on ? "Collapse the entire hierarchy" : null)})
          )
        ]
      }
      {
        size = flex()
        children = scrollListScenes
      }
      statusLineScenes
      createScenePropertiesControl()
      {
        flow = FLOW_HORIZONTAL
        size = FLEX_H
        halign = ALIGN_CENTER
        gap = fsh(0.5)
        children = [
          createSelectButton()
          createHideObjectsButton()
          createLockObjectsButton()
          createRemoveObjectsButton()
        ]
      }
    ]
    hotkeys = [
      ["L.Ctrl A", markAllObjects]
    ]
  }
}

return {
  id = SceneOutlinerWndId
  onAttach = updateAllScenes
  mkContent = mkScenesList
  saveState=true
  headerText = "Scene Outliner"
}
