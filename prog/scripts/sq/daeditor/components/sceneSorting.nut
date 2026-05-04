from "%darg/ui_imports.nut" import *

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

return {
  sortScenesByLoadType
  sortScenesByEntities
  sortScenesByPaths
}
