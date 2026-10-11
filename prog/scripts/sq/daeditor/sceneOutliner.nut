from "string" import format
from "math" import min, max
from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let entity_editor = require_optional("entity_editor")
let { SceneOutlinerWndId, selectedEntities, edObjectFlagsUpdateTrigger, DEPENDS_ON } = require("state.nut")
let { sceneTree, sortedScenes, sceneIdMap, canSceneBeModified, sceneDisplayName } = require("sceneModel.nut")
let { selection, selectedItems, hasSelection, markedSceneCount, isObjMarked,
  markObjects, toggleObject, setSelection, clearSelection, applyToEditor } = require("selection.nut")
let { colors } = require("components/style.nut")
let textButton = require("components/textButton.nut")
let { mkListFilter } = require("components/mkFilteredList.nut")
let mkCheckBox = require("components/mkCheckBox.nut")
let { makeVertScroll } = require("%daeditor/components/scrollbar.nut")
let textInput = require("%daeditor/components/textInput.nut")
let { getSceneLoadTypeText } = require("%daeditor/daeditor_es.nut")
let { addModalWindow, removeModalWindow, getPopupZOrder } = require("%daeditor/components/modalWindows.nut")
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
let expandedStateScenes = mkWatched(persist, "expandedStateScenes", {})
let filterString = mkWatched(persist, "filterString", "")
let scrollHandler = ScrollHandler()

let statusAnimTrigger = { lastN = null }

function sceneToText(scene) {
  return $"{scene.importDepth == 0 ? "*** " : ""}{sceneDisplayName(scene)}"
}

let removeSelectedByEditorTemplate = @(tname) tname.replace("+daeditor_selected+","+").replace("+daeditor_selected","").replace("daeditor_selected+","")

function entityToTxt(eid) {
  local tplName = ecs.g_entity_mgr.getEntityTemplateName(eid) ?? ""
  let name = removeSelectedByEditorTemplate(tplName)
  return $"{eid} - {name}"
}

// The filter controls as one value of plain fields, so an unchanged filter does not
// propagate. The selection joins it in filteredTree: that table is edited in place, and a
// Computed that carried it in its value would see the same object and never propagate.
let itemFilter = Computed(@() {
  showEntities = showEntities.get()
  showClientScenes = showClientScenes.get()
  showCommonScenes = showCommonScenes.get()
  showImportScenes = showImportScenes.get()
  showOtherScenes = showOtherScenes.get()
  onlySelected = filterSelectedEntities.get()
  filterStr = (filterString.get() ?? "").tolower()
})

let isFilteringEnabled = Computed(function() {
  let filter = itemFilter.get()
  return filter.onlySelected || filter.filterStr.len() != 0 || !filter.showEntities
})

let fakeScene = {
  loadType = 3
  path = "Entities without scene"
  importDepth = 1
  parent = ecs.INVALID_SCENE_ID
  hasParent = false
  imports = 0
  id = ecs.INVALID_SCENE_ID
}

let isFakeSceneHidden = Computed(function() {
  DEPENDS_ON(edObjectFlagsUpdateTrigger)

  let entities = sceneTree.get().freeEntities
  foreach (id in entities) {
    if (!entity_editor?.get_instance().isEntityHidden(id)) {
      return false
    }
  }
  return true
})

