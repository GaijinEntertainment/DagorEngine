from "%darg/ui_imports.nut" import *
from "%sqstd/ecs.nut" import *

const SORT_BY_INDEX = " # "
const SORT_BY_NAMES = " ABC "
const SORT_BY_EIDS  = " EID "

function sortEntityByEid(eid1, eid2) {
  return eid1 <=> eid2
}

function sortEntityByNames(eid1, eid2) {
  let tplName1 = g_entity_mgr.getEntityTemplateName(eid1)
  let tplName2 = g_entity_mgr.getEntityTemplateName(eid2)
  if (tplName1 < tplName2)
    return -1
  if (tplName1 > tplName2)
    return 1
  let riExtraName1 = obsolete_dbg_get_comp_val(eid1, "ri_extra__name")
  let riExtraName2 = obsolete_dbg_get_comp_val(eid2, "ri_extra__name")
  if (riExtraName1 != null && riExtraName2 != null)
    return riExtraName1 <=> riExtraName2
  return eid1 <=> eid2
}

function toggleSortMode(state) {
  let sortMode = state.value?.mode ?? SORT_BY_INDEX
  if (sortMode == SORT_BY_INDEX)
    state({
      mode = SORT_BY_NAMES
      func = sortEntityByNames
    })
  else if (sortMode == SORT_BY_NAMES)
    state({
      mode = SORT_BY_EIDS
      func = sortEntityByEid
    })
  else
    state({
      mode = SORT_BY_INDEX
      func = null
    })
}

function mkSortModeButton(state, style={}) {
  let stateFlags = Watched(0)
  let fillColor = style?.fillColor ?? Color(0,0,0,64)
  return @() {
    rendObj = ROBJ_BOX
    size = SIZE_TO_CONTENT
    watch = [state, stateFlags]
    fillColor = (stateFlags.value & S_TOP_HOVER) ? Color(255,255,255,255) : fillColor
    borderRadius = hdpx(4)
    behavior = Behaviors.Button

    onClick = @() toggleSortMode(state)
    onElemState = @(sf) stateFlags.update(sf & S_TOP_HOVER)

    children = {
      rendObj = ROBJ_TEXT
      text = state.value?.mode ?? SORT_BY_INDEX
      color = (stateFlags.value & S_TOP_HOVER) ? Color(0,0,0) : Color(200,200,200)
      margin = fsh(0.5)
    }
  }
}


return mkSortModeButton
