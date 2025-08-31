from "dagor.math" import IPoint2, Point2, Point3
from "%darg/ui_imports.nut" import *
let panelCursor = Cursor({
  rendObj = ROBJ_VECTOR_CANVAS
  size = 24
  hotspot = [12, 12]

  commands = [
    [VECTOR_WIDTH, 3],
    [VECTOR_FILL_COLOR, Color(160,0,0,160)],
    [VECTOR_COLOR, Color(200, 200, 200, 200)],
    [VECTOR_ELLIPSE, 50, 50, 50, 50],
  ]
})


let buttonCursor = Cursor({
  rendObj = ROBJ_VECTOR_CANVAS
  size = 24
  hotspot = [12, 12]

  transform = {}
  animations = [
    { prop=AnimProp.rotate, from = 0, to = 360, duration = 1, play = true, loop = true }
  ]


  commands = [
    [VECTOR_WIDTH, 3],
    [VECTOR_FILL_COLOR, Color(0,0,160,160)],
    [VECTOR_COLOR, Color(200, 200, 200, 200)],
    [VECTOR_RECTANGLE, 0, 0, 100, 100],
  ]
})


let button1 = watchElemState( @(sf) {
  rendObj = ROBJ_BOX
  padding = 20
  fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
  borderWidth = (sf & S_HOVER) ? 2 : 0
  behavior = Behaviors.Button
  halign = ALIGN_CENTER
  onClick = function() { vlog("clicked")}
  onHover = function(hover) {vlog($"hover: {hover}")}
  children = {
    rendObj = ROBJ_TEXT
    text = (sf & S_ACTIVE) ? "pressed": (sf & S_HOVER) ? "hover" : "button"
    color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0)
    pos = (sf & S_ACTIVE) ? [0,2] : [0,0]
    fontSize = 30
  }

  cursor = buttonCursor

  sound = {
    click  = "ui/button_click"
    hover  = "ui/menu_highlight"
    active = "ui/button_action"
  }
})


let freePanelLayout = {
  worldAnchor   = PANEL_ANCHOR_SCENE
  worldGeometry = PANEL_GEOMETRY_RECTANGLE
  worldOffset   = Point3(0, 0, 5)
  worldAngles   = Point3(0, 20, 0)
  worldSize     = Point2(1, 1)
  canvasSize    = IPoint2(512, 640)

  worldBrightness = 1

  worldCanBePointedAt = true
  //worldPointerTexture = "panel_cursor"
  cursor         = panelCursor

  size           = static [512, 640]

  flow           = FLOW_VERTICAL

  children = [
    {
      size           = 512
      rendObj        = ROBJ_SOLID
      color          = Color(50,200,50)
      halign         = ALIGN_CENTER
      valign         = ALIGN_CENTER

      flow           = FLOW_VERTICAL
      gap            = 20

      stopMouse      = true

      children = [
        {
          rendObj       = ROBJ_TEXTAREA
          color         = Color(20,20,20)
          textOverflowX = TOVERFLOW_CLIP
          textOverflowY = TOVERFLOW_LINE
          text          = "This is a\nfree floating\npanel, positioned\nin 3d space"
          behavior      = Behaviors.TextArea
          fontSize      = 60
        }
        button1
      ]
    }
    {
      size = static [512, 128]
      rendObj = ROBJ_TEXT
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      fontSize = 40
      text = "Input-transparent panel part"
    }
  ]
}


function root() {
  return {
    size = flex()
    function onAttach() {
      gui_scene.addPanel(15, freePanelLayout)
    }
    function onDetach() {
      gui_scene.removePanel(15)
    }
  }
}


return root
