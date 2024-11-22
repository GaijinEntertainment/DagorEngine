from "%darg/ui_imports.nut" import *

let {IPoint2, Point2, Point3} = require("dagor.math")
let {makeHVScrolls} = require("samples_prog/_basic/components/scrollbar.nut")


let panelCursor = Cursor({
  rendObj = ROBJ_VECTOR_CANVAS
  size = [24, 24]
  hotspot = [12, 12]

  commands = [
    [VECTOR_WIDTH, 3],
    [VECTOR_FILL_COLOR, Color(160,0,0,160)],
    [VECTOR_COLOR, Color(200, 200, 200, 200)],
    [VECTOR_ELLIPSE, 50, 50, 50, 50],
  ]
})


function makeCells() {
  let colors = [
    Color(200,30,20), Color(20,200,30), Color(0,30,200),
    Color(180,160,20), Color(150,30,170), Color(50,140,160),
    Color(140,160,180)
  ]


  let rows = []
  local colorIdx = 0
  for (local i=0; i<10; ++i) {
    let cols = []
    for (local j=0; j<16; ++j, ++colorIdx) {
      cols.append({
        rendObj = ROBJ_SOLID
        size = [sh(17), sh(17)]
        color = colors[colorIdx % colors.len()]
      })
    }
    rows.append({
      flow = FLOW_HORIZONTAL
      size = SIZE_TO_CONTENT
      children = cols
    })
  }

  let pannable = {
    size = SIZE_TO_CONTENT
    flow = FLOW_VERTICAL
    children = rows
  }

  let container = {
    size = flex()
    rendObj = ROBJ_FRAME
    color = Color(220, 220, 0)
    borderWidth = 4
    padding = 4

    children = makeHVScrolls(pannable, {behaviors = [Behaviors.Pannable]})
  }

  return container
}


let freePanelLayout = {
  worldAnchor   = PANEL_ANCHOR_SCENE
  worldGeometry = PANEL_GEOMETRY_RECTANGLE
  worldOffset   = Point3(0, 0, 5)
  worldAngles   = Point3(0, 20, 0)
  worldSize     = Point2(1, 1)
  canvasSize    = IPoint2(512, 640)

  worldBrightness = 1

  worldCanBePointedAt = true
  cursor         = panelCursor

  size           = [512, 512]

  flow           = FLOW_VERTICAL

  children = makeCells()
}


function root() {
  return {
    size = flex()
    function onAttach() {
      gui_scene.addPanel(14, freePanelLayout)
    }
    function onDetach() {
      gui_scene.removePanel(14)
    }
  }
}


return root
