from "%sqstd/ecs.nut" import *
from "%darg/ui_imports.nut" import *
from "types" import Integer

let entity_editor = require_optional("entity_editor")
let { selectedEntity, selectedEntities, selectedEntitiesSetKeyVal, selectedEntitiesDeleteKey,
      selectedCompName, markedScenes, sceneIdMap } = require("state.nut")
let { fileName } = require("%sqstd/path.nut")
let ecs = require("%sqstd/ecs.nut")

// DO NOT add extra components here as this code is not game specific.
// if you want extra entity info, use DAS game specific code instead
// Example:
//
// require ecs.ecs_quirrel
// [quirrel_bind(module_name="das.daeditor")]
// def get_entity_extra_name(eid : EntityId)
//   return "{eid}"
let defaultGetEntityExtraNameQuery = SqQuery("defaultGetEntityExtraNameQuery", {
  comps_ro = [["ri_extra__name", TYPE_STRING, null]]
})

let {
  get_entity_extra_name = @(eid) defaultGetEntityExtraNameQuery(eid, @(_eid, comp) comp.ri_extra__name)
} = require_optional("das.daeditor")


selectedEntities.subscribe_with_nasty_disregard_of_frp_update(function(val) {
  if (val.len() == 1)
    selectedEntity.set(val.keys()[0])
  else
    selectedEntity.set(INVALID_ENTITY_ID)
})

selectedEntity.subscribe_with_nasty_disregard_of_frp_update(function(_eid) {
  selectedCompName.set(null)
})

register_es("update_selected_entities", {
    onInit = @(eid, _comp) selectedEntitiesSetKeyVal(eid, true)
    onDestroy = @(eid, _comp) selectedEntitiesDeleteKey(eid)
  },
  { comps_rq = ["daeditor__selected"]}
)

function getEntityExtraName(eid): string|null {
  let extraName = get_entity_extra_name?(eid) ?? ""

  return extraName.strip() == "" ? null : extraName
}

function getSceneLoadTypeText(v): string {
  let loadTypeVal = v instanceof Integer ? v : v.loadType
  let loadType = (
    (loadTypeVal == 1) ? "COMMON" :
    (loadTypeVal == 2) ? "CLIENT" :
    (loadTypeVal == 3) ? "IMPORT" :
    "UNKNOWN"
  )
  return loadType
}

function getNumMarkedScenes(): int {
  local nSel = 0
  foreach (_sceneId, marked in markedScenes.get()) {
    if (marked)
      ++nSel
  }
  return nSel
}

function matchEntityByScene(eid) {
  local id = entity_editor?.get_instance().getEntityRecordSceneId(eid)
  if (markedScenes.get()?[id])
    return true
  return entity_editor?.get_instance().isSceneEntity(eid)
}

function getScenePrettyName(index) {
  return entity_editor?.get_instance().getScenePrettyName(index) ?? ""
}

function sceneToComboboxEntry(scene): string {
  if (scene.importDepth == 0 && !scene.hasParent) {
    return "MAIN"
  }

  local prettyName = getScenePrettyName(scene.id)
  local strippedPath = fileName(scene.path)
  local loadType = getSceneLoadTypeText(scene)
  return $"{loadType}:{scene.id}:{prettyName.len() == 0 ? strippedPath : $"{prettyName} ({strippedPath})"}"
}

function canSceneBeModified(scene): bool {
  if (scene == null) {
    return false
  }

  while (scene?.loadType != null) {
    if (scene.loadType != 3 || (scene.importDepth != 0 && !entity_editor?.get_instance().isChildScene(scene.id))) {
      return false
    }

    if (entity_editor?.get_instance()?.isSceneInLockedHierarchy(scene.id)) {
      return false
    }

    scene = sceneIdMap?.get()[scene.parent]
  }

  return true
}

function isEntityInLockedHierarchy(eid) {
  if (entity_editor?.get_instance()?.isEntityLocked(eid) ?? false) {
    return true
  }

  let sceneId = entity_editor?.get_instance()?.getEntityRecordSceneId(eid) ?? ecs.INVALID_SCENE_ID
  if (sceneId == ecs.INVALID_SCENE_ID) {
    return false
  }

  return entity_editor?.get_instance()?.isSceneInLockedHierarchy(sceneId) ?? false
}

return {
  getEntityExtraName

  getSceneLoadTypeText

  getNumMarkedScenes
  matchEntityByScene

  getScenePrettyName
  sceneToComboboxEntry
  canSceneBeModified
  isEntityInLockedHierarchy
}
