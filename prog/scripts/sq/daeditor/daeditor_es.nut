from "%sqstd/ecs.nut" import *
from "state.nut" import selectedEntity, selectedEntities, selectedEntitiesSetKeyVal, selectedEntitiesDeleteKey, selectedCompName
from "%darg/ui_imports.nut" import *

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


selectedEntities.subscribe(function(val) {
  if (val.len() == 1)
    selectedEntity(val.keys()[0])
  else
    selectedEntity(INVALID_ENTITY_ID)
})

selectedEntity.subscribe(function(_eid) {
  selectedCompName(null)
})

register_es("update_selected_entities", {
    onInit = @(eid, _comp) selectedEntitiesSetKeyVal(eid, true)
    onDestroy = @(eid, _comp) selectedEntitiesDeleteKey(eid)
  },
  { comps_rq = ["daeditor__selected"]}
)

function getEntityExtraName(eid) {
  let extraName = get_entity_extra_name?(eid) ?? ""

  return extraName.strip() == "" ? null : extraName
}

return {
  getEntityExtraName
}