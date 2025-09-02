from "%darg/ui_imports.nut" import *
from "%sqstd/ecs.nut" import *

const SORT_BY_LOADTYPE = " LOAD "
const SORT_BY_PATHS = " PATH "
const SORT_BY_ENTITIES  = " ENT# "

function sortScenesByLoadType(scene1, scene2) {
  return scene1.loadType != scene2.loadType ?
    scene2.loadType <=> scene1.loadType : scene1.index <=> scene2.index
}

function sortScenesByEntities(scene1, scene2) {
  return scene1.entityCount <=> scene2.entityCount
}

function sortScenesByPaths(scene1, scene2) {
  let path1 = scene1.path
  let path2 = scene2.path
  if (path1 != null && path2 != null)
    return path1 <=> path2
  return path1 != null ? 1 : -1
}

let defaultScenesSortMode = {
  mode = SORT_BY_LOADTYPE
  func = sortScenesByLoadType
}

function toggleScenesSortMode(state) {
  let sortMode = state.get()?.mode ?? SORT_BY_LOADTYPE
  if (sortMode == SORT_BY_LOADTYPE)
    state.set({
      mode = SORT_BY_PATHS
      func = sortScenesByPaths
    })
  else if (sortMode == SORT_BY_PATHS)
    state.set({
      mode = SORT_BY_ENTITIES
      func = sortScenesByEntities
    })
  else
    state.set(defaultScenesSortMode)
}

function mkSceneSortModeButton(state, style={}) {
  let stateFlags = Watched(0)
  let fillColor = style?.fillColor ?? Color(0,0,0,64)
  return @() {
    rendObj = ROBJ_BOX
    size = SIZE_TO_CONTENT
    watch = [state, stateFlags]
    fillColor = (stateFlags.get() & S_TOP_HOVER) ? Color(255,255,255,255) : fillColor
    borderRadius = hdpx(4)
    behavior = Behaviors.Button

    onClick = @() toggleScenesSortMode(state)
    onElemState = @(sf) stateFlags.set(sf & S_TOP_HOVER)

    children = {
      rendObj = ROBJ_TEXT
      text = state.get()?.mode ?? SORT_BY_LOADTYPE
      color = (stateFlags.get() & S_TOP_HOVER) ? Color(0,0,0) : Color(200,200,200)
      margin = fsh(0.5)
    }
  }
}


return {
  defaultScenesSortMode
  mkSceneSortModeButton
}
