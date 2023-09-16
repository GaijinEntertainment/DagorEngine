from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let mkPieMenu = require("mkPieMenu.nut")
let math = require("math")

let function isCurrent(curIdx,i){
  return curIdx==i
}
let m = @(curIdx, idx, sf) (sf & S_HOVER) || isCurrent(curIdx, idx) ? 2:1
let function s(idx){
  let c = Color(math.rand(),math.rand(),math.rand(),255)
  let onSelect = @() dlog($"{idx} do it")
  return {
    ctor=function(curIdx, idx){
      let stateFlags = Watched(0)
      return function(){
        let sf = stateFlags.value
        return {
          watch=[curIdx, stateFlags],
          rendObj=ROBJ_SOLID
          size = [hdpx(60)*m(curIdx.value, idx, sf),hdpx(60)*m(curIdx.value, idx, sf)]
          color = c
          children = {rendObj = ROBJ_TEXT text = $"long text {idx}", color = (sf & S_HOVER) || isCurrent(curIdx.value, idx) ? Color(200,200,200) : Color(50,50,50) hplace = ALIGN_CENTER vplace = ALIGN_CENTER}
          behavior = Behaviors.Button
          onHover = @(on) vlog($"on={on} {idx}")
          onElemState = @(sf) stateFlags(sf)
          onClick = onSelect

        }
      }
    }
    onSelect=onSelect
    text = $"action number {idx}"
  }
}

let radius = hdpx(350)
let sectorWidth = 1.0/2
let sangle = 15
//let selectedBorderColor = Color(200,200,200)
//local showPieMenu = Watched(false)
let pieMenu = mkPieMenu({
//  hotkeys = [["J:D.Up", @() showPieMenu(!showPieMenu.value)]]
  hotkeyToSelect = "J:D.Down"
//  hotkeys = "J:D.Up"
//  showPieMenu = showPieMenu,
  radius = radius
  objs = array(6).map(@(_,i) s(i))
  //devId = DEVID_JOYSTICK
  devId = DEVID_MOUSE
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

let function root() {
  return {
    key = "showPieMenu"
    hplace = ALIGN_CENTER
    vplace = ALIGN_CENTER
    size = flex()
    rendObj = ROBJ_SOLID
    color = Color(90,40,40)
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    cursor = cursors.normal

    children = [
      pieMenu
    ]
  }
}

return root