let isFakeSceneLocked = Computed(function() { // warning disable: -similar-assigned-expr
  DEPENDS_ON(edObjectFlagsUpdateTrigger)

  let entities = sceneTree.get().freeEntities
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

// A scene type that is switched off hides its whole subtree.
function isSceneTypeHidden(scene, filter) {
  if (scene.id == fakeScene.id) {
    return !filter.showOtherScenes
  }
  return (scene.loadType == 1 && !filter.showCommonScenes) ||
    (scene.loadType == 2 && !filter.showClientScenes) ||
    (scene.loadType == 3 && !filter.showImportScenes)
}

// item is a tree node or a row: { scene } or { entity = { id } }
function filterItem(item, filter) {
  if (item?.entity != null && !filter.showEntities) {
    return false
  }

  if (filter.onlySelected) {
    if (item?.entity == null) {
      return false
    }

    if (!filter.selectedEntities?[item.entity.id]) {
      return false
    }
  }

  if (filter.filterStr.len() != 0) {
    let itemStr = item?.scene != null ? sceneToText(item.scene) : entityToTxt(item.entity.id)
    return itemStr.tolower().contains(filter.filterStr)
  }
  return true
}

// The scene's node { scene, children } in editor order with the filtered-out content
// removed, or null when neither the scene nor anything under it passes the filter.
function filterSceneNode(scene, scenes, tree, filter) {
  if (isSceneTypeHidden(scene, filter)) {
    return null
  }

  let children = []
  if (scene.id == fakeScene.id) {
    foreach (eid in tree.freeEntities) {
      let child = { entity = { id = eid, parentSceneId = fakeScene.id } }
      if (filterItem(child, filter)) {
        children.append(child)
      }
    }
  }
  else {
    foreach (entry in tree.entries?[scene.id] ?? []) {
      if (entry.isEntity) {
        let child = { entity = { id = entry.eid, parentSceneId = scene.id } }
        if (filterItem(child, filter)) {
          children.append(child)
        }
      }
      else {
        let subScene = scenes?[entry.sid]
        let child = subScene != null ? filterSceneNode(subScene, scenes, tree, filter) : null
        if (child != null) {
          children.append(child)
        }
      }
    }
  }

  let node = { scene, children }
  return children.len() != 0 || filterItem(node, filter) ? node : null
}

// Root nodes: the root scenes in sorted order, then the entities without scene.
let filteredTree = Computed(function() {
  let filter = itemFilter.get().__merge({ selectedEntities = selectedEntities.get() })
  let tree = sceneTree.get()
  let scenes = sceneIdMap.get()

  let roots = []
  foreach (scene in sortedScenes.get()) {
    if (!scene.hasParent) {
      let node = filterSceneNode(scene, scenes, tree, filter)
      if (node != null) {
        roots.append(node)
      }
    }
  }
  let freeEntities = filterSceneNode(fakeScene, scenes, tree, filter)
  if (freeEntities != null) {
    roots.append(freeEntities)
  }
  return roots
})

function forEachTreeNode(nodes, fn) {
  foreach (node in nodes) {
    fn(node)
    if (node?.children != null) {
      forEachTreeNode(node.children, fn)
    }
  }
}

function findSceneNode(nodes, sceneId) {
  foreach (node in nodes) {
    if (node?.scene != null) {
      if (node.scene.id == sceneId) {
        return node
      }
      let found = findSceneNode(node.children, sceneId)
      if (found != null) {
        return found
      }
    }
  }
  return null
}

// Rows of the visible part of the tree. order is the index among the shown siblings; the
// list of entities without scene is cut at MAX_FREE_ENTITIES rows.
function appendRows(node, order, depth, expanded, rows) {
  if (node?.entity != null) {
    rows.append({ entity = node.entity, order, depth })
    return
  }

  rows.append({ scene = node.scene, order, depth, hasChildren = node.children.len() != 0 })
  if (!(expanded?[node.scene.id] ?? false)) {
    return
  }

  let isFakeScene = node.scene.id == fakeScene.id
  foreach (i, child in node.children) {
    if (isFakeScene && i >= MAX_FREE_ENTITIES) {
      rows.append({ text = "More entities ...", parentSceneId = fakeScene.id, depth = depth + 1 })
      break
    }
    appendRows(child, i, depth + 1, expanded, rows)
  }
}

let filteredItems = Computed(function() {
  let expanded = expandedStateScenes.get()
  let rows = []
  foreach (order, node in filteredTree.get()) {
    appendRows(node, order, 0, expanded, rows)
  }
  return rows
})

function getEntityCount(scene, tree) {
  if (scene.id == fakeScene.id) {
    return tree.freeEntities.len()
  }
  return tree.entityCounts?[scene.id] ?? 0
}

let filteredScenesCount = Computed(@() filteredItems.get().filter(@(item) item?.scene != null && item.scene.id != ecs.INVALID_SCENE_ID).len())

let filteredScenesEntityCount = Computed(function() {
  let tree = sceneTree.get()
  local eCount = 0
  foreach (item in filteredItems.get()) {
    if (item?.scene != null) {
      eCount += getEntityCount(item.scene, tree)
    }
  }
  return eCount
})

let numMarkedScenesEntityCount = Computed(function() {
  let tree = sceneTree.get()
  let marked = selection.get().scenes
  local count = 0
  foreach (item in filteredItems.get()) {
    if (item?.scene != null && item.scene.id in marked) {
      count += getEntityCount(item.scene, tree)
    }
  }
  return count
})

// { id, isEntity, loadType? } as the editor commands take it; null for a text row
function itemToObj(item) {
  if (item?.entity != null) {
    return { id = item.entity.id, isEntity = true }
  }
  if (item?.scene != null) {
    return { id = item.scene.id, isEntity = false, loadType = item.scene.loadType }
  }
  return null
}

function isItemSelected(item, sel) {
  let obj = itemToObj(item)
  return obj != null && isObjMarked(sel, obj)
}

function markTreeNodes(nodes, on) {
  let objs = []
  forEachTreeNode(nodes, @(node) objs.append(itemToObj(node)))
  markObjects(objs, on)
}

function scrollScenesBySelection() {
  let sel = selection.get()
  scrollHandler.scrollToChildren(@(desc) ("item" in desc) && isItemSelected(desc.item, sel), 2, false, true)
}

function scnTxt(count): string { return count==1 ? "scene" : "scenes" }
function entTxt(count): string { return count==1 ?  "entity" : "entities" }
function statusText(count, textFunc: function): string { return format("%d %s", count, textFunc(count)) }

function statusLineScenes() {
  let sMrk = markedSceneCount.get()
  let eMrk = numMarkedScenesEntityCount.get()
  let eRec = filteredScenesEntityCount.get()

  if (statusAnimTrigger.lastN != null && statusAnimTrigger.lastN != sMrk)
    anim_start(statusAnimTrigger)
  statusAnimTrigger.lastN = sMrk

  return {
    watch = [numMarkedScenesEntityCount, filteredScenesCount, filteredScenesEntityCount, markedSceneCount]
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
  setSelection(filteredItems.get().map(itemToObj).filter(@(obj) obj != null))
}

function getIcon(name) {
  return $"!%daeditor/images/{name}"
}

let IconButton = StatefulComp(function(scope, icon, onClick, visible, parentHovered, opacity, tooltip) {
  let stateFlags = scope.Watched(0)
  return function() {
    let hovered = (stateFlags.get() & S_HOVER) != 0
    let click = onClick.get()
    return {
      watch = [stateFlags, icon, onClick, visible, parentHovered, opacity, tooltip]
      rendObj = ROBJ_BOX
      behavior = (click != null && visible.get()) ? Behaviors.Button : Behaviors.TrackMouse
      onClick = click
      onElemState = @(sf) stateFlags.set(sf & S_HOVER)
      size = SIZE_TO_CONTENT
      onHover = @(on) setTooltip(on && tooltip.get() != null ? tooltip : null)
      children = {
        rendObj = ROBJ_IMAGE
        image = !visible.get() ? null : (hovered || parentHovered.get() ? Picture(icon.get()) : null)
        size = const [hdpx(20), hdpx(20)]
        valign = ALIGN_CENTER
        halign = ALIGN_CENTER
        color = click != null && hovered ? Color(66, 176, 255) : Color(255, 255, 255, 255)
        opacity = opacity.get()
      }
    }
  }
})

function gatherItemsAndModifySelection(op) {
  markTreeNodes(filteredTree.get(), op)
}

let hasFilteredItems = Computed(@() isFilteringEnabled.get() && filteredItems.get().len() != 0)

let filter = mkListFilter(filterString, {
  placeholder = "Filter and search"
  additionalChildren = [
    @() {
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      size = SIZE_TO_CONTENT
      pad = fsh(0.5)
      watch = [isFilteringEnabled, filteredItems]
      children = IconButton(getIcon("filter_selected"),
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
      children = IconButton(getIcon("filter_unselected"),
        hasFilteredItems.get() ? @() gatherItemsAndModifySelection(false) : null,
        true, true, hasFilteredItems.get() ? 0.9 : 0.5, hasFilteredItems.get()
          ? "Unselect all currently filtered items"
          : "There are no matches for unselection")
    }
  ]
})

// Every scene needs an entry: "Expand all" and "Collapse all" walk the entries
function initScenesList() {
  let scenes = sceneIdMap.get()
  expandedStateScenes.modify(function(expanded) {
    let next = {}
    foreach (id, _scene in scenes) {
      next[id] <- expanded?[id] ?? false
    }
    next[ecs.INVALID_SCENE_ID] <- expanded?[ecs.INVALID_SCENE_ID] ?? false
    return next
  })
}

sortedScenes.subscribe_with_nasty_disregard_of_frp_update(@(_v) deferOnce(initScenesList))

const setSceneNameUID = "set_scene_name_modal_window"

function setScenePrettyName(sceneId) {
  let sceneName = Watched(entity_editor?.get_instance().getScenePrettyName(sceneId))
  let close = @() removeModalWindow(setSceneNameUID)

  function doSetSceneName() {
    local confirmationUID = "clear_scene_name_modal_window"

    function applySceneName() {
        entity_editor?.get_instance().setScenePrettyName(sceneId, sceneName.get())
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

function getItemParentScene(item) {
  return item?.scene != null ? item.scene.parent : (item?.entity != null ? item.entity.parentSceneId : item.parentSceneId)
}

function getTreeControl(item, isLastSibling) {
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
        size = const [hdpx(TREE_CONTROL_CONTROL_WIDTH), flex()]
        valign = ALIGN_CENTER
        halign = ALIGN_CENTER
        children = {
          rendObj = ROBJ_SOLID
          size = const [1, flex()]
          color = TREE_CONNECTIONS_COLOR
        }
      }
    )

    ++currentOffset
  }

  if ((isScene && !(item.scene?.importDepth == 0 || (item.scene?.importDepth != 0 && !entity_editor?.get_instance().isChildScene(item.scene.id))))
    || !isScene) {
    if (isLastSibling) {
      objs.append(
        {
          size = const [hdpx(TREE_CONTROL_CONTROL_WIDTH), flex()]
          valign = ALIGN_CENTER
          halign = ALIGN_RIGHT
          flow = FLOW_HORIZONTAL
          children = [
            {
              size = const [1, flex()]
              flow = FLOW_VERTICAL
              children = [
                {
                  rendObj = ROBJ_SOLID
                  valign = ALIGN_TOP
                  size = const [1, flex()]
                  color = TREE_CONNECTIONS_COLOR
                }
                {
                  valign = ALIGN_TOP
                  size = const [1, flex()]
                }
              ]
            }
            {
              halign = ALIGN_RIGHT
              rendObj = ROBJ_SOLID
              size = const [hdpx(TREE_CONTROL_CONTROL_WIDTH) / 2, 1]
              color = TREE_CONNECTIONS_COLOR
            }
          ]
        }
      )
    }
    else {
      objs.append(
        {
          size = const [hdpx(TREE_CONTROL_CONTROL_WIDTH), flex()]
          valign = ALIGN_CENTER
          halign = ALIGN_RIGHT
          flow = FLOW_HORIZONTAL
          children = [
            {
              rendObj = ROBJ_SOLID
              size = const [1, flex()]
              color = TREE_CONNECTIONS_COLOR
            }
            {
              halign = ALIGN_RIGHT
              rendObj = ROBJ_SOLID
              size = const [hdpx(TREE_CONTROL_CONTROL_WIDTH) / 2, 1]
              color = TREE_CONNECTIONS_COLOR
            }
          ]
        }
      )
    }
  }

  if (isScene && item.hasChildren) {
    objs.append({
      size = const [hdpx(TREE_CONTROL_CONTROL_WIDTH), flex()]
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
    padding = const [0, fsh(0.5), 0, fsh(0.5)]
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
          size = const [hdpx(20), hdpx(20)]
          color = color
        }
        : null
    ]
  }
}

function isItemHidden(item) {
  if (item?.scene != null) {
    if (item.scene.id == fakeScene.id) {
      return isFakeSceneHidden.get()
    }
    else {
      return entity_editor?.get_instance().isSceneHidden(item.scene.id)
    }
  }
  else if (item?.entity != null) {
    return entity_editor?.get_instance().isEntityHidden(item.entity.id)
  }
  else {
    return true
  }
}

function isItemLocked(item) {
  if (item?.scene != null) {
    if (item.scene.id == fakeScene.id) {
      return isFakeSceneLocked.get()
    }
    else {
      return entity_editor?.get_instance().isSceneLocked(item.scene.id)
    }
  }
  else if (item?.entity != null) {
    return entity_editor?.get_instance().isEntityLocked(item.entity.id)
  }
  else {
    return true
  }
}

// row = { hidden, locked, tooltips }: the row's observables that the icons read
function mkDataRow(item, textColor, hovered, row) {
  let { hidden, locked, tooltips } = row
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

  let obj = itemToObj(item)

  function getSceneTypeTagText(scene) {
    return scene.id != fakeScene.id ? getSceneLoadTypeText(scene) : "OTHER"
  }

  return {
    size = const [flex(), SIZE_TO_CONTENT]
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
        children = IconButton(
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
        ? mkTag(getIcon("select_none"), Color(75, 51, 49), Color(174, 143, 109), getEntityCount(item.scene, sceneTree.get()), "88888",
          $"{getEntityCount(item.scene, sceneTree.get())} entities")
        : { rendObj = ROBJ_BOX, size = [ getTagWidth(true, "88888"), flex() ] }
      {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        children = IconButton(getIcon("move"), null, true, true, getTransformableIconOpacity(),
          isTransformable() ? "Transformable" : "Not transformable")
      }
      {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        children = IconButton(getIcon("zoom_and_center"), function() {
          setSelection([itemToObj(item)])
          applyToEditor()
          entity_editor?.get_instance().zoomAndCenter()
        }, !isOtherItem, hovered, 0.9, "Zoom in at center in the viewport")
      }
      {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        children = IconButton(getIcon("layer_entity"), function() {
          let wasSelected = isItemSelected(item, selection.get())
          if (isScene) {
            let node = findSceneNode(filteredTree.get(), item.scene.id)
            markTreeNodes(node != null ? [node] : [], !wasSelected)
          }
          else if (isEntity) {
            markObjects([itemToObj(item)], !wasSelected)
          }
        }, !isOtherItem, hovered, 0.9, tooltips.select)
      }
      @() {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        watch = hidden
        children = IconButton(hidden.get() ? getIcon("eye_hide") : getIcon("eye_show"), function() {
          if (hidden.get()) {
            entity_editor?.get_instance().unhideObjects([obj])
          }
          else {
            entity_editor?.get_instance().hideObjects([obj])
          }
        }, !isOtherItem, hovered || hidden.get(), 0.9, tooltips.hide)
      }
      @() {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        watch = locked
        children = IconButton(locked.get() ? getIcon("lock_close") : getIcon("lock_open"), function() {
          if (locked.get()) {
            entity_editor?.get_instance().unlockObjects([obj])
          }
          else {
            entity_editor?.get_instance().lockObjects([obj])
          }
        },
        !isOtherItem,
        hovered || locked.get(), 0.9, tooltips.lock)
      }
      {
        halign = ALIGN_LEFT
        valign = ALIGN_CENTER
        children = IconButton(getIcon("delete"), function() {
          entity_editor?.get_instance().removeObjects([obj])
        }, !isOtherItem, hovered, canBeRemoved() ? 0.9 : 0.5, canBeRemoved()
          ? "Remove item"
          : "This item cannot be removed")
      }
    ]
  }
}

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

function updateDropPosition(newPosition) {
  let dragDest = dragDestData.get()
  if (dragDest != null && dragDest.dropPosition != newPosition) {
    dragDestData.mutate(@(v) v.dropPosition = newPosition)
  }
}

function separatorColor(dropPosition, canDrop, position) {
  if (dropPosition != null && dropPosition != 0 && dropPosition == position) {
    return canDrop ? Color(255, 255, 255) : Color(255, 0, 0)
  }
  return Color(0, 0, 0)
}

let itemKey = @(item) item?.scene != null ? $"s{item.scene.id}" : item?.entity != null ? $"e{item.entity.id}" : $"t{item.parentSceneId}"

// The key is the item id, so a row keeps its state while filteredItems is rebuilt
// around it. Items are fresh tables on each rebuild, so nothing caches item.get().
let SceneRow = StatefulComp(function(scope, item, idx, isLastSibling) {
  let stateFlags = scope.Watched(0)
  let isMarked = scope.Computed(@() isItemSelected(item.get(), selection.get()))
  let hidden = scope.Computed(function() {
    DEPENDS_ON(edObjectFlagsUpdateTrigger)
    return isItemHidden(item.get())
  })
  let locked = scope.Computed(function() {
    DEPENDS_ON(edObjectFlagsUpdateTrigger)
    return isItemLocked(item.get())
  })
  // -1: drop above this row, 1: below it, 0: into it, null: not a drop target
  let dropPosition = scope.Computed(function() {
    let dragDest = dragDestData.get()
    if (dragDest == null) {
      return null
    }
    if (dragDest.dropPosition == null) {
      return dragDest.item == item.get() ? 0 : null
    }
    let destPos = dragDest.dropPosition > 0 ? 0 : -1
    if (idx.get() - 1 == dragDest.index + destPos) {
      return -1
    }
    if (idx.get() == dragDest.index + destPos) {
      return 1
    }
    return null
  })
  let textColor = scope.Computed(function() {
    let sf = stateFlags.get()
    if ((sf & S_DRAG) || dropPosition.get() == 0) {
      return (sf & S_DRAG) ? Color(255, 255, 0) : canBeDropped.get() ? Color(255, 255, 255) : Color(255, 0, 0)
    }
    return isMarked.get() ? colors.TextDefault : (item.get()?.scene != null ? Color(50, 166, 168) : colors.TextDarker)
  })
  let topSeparatorColor = scope.Computed(@() separatorColor(dropPosition.get(), canBeDropped.get(), -1))
  let bottomSeparatorColor = scope.Computed(@() separatorColor(dropPosition.get(), canBeDropped.get(), 1))
  let tooltips = {
    select = scope.Computed(@() isMarked.get()
      ? "Selected\nClick to unselect this item and all nested items in the viewport"
      : "Not selected\nClick to select this item and all nested items in the viewport")
    hide = scope.Computed(@() hidden.get()
      ? "Hidden\nClick to show debug visualization in the viewport"
      : "Visible\nClick to hide debug visualization in the viewport")
    lock = scope.Computed(@() locked.get()
      ? "Locked\nClick to unlock editing"
      : "Unlocked\nClick to lock editing")
  }
  let row = { hidden, locked, tooltips }

  return function() {
    let cur = item.get()
    let isScene = cur?.scene != null
    let sf = stateFlags.get()
    let rowColor = isMarked.get() ? colors.Active
      : sf & S_TOP_HOVER ? colors.GridRowHover
      : colors.GridBg[idx.get() % colors.GridBg.len()]

    return {
      rendObj = ROBJ_SOLID
      size = FLEX_H
      color = rowColor
      item = cur
      watch = [item, idx, isLastSibling, isMarked, stateFlags, textColor, topSeparatorColor, bottomSeparatorColor,
        expandedStateScenes, sceneIdMap]
      behavior = cur?.text == null ? [Behaviors.TrackMouse, Behaviors.DragAndDrop] : []
      flow = FLOW_HORIZONTAL
      eventPassThrough = true
      dropData = cur
      onElemState = @(flags) stateFlags.set(flags & (S_TOP_HOVER | S_HOVER | S_DRAG))

      canDrop = function(_data) {
        dragDestData.set({item = cur, index = idx.get(), dropPosition = dragDestData?.get().dropPosition })
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
        if (isMarked.get()) {
          itemDragData.set(on ? selectedItems.get() : null)
        }
        else {
          let dragItem = {}
          dragItem.id <- isScene ? cur.scene.id : cur.entity.id
          dragItem.isEntity <- !isScene
          if (isScene) {
            dragItem.loadType <- cur.scene.loadType ?? 0
          }
          itemDragData.set(on ? [dragItem] : null)
        }

        if (!on) {
          dragDestData.set(null)
        }
      }

      onDoubleClick = function(_evt) {
        setSelection([itemToObj(cur)])
        applyToEditor()
      }

      onClick = function(evt) {
        let obj = itemToObj(cur)
        if (obj == null) {
          return
        }
        if (evt.shiftKey) {
          let sel = selection.get()
          let items = filteredItems.get()
          local idx1 = idx.get()
          local idx2 = idx.get()
          foreach (i, filteredItem in items) {
            if (isItemSelected(filteredItem, sel)) {
              idx1 = min(idx1, i)
              idx2 = max(idx2, i)
            }
          }
          if (hasSelection.get()) {
            markObjects(items.slice(idx1, idx2 + 1).map(itemToObj).filter(@(o) o != null), !evt.ctrlKey)
          }
        }
        else if (evt.ctrlKey) {
          toggleObject(obj)
        }
        else if (isItemSelected(cur, selection.get())) {
          clearSelection()
        }
        else {
          setSelection([obj])
        }
      }

      children = [
        {
          flow = FLOW_VERTICAL
          size = const [flex(), SIZE_TO_CONTENT]
          children = [
            {
              rendObj = ROBJ_SOLID
              size = const [flex(), 1]
              color = topSeparatorColor.get()
            }
            {
              flow = FLOW_HORIZONTAL
              size = const [ flex(), SIZE_TO_CONTENT ]
              children = [
                {
                  halign = ALIGN_CENTER
                  valign = ALIGN_CENTER
                  flow = FLOW_HORIZONTAL
                  size = const [ SIZE_TO_CONTENT, flex() ]
                  padding = const [0, fsh(0.5), 0, fsh(0.5)]
                  children = getTreeControl(cur, isLastSibling.get())
                }
                {
                  size = const [ flex(), SIZE_TO_CONTENT ]
                  padding = const [fsh(0.5), fsh(0.5), fsh(0.5), 0]
                  children = mkDataRow(cur, textColor.get(), (sf & S_HOVER) != 0, row)
                }
              ]
            }
            {
              rendObj = ROBJ_SOLID
              size = const [flex(), 1]
              color = bottomSeparatorColor.get()
            }
          ]
        }
      ]
    }
  }
}, itemKey)


function singleMarkedImportScene(sel, scenes) {
  let sceneIds = sel.scenes.keys()
  if (sceneIds.len() != 1 || sel.entities.len() != 0) {
    return null
  }
  let scene = scenes?[sceneIds[0]]
  return scene?.loadType == 3 ? scene : null
}

function createSelectButton() {
  return textButton("Select", applyToEditor, {
    boxStyle = {
      normal = {
        margin = 0
      }},
    onHover = @(on) setTooltip(on ? "Select items in the viewport" : null) })
}

let canAddImport = Computed(function() {
  let scene = singleMarkedImportScene(selection.get(), sceneIdMap.get())
  return scene != null && canSceneBeModified(scene)
})

function createRemoveObjectsButton() {
  let canRemoveObjects = Computed(function() {
    DEPENDS_ON(edObjectFlagsUpdateTrigger)

    if (!hasSelection.get()) {
      return false
    }

    foreach (item in selectedItems.get()) {
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
      entity_editor?.get_instance().removeObjects(selectedItems.get())
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
  let isSelectionHidden = Computed(function() {
    DEPENDS_ON(edObjectFlagsUpdateTrigger)

    foreach (item in selectedItems.get()) {
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
    watch = [isSelectionHidden, hasSelection]
    children = textButton(isSelectionHidden.get() && hasSelection.get() ? "Unhide" : "Hide", function() {
      if (isSelectionHidden.get()) {
        entity_editor?.get_instance().unhideObjects(selectedItems.get())
      }
      else {
        entity_editor?.get_instance().hideObjects(selectedItems.get())
      }
    }, {
      off = !hasSelection.get(),
      disabled = Computed(@() !hasSelection.get() ),
      onHover = @(on) setTooltip(on ? Computed(function() {
        if (!hasSelection.get()) {
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
  let isSelectionLocked = Computed(function() {
    DEPENDS_ON(edObjectFlagsUpdateTrigger)

    foreach (item in selectedItems.get()) {
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
    watch = [isSelectionLocked, hasSelection]
    children = textButton(isSelectionLocked.get() && hasSelection.get() ? "Unlock" : "Lock", function() {
      if (isSelectionLocked.get()) {
        entity_editor?.get_instance().unlockObjects(selectedItems.get())
      }
      else {
        entity_editor?.get_instance().lockObjects(selectedItems.get())
      }
    }, {
      off = !hasSelection.get(),
      disabled = Computed(@() !hasSelection.get() )
      onHover = @(on) setTooltip(on ? Computed(function() {
        if (!hasSelection.get()) {
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

let ScenePropertiesControl = StatefulComp(function(scope) {
  let sceneIndex = scope.Computed(function() {
    let sceneIds = selection.get().scenes.keys()
    if (sceneIds.len() != 1 || sceneIdMap.get()?[sceneIds[0]].loadType != 3) {
      return -1
    }
    return sceneIds[0]
  })

  let isTransformable = scope.Watched(false)
  let pivotX = scope.Watched("")
  let pivotY = scope.Watched("")
  let pivotZ = scope.Watched("")

  // A field whose text already means the record's value is left alone: the user may be typing in it
  function syncPivotField(field, value) {
    let text = field.get().tostring()
    if (!isStringFloat(text) || text.tofloat() != value) {
      field.set(value)
    }
  }

  // Undo and redo change the pivot and the flag behind the panel's back
  function readFromEditor(_v) {
    let scene = sceneIdMap.get()?[sceneIndex.get()]
    isTransformable.set(scene?.transformable ?? false)
    if (scene == null) {
      pivotX.set("")
      pivotY.set("")
      pivotZ.set("")
      return
    }
    syncPivotField(pivotX, scene.pivot.x)
    syncPivotField(pivotY, scene.pivot.y)
    syncPivotField(pivotZ, scene.pivot.z)
  }
  readFromEditor(null)
  // The callback writes Watcheds, so it cannot be a checked scope.subscribe.
  // sceneIndex belongs to the scope; the module observable needs a manual release.
  sceneIndex.subscribe_with_nasty_disregard_of_frp_update(readFromEditor)
  sceneIdMap.subscribe_with_nasty_disregard_of_frp_update(readFromEditor)
  scope.onDetach(@() sceneIdMap.unsubscribe(readFromEditor))

  function onPivotXChanged(val) {
    if (isStringFloat(val) && isStringFloat(pivotY.get()) && isStringFloat(pivotZ.get())) {
      entity_editor?.get_instance().setScenePivot(sceneIndex.get(),
        Point3(val.tofloat(), pivotY.get().tofloat(), pivotZ.get().tofloat()))
    }
  }

  function onPivotYChanged(val) {
    if (isStringFloat(val) && isStringFloat(pivotX.get()) && isStringFloat(pivotZ.get())) {
      entity_editor?.get_instance().setScenePivot(sceneIndex.get(),
        Point3(pivotX.get().tofloat(), val.tofloat(), pivotZ.get().tofloat()))
    }
  }

  function onPivotZChanged(val) {
    if (isStringFloat(val) && isStringFloat(pivotX.get()) && isStringFloat(pivotY.get())) {
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

  // textInput and mkCheckBox allocate their own state, so build them once.
  let controls = getPropertiesControls()

  return @() {
    flow = FLOW_HORIZONTAL
    halign = ALIGN_LEFT
    valign = ALIGN_CENTER
    watch = [sceneIndex]
    children = sceneIndex.get() != -1 ? controls : []
  }
})

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
          size = const [hdpx(46), hdpx(25)]
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
    size = const [SIZE_TO_CONTENT, flex()]
    watch = [canAddImport]
    children = textButton("Import", function() {
        addImportDialog(function(path) {
          let scene = singleMarkedImportScene(selection.get(), sceneIdMap.get())
          if (scene != null) {
            entity_editor?.get_instance().addImportScene(scene.id, path)
            expandedStateScenes.mutate(@(value) value[scene.id] <- true)
          }
        })
      },
      {
        off = !canAddImport.get(),
        disabled = Computed(@() !canAddImport.get() ),
        onHover = function(on) {
          local text = null
          if (on) {
            let scene = canAddImport.get() ? singleMarkedImportScene(selection.get(), sceneIdMap.get()) : null
            if (scene != null) {
              text = $"Add a new scene or import an existing one into {fileName(scene.path)}"
            }
            else {
              text = "Add a new scene or import an existing one\nFirst, select the target scene to import"
            }
          }
          setTooltip(text)
        },
        boxStyle = {
          normal = {
            size = const [SIZE_TO_CONTENT, flex()]
            margin = 0
            padding = const [hdpx(1),hdpx(10)]
          }
        }
        textStyle = {
          normal = {
            size = const [SIZE_TO_CONTENT, flex()]
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

  let isAnyFilterOptionEnabled = Computed(function() {
    return !showEntities.get() || !showClientScenes.get() || !showCommonScenes.get() || !showImportScenes.get() || !showOtherScenes.get()
  })

  function dropdownBgOverlay(onClick) {
    return {
      pos = const [-9000, -9000]
      size = const [19999, 19999]
      behavior = Behaviors.ComboPopup
      eventPassThrough = true
      onClick
    }
  }

  function popupWrapper(popupContent) {
    let children = [
      {size = const [flex(), ph(100)]}
      {size = const [flex(), hdpx(2)]}
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
      zOrder = getPopupZOrder()
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
    size = const [SIZE_TO_CONTENT, flex()]
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    watch = [dropDownOpen]
    fillColor = (sf & S_HOVER) ? Color(255, 255, 255, 255) : Color(0,0,0,64)
    onClick = @() toggleDropDown()
    onHover = @(on) setTooltip(on ? "Outliner filter settings" : null)

    children = @() {
      rendObj = ROBJ_IMAGE
      image = Picture(getIcon("filter_default"))
      size = const [hdpx(26), hdpx(26)]
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      watch = [dropDownOpen, isAnyFilterOptionEnabled]
      color = (sf & S_HOVER) || isAnyFilterOptionEnabled.get() ? Color(66, 176, 255) : Color(255, 255, 255, 255)
      children = dropDownOpen.get() ? dropDownList : null
    }
  })
}

// Built once: they own observables, and daRg reuses a child element only
// when the parent passes the same builder closure again.
let importButton = mkImportButton()
let filterOptionsButton = mkFilterOptionsButton()
let filterControls = createFilterControls()
let selectButton = createSelectButton()
let hideObjectsButton = createHideObjectsButton()
let lockObjectsButton = createLockObjectsButton()
let removeObjectsButton = createRemoveObjectsButton()

function mkScenesList() {

  function listSceneContent() {
    let items = filteredItems.get()
    let sRows = items.map(@(item, idx) SceneRow(item, idx, idx + 1 == items.len() || item.depth > items[idx + 1].depth))

    return {
      watch = filteredItems
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
    size = flex()
    children = [
      {
        size = FLEX_H
        flow = FLOW_HORIZONTAL
        gap = fsh(0.5)
        children = [
          importButton
          filter
          filterOptionsButton
        ]
      }
      {
        flow = FLOW_HORIZONTAL
        size = const [flex(), SIZE_TO_CONTENT]
        children = [
          filterControls
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
      ScenePropertiesControl()
      {
        flow = FLOW_HORIZONTAL
        size = FLEX_H
        halign = ALIGN_CENTER
        gap = fsh(0.5)
        children = [
          selectButton
          hideObjectsButton
          lockObjectsButton
          removeObjectsButton
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
  mkContent = mkScenesList
  saveState=true
  headerText = "Scene Outliner"
}
