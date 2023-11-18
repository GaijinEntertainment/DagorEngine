from "%darg/ui_imports.nut" import *
from "math" import sin, cos, PI

let cursors = require("samples_prog/_cursors.nut")


function generateHexGridMesh(settings) {
  let {gridCellSizeX, gridCellSizeY, gridColumns, gridRaws, gridLineWidth, gridLineColor, gridCellDisplacement} = settings

  const gridNodeStep = 6
  local gridVertices = []
  local gridQuadIndices = []

  function addHexNode(ix, iy) {
    local centerX = ix * gridCellSizeX + (((ix ^ iy) & 1) ? -gridCellDisplacement : gridCellDisplacement) * gridCellSizeX
    local centerY = iy * gridCellSizeY

    for (local i = 0; i < 6; i++) {
      local angle = (i - 2) * PI * 60.0 / 180.0
      local x = centerX + gridLineWidth * cos(angle)
      local y = centerY + gridLineWidth * sin(angle)
      gridVertices.append(x, y, gridLineColor)
    }
  }

  function addHexNodeIndices(ix, iy) {
    local i = (iy * (gridColumns + 1) + ix) * gridNodeStep
    gridQuadIndices.append(i + 0, i + 1, i + 2, i + 5, i + 5, i + 2, i + 3, i + 4);
  }

  function addNodeConnectionIndices(ix0, iy0, ix1, iy1) {
    local i0 = (iy0 * (gridColumns + 1) + ix0) * gridNodeStep
    local i1 = (iy1 * (gridColumns + 1) + ix1) * gridNodeStep
    if (iy0 == iy1 && ix0 < ix1)
      gridQuadIndices.append(i0 + 1, i1 + 0, i1 + 5, i0 + 2, i0 + 2, i1 + 5, i1 + 4, i0 + 3)

    if (ix0 == ix1 && iy0 < iy1 && ((ix0 ^ iy0) & 1) != 0)
      gridQuadIndices.append(i0 + 2, i1 + 1, i1 + 0, i0 + 3, i0 + 3, i1 + 0, i1 + 5, i0 + 4)

    if (ix0 == ix1 && iy0 < iy1 && ((ix0 ^ iy0) & 1) == 0)
      gridQuadIndices.append(i0 + 4, i1 + 1, i1 + 0, i0 + 5, i0 + 4, i0 + 3, i1 + 2, i1 + 1)
  }

  for (local iy = 0; iy <= gridRaws; iy++) {
    for (local ix = 0; ix <= gridColumns; ix++) {
      addHexNode(ix, iy)
      addHexNodeIndices(ix, iy)

      if (ix < gridColumns && ((ix ^ iy) & 1) == 0)
        addNodeConnectionIndices(ix, iy, ix + 1, iy)

      if (iy < gridRaws)
        addNodeConnectionIndices(ix, iy, ix, iy + 1)
    }
  }

  return [VECTOR_QUADS, gridVertices, gridQuadIndices]
}


