from "%darg/ui_imports.nut" import *
from "math" import PI, sin, cos


let function place_by_circle(params) {
  let objs = params?.objects ?? []
  let radius = params?.radius ?? hdpx(100)
  let offset = params?.offset ?? (3.0/4)
  let seg_angle = 2.0*PI/objs.len()

  return objs.map(function(o,i) {
    let angle = seg_angle*i-PI/2
    let pos = [cos(angle)*radius*offset+radius, sin(angle)*radius*offset+radius]
    return {
      pos
      halign = ALIGN_CENTER
      valign=ALIGN_CENTER
      size=[0,0]
      rendObj=ROBJ_TEXT
      text=",".concat(pos[0],pos[1])
      children = o
    }
  })
}

let selectedBorderColor = Color(85, 85, 85, 100)
let selectedBgColor = Color(0, 0, 0, 120)
let sectorWidth = 1.0/2

let function mDefCtor(text) {
  return function (curIdx, idx) {
    return watchElemState(function(sf) {
      return {
        rendObj = ROBJ_TEXT
        text = text
        color = (curIdx.value==idx || (S_HOVER & sf)) ? Color(250,250,250) : Color(120,120,120)
      }
    })
  }
}

let defParams = {
  objs=[], radius=hdpx(250) back=null, axisXY=[],
  sectorCtor = null, nullSectorCtor = null,
  eventHandlers= null, hotkeys = null
  devId = DEVID_MOUSE, stickNo = 1
}


let function makeDefaultBack(radius, size) {
  return {
    rendObj = ROBJ_VECTOR_CANVAS
    color = selectedBgColor
    size = size
    commands = [
      [VECTOR_WIDTH, sectorWidth*radius],
      [VECTOR_FILL_COLOR, Color(0, 0, 0, 0)],
      [VECTOR_ELLIPSE, 50, 50, 50*sectorWidth*3/2, 50*sectorWidth*3/2],
      [VECTOR_WIDTH, hdpx(5)],
      [VECTOR_COLOR, selectedBorderColor],
      [VECTOR_FILL_COLOR, 0],
      [VECTOR_ELLIPSE, 50, 50, 50*(1-sectorWidth), 50*(1-sectorWidth)],
//      [VECTOR_ELLIPSE, 50, 50, 50, 50]
    ]
  }
}

let function makeDefaultSector(size, radius, sangle) {
  return {
    rendObj = ROBJ_VECTOR_CANVAS
    color = selectedBgColor
//    fillColor = selectedFillColor
    size = size
    commands = [
//      [VECTOR_COLOR, selectedBorderColor],
      [VECTOR_FILL_COLOR, Color(0, 0, 0, 0)],
      [VECTOR_WIDTH, radius*sectorWidth],
      [VECTOR_SECTOR, 50, 50, 50*sectorWidth*3/2, 50*sectorWidth*3/2, -sangle-90, sangle-90],
      [VECTOR_COLOR, selectedBorderColor],
      [VECTOR_WIDTH, hdpx(5)],
      [VECTOR_SECTOR, 50, 50, 50, 50, -sangle-90, sangle-90],
    ]
  }
}


let function mkPieMenu(params=defParams){
  let radius = params?.radius ?? hdpx(250)
  let objs = params?.objs ?? []
  let objsnum = objs.len()
  let size = [radius*2, radius*2]
  let back = params?.back?() ?? makeDefaultBack(radius, size)
  if (objsnum==0)
    return back
  let sangle = 360.0 / objsnum/2

  let internalIdx = Watched(null)
  let curIdx = params?.curIdx ?? Watched(null)
  let curAngle = Watched(null)
  internalIdx.subscribe(function(v) { if (v != null) curIdx(v)})
  let children = place_by_circle({
    radius=radius, objects=objs.map(@(v, i) v?.ctor?(curIdx, i) ?? mDefCtor(v?.text)(curIdx, i) ), offset=3.0/4
  })
  let sector = params?.sectorCtor?() ?? makeDefaultSector(size, radius, sangle)

  let function angle() {
    return {
      rendObj = ROBJ_VECTOR_CANVAS
      color = Color(0,0,0,180)
      transform = {
        pivot = [0.5,0.5]
        rotate = (curAngle.value ?? 0.0)*180.0/PI
      }
      size
      watch = curAngle
      commands = curAngle.value!=null
      ? [
          [VECTOR_FILL_COLOR, Color(0, 0, 0, 0)],
          [VECTOR_WIDTH, hdpx(6)],
          [VECTOR_COLOR, Color(250,250,250)],
          [VECTOR_SECTOR, 50, 50, 50*sectorWidth*0.95, 50*sectorWidth*0.95, -sangle-90, sangle-90],
        ]
      : []
    }
  }

  let nullSector = params?.nullSectorCtor?() ?? {
    rendObj = ROBJ_VECTOR_CANVAS
    color = selectedBorderColor
    fillColor = Color(0,0,0,50)
    size
    commands = [
      [VECTOR_WIDTH, hdpx(5)],
      [VECTOR_ELLIPSE, 50, 50, 50*(1-sectorWidth), 50*(1-sectorWidth)]
    ]
  }

  let function activeSector() {
    return {
      watch = [curIdx]
      size
      children = [
        objs?[curIdx.value] != null ? sector : null,
        curIdx.value != null ? nullSector : null,
      ]
      transform = {
        rotate = (curIdx.value ?? 0) * 2*sangle
        pivot = [0.5,0.5]
      }
    }
  }

  let function pieMenu() {
    return {
      size
      watch = curIdx
      behavior = Behaviors.PieMenu
      skipDirPadNav = true
      devId = params?.devId ?? defParams.devId
      stickNo = params?.stickNo ?? defParams.stickNo
      curSector = internalIdx
      curAngle
      sectorsCount = objsnum
      onClick = @(idx) params?.onClick?(idx)
      onDetach = params?.onDetach
      children = [back, activeSector, angle].extend(params?.children ?? []).extend(children)
    }
  }

  return pieMenu
}


let function mkPieMenuActivator(params = defParams){
  params = defParams.__merge(params)
  local hotkeys = params?.hotkeys
  let showPieMenu = params?.showPieMenu ?? Watched(hotkeys==null)
  let pieMenu = mkPieMenu(params)
  let eventHandlers = params?.eventHandlers
  if (type(hotkeys)=="string")
    hotkeys = [[hotkeys, @() showPieMenu(!showPieMenu.value)]]

  return function() {
    return {
      hotkeys
      eventHandlers
      children = showPieMenu.value ? pieMenu : null
      watch = showPieMenu
    }
  }
}

return mkPieMenuActivator
