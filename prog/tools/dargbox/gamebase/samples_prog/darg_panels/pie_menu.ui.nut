from "%darg/ui_imports.nut" import *

let {IPoint2, Point2, Point3} = require("dagor.math")

let mkPieMenu = require("samples_prog/advanced/mkPieMenu.nut")
let math = require("math")



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



function isCurrent(curIdx,i) {
  return curIdx==i
}

let m = @(curIdx, idx, sf) (sf & S_HOVER) || isCurrent(curIdx, idx) ? 2:1

function s(idx){
  let c = Color(math.rand(),math.rand(),math.rand(),255)
  let onSelect = @() dlog($"{idx} selected")
  return {
    ctor=function(curIdx, idx){
      let stateFlags = Watched(0)
      return function(){
        let sf = stateFlags.value
        return {
          watch=[curIdx, stateFlags],
          rendObj=ROBJ_SOLID
          size = [hdpx(120)*m(curIdx.value, idx, sf),hdpx(120)*m(curIdx.value, idx, sf)]
          color = c
          children = {
            rendObj = ROBJ_TEXT
            fontSize = 60
            text = idx
            color = (sf & S_HOVER) || isCurrent(curIdx.value, idx) ? Color(250,250,50) : Color(200,200,200)
            hplace = ALIGN_CENTER
            vplace = ALIGN_CENTER
          }
          behavior = Behaviors.Button
          onHover = @(on) on ? vlog($"on {idx}") : null
          onElemState = @(sf) stateFlags(sf)
          onClick = onSelect
        }
      }
    }
    onSelect=onSelect
    text = $"action number {idx}"
  }
}

let radius = hdpx(512)
let sectorWidth = 1.0/2
let sangle = 15
//local showPieMenu = Watched(false)
let pieMenu = mkPieMenu({
//  hotkeys = [["J:D.Up", @() showPieMenu(!showPieMenu.value)]]
  hotkeyToSelect = "J:D.Down"
//  hotkeys = "J:D.Up"
//  showPieMenu = showPieMenu,
  radius = radius
  objs = array(6).map(@(_,i) s(i))
  //devId = DEVID_JOYSTICK
  devId = DEVID_VR
  stickNo = 1
  _back = @(){}
  _sectorCtor = @() {
    rendObj = ROBJ_VECTOR_CANVAS
    color = Color(0,0,0,180)
//    fillColor = selectedFillColor
    size = [radius*2,radius*2]
    commands = [
//      [VECTOR_COLOR, selectedBorderColor],
      [VECTOR_FILL_COLOR, Color(0, 0, 0, 0)],
      [VECTOR_WIDTH, radius*sectorWidth*2],
      [VECTOR_SECTOR, 50, 50, 50*sectorWidth*3/2, 50*sectorWidth*3/2, -sangle-90, sangle-90],
//      [VECTOR_COLOR, selectedBorderColor],
//      [VECTOR_WIDTH, hdpx(5)],
//      [VECTOR_SECTOR, 50, 50, 50, 50, -sangle-90, sangle-90],
    ]
  }

})


let freePanelLayout = {
  worldAnchor   = PANEL_ANCHOR_SCENE
  worldGeometry = PANEL_GEOMETRY_RECTANGLE
  worldOffset   = Point3(0, 0, 5)
  worldAngles   = Point3(0, 20, 0)
  worldSize     = Point2(1, 1)
  canvasSize    = IPoint2(1024, 1024)

  worldBrightness = 1

  worldCanBePointedAt = true
  //worldPointerTexture = "panel_cursor"
  cursor         = panelCursor

  size           = [1024, 1024]

  flow           = FLOW_VERTICAL

  children = [
    {
      size           = flex()
      rendObj        = ROBJ_SOLID
      color          = Color(50,200,50)
      halign         = ALIGN_CENTER
      valign         = ALIGN_CENTER

      stopMouse      = true
      children       = pieMenu
    }
  ]
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
