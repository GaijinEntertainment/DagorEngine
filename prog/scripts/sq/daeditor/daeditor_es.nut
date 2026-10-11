from "%sqstd/ecs.nut" import *
from "%darg/ui_imports.nut" import *
from "types" import Integer

let entity_editor = require_optional("entity_editor")
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

function getScenePrettyName(index) {
  return entity_editor?.get_instance().getScenePrettyName(index) ?? ""
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

  getScenePrettyName
  isEntityInLockedHierarchy
}