function generateHexGridMeshAA(settings) {
  let {gridCellSizeX, gridCellSizeY, gridColumns, gridRaws, gridLineWidth, gridLineColor, gridCellDisplacement,
    gridAAWidth} = settings

  const gridNodeStep = 12
  local gridVertices = []
  local gridQuadIndices = []

  function addHexNode(ix, iy) {
    local centerX = ix * gridCellSizeX + (((ix ^ iy) & 1) ? -gridCellDisplacement : gridCellDisplacement) * gridCellSizeX
    local centerY = iy * gridCellSizeY

    for (local i = 0; i < 6; i++) {
      local angle = (i - 2) * PI * 60.0 / 180.0
      local x = centerX + gridLineWidth * cos(angle)
      local y = centerY + gridLineWidth * sin(angle)
      gridVertices.append(x, y, gridLineColor)
    }

    for (local i = 0; i < 6; i++) {
      local angle = (i - 2) * PI * 60.0 / 180.0
      local x = centerX + (gridLineWidth + gridAAWidth) * cos(angle)
      local y = centerY + (gridLineWidth + gridAAWidth) * sin(angle)
      gridVertices.append(x, y, Color(0, 0, 0, 0))
    }
  }

  function addHexNodeIndices(ix, iy) {
    local i = (iy * (gridColumns + 1) + ix) * gridNodeStep
    gridQuadIndices.append(i + 0, i + 1, i + 2, i + 5, i + 5, i + 2, i + 3, i + 4);

    // antialiasing
    if (iy == 0) {
      gridQuadIndices.append(i + 0, i + 1, i + 7, i + 6)
      if ((ix & 1) != 0)
        gridQuadIndices.append(i + 1, i + 2, i + 8, i + 7)
      else
        gridQuadIndices.append(i + 5, i + 0, i + 6, i + 11)
    }

    if (iy == gridRaws) {
      gridQuadIndices.append(i + 3, i + 4, i + 10, i + 9)
      if (((ix ^ iy) & 1) == 0)
        gridQuadIndices.append(i + 4, i + 5, i + 11, i + 10)
      else
        gridQuadIndices.append(i + 2, i + 3, i + 9, i + 8)
    }

    if (ix == 0 && (iy & 1) != 0) {
      gridQuadIndices.append(i + 0, i + 6, i + 11, i + 5)
      gridQuadIndices.append(i + 5, i + 11, i + 10, i + 4)
    }

    if (ix == gridColumns && ((iy ^ ix) & 1) == 0) {
      gridQuadIndices.append(i + 1, i + 2, i + 8, i + 7)
      gridQuadIndices.append(i + 2, i + 8, i + 9, i + 3)
    }
  }

  function addNodeConnectionIndices(ix0, iy0, ix1, iy1) {
    local i0 = (iy0 * (gridColumns + 1) + ix0) * gridNodeStep
    local i1 = (iy1 * (gridColumns + 1) + ix1) * gridNodeStep
    if (iy0 == iy1 && ix0 < ix1) {
      gridQuadIndices.append(i0 + 1, i1 + 0, i1 + 5, i0 + 2, i0 + 2, i1 + 5, i1 + 4, i0 + 3)
      // antialiasing
      gridQuadIndices.append(i0 + 1, i0 + 7, i1 + 6, i1 + 0)
      gridQuadIndices.append(i0 + 3, i0 + 9, i1 + 10, i1 + 4)
    }

    if (ix0 == ix1 && iy0 < iy1 && ((ix0 ^ iy0) & 1) != 0) {
      gridQuadIndices.append(i0 + 2, i1 + 1, i1 + 0, i0 + 3, i0 + 3, i1 + 0, i1 + 5, i0 + 4)
      // antialiasing
      gridQuadIndices.append(i0 + 8, i1 + 7, i1 + 1, i0 + 2)
      gridQuadIndices.append(i0 + 4, i1 + 5, i1 + 11, i0 + 10)
    }

    if (ix0 == ix1 && iy0 < iy1 && ((ix0 ^ iy0) & 1) == 0) {
      gridQuadIndices.append(i0 + 4, i1 + 1, i1 + 0, i0 + 5, i0 + 4, i0 + 3, i1 + 2, i1 + 1)
      // antialiasing
      gridQuadIndices.append(i0 + 9, i0 + 3, i1 + 2, i1 + 8)
      gridQuadIndices.append(i0 + 5, i0 + 11, i1 + 6, i1 + 0)
    }
  }

  for (local iy = 0; iy <= gridRaws; iy++) {
    for (local ix = 0; ix <= gridColumns; ix++) {
      addHexNode(ix, iy)
      addHexNodeIndices(ix, iy)

      if (ix < gridColumns && ((ix ^ iy) & 1) == 0)
        addNodeConnectionIndices(ix, iy, ix + 1, iy)

      if (iy < gridRaws)
        addNodeConnectionIndices(ix, iy, ix, iy + 1)
    }
  }

  return [VECTOR_QUADS, gridVertices, gridQuadIndices]
}



let vectorCanvasVerticesIndices = {
  rendObj = ROBJ_VECTOR_CANVAS
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = [hdpx(300), hdpx(300)]
  lineWidth = hdpx(2.5)
  color = Color(50, 200, 255)
  fillColor = Color(255, 120, 100, 0)
  commands = [
    [VECTOR_COLOR, Color(255, 255, 255)],  // set stroke color = white
    [VECTOR_TM_OFFSET, 0, -100],
    generateHexGridMesh({
      gridCellSizeX = 22
      gridCellSizeY = 15
      gridCellDisplacement = 0.2
      gridColumns = 5
      gridRaws = 5
      gridLineWidth = 2.0
      gridLineColor = Color(255, 255, 255)
      gridAAWidth = 0.5
    }),

    [VECTOR_TM_OFFSET, 0, 100],
    generateHexGridMeshAA({
      gridCellSizeX = 22
      gridCellSizeY = 15
      gridCellDisplacement = 0.2
      gridColumns = 5
      gridRaws = 5
      gridLineWidth = 2.0
      gridLineColor = Color(255, 255, 255)
      gridAAWidth = 0.4
    })

  ]
}


let function basicsRoot() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(30, 40, 50)
    cursor = cursors.normal
    size = flex()
    padding = 50
    children = [
      {
        valign = ALIGN_CENTER
        halign = ALIGN_CENTER
        flow = FLOW_VERTICAL
        size = flex()
        gap = 40
        children = [vectorCanvasVerticesIndices]
      }
    ]
  }
}

return basicsRoot
