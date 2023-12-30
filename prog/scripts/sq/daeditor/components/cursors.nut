from "%darg/ui_imports.nut" import *

let tooltipBox = @(content) {
  rendObj = ROBJ_BOX
  fillColor = Color(30, 30, 30, 160)
  borderColor = Color(50, 50, 50, 20)
  size = SIZE_TO_CONTENT
  borderWidth = hdpx(1)
  padding = fsh(1)
  children = content
}

let tooltipGen = Watched(0)
let tooltipComp = {value = null}
function setTooltip(val){
  tooltipComp.value = val
  tooltipGen(tooltipGen.value+1)
}
let getTooltip = @() tooltipComp.value

let colorBack = Color(0,0,0,120)

let tooltipCmp = @(){
  key = "tooltip"
  pos = [0, hdpx(38)]
  watch = tooltipGen
  behavior = Behaviors.BoundToArea
  transform = {}
  children = type(getTooltip()) == "string"
  ? tooltipBox({
      rendObj = ROBJ_TEXTAREA
      behavior = Behaviors.TextArea
      maxWidth = hdpx(500)
      text = getTooltip()
      color = Color(180, 180, 180, 120)
    })
  : getTooltip()
}

let cursors = {getTooltip, setTooltip, tooltipCmp, tooltip = {}}

let cursorC = Color(255,255,255,255)

let cursorPc = {
  rendObj = ROBJ_VECTOR_CANVAS
  size = [fsh(2), fsh(2)]
  commands = [
    [VECTOR_WIDTH, hdpx(1)],
    [VECTOR_FILL_COLOR, cursorC],
    [VECTOR_COLOR, Color(20, 40, 70, 250)],
    [VECTOR_POLY, 0,0, 100,50, 56,56, 50,100],
  ]
    transform = {
      pivot = [0, 0]
      rotate = 29
    }
}

let cursorCir = {
  rendObj = ROBJ_VECTOR_CANVAS
  size = [fsh(2), fsh(2)]
  commands = [
    [VECTOR_WIDTH, hdpx(1)],
    [VECTOR_FILL_COLOR, Color(0,0,0,0)],
    [VECTOR_COLOR, cursorC],
    [VECTOR_ELLIPSE, 0, 0, 50, 50],
    [VECTOR_ELLIPSE, 0, 0, 5, 5],
  ]
    transform = {
      pivot = [0, 0]
      rotate = 29
    }
}

let cursorPick = {
  rendObj = ROBJ_VECTOR_CANVAS
  size = [fsh(2), fsh(2)]
  commands = [
    [VECTOR_WIDTH, hdpx(1)],
    [VECTOR_FILL_COLOR, Color(0,0,0,255)],
    [VECTOR_COLOR, cursorC],
    [VECTOR_LINE, -15,  0,  15,  0],
    [VECTOR_LINE,   0,-15,   0, 15],
    [VECTOR_LINE, -50,-50, -20,-50],
    [VECTOR_LINE, -50,-50, -50,-20],
    [VECTOR_LINE,  50,-50,  20,-50],
    [VECTOR_LINE,  50,-50,  50,-20],
    [VECTOR_LINE, -50, 50, -20, 50],
    [VECTOR_LINE, -50, 50, -50, 20],
    [VECTOR_LINE,  50, 50,  20, 50],
    [VECTOR_LINE,  50, 50,  50, 20],
  ]
    transform = {
      pivot = [0, 0]
      rotate = 0
    }
}

function mkPcCursor(children, cursorBase=cursorPc){
  return {
    size = [fsh(2), fsh(2)]
    hotspot = [0, 0]
    children = [cursorBase].extend(children)
    transform = {
      pivot = [0, 0]
    }
  }
}

cursors.normal <- Cursor(@() mkPcCursor([tooltipCmp]))

let helpSign = {
  rendObj = ROBJ_INSCRIPTION
  text = "?"
  fontSize = hdpx(20)
  vplace = ALIGN_CENTER
  fontFx = FFT_GLOW
  fontFxFactor=48
  fontFxColor = colorBack
  pos = [hdpx(25), hdpx(10)]
}

cursors.help <- Cursor(function(){
  return mkPcCursor([
    helpSign,
    tooltipCmp
  ])
})

cursors.actionCircle <- Cursor(function(){
  return mkPcCursor([
    tooltipCmp
  ], cursorCir)
})

cursors.actionPick <- Cursor(function(){
  return mkPcCursor([
    tooltipCmp
  ], cursorPick)
})

let getEvenIntegerHdpx = @(px) hdpx(0.5 * px).tointeger() * 2
let cursorSzResizeDiag = getEvenIntegerHdpx(18)

function mkResizeC(commands, angle=0){
  return {
    rendObj = ROBJ_VECTOR_CANVAS
    size = [cursorSzResizeDiag, cursorSzResizeDiag]
    commands = [
      [VECTOR_WIDTH, hdpx(1)],
      [VECTOR_FILL_COLOR, cursorC],
      [VECTOR_COLOR, Color(20, 40, 70, 250)],
      [VECTOR_POLY].extend(commands.map(@(v) v.tofloat()*100/7.0))
    ]
    hotspot = [cursorSzResizeDiag / 2, cursorSzResizeDiag / 2]
    transform = {rotate=angle}
  }
}
let horArrow = [0,3, 2,0, 2,2, 5,2, 5,0, 7,3, 5,6, 5,4, 2,4, 2,6]
cursors.sizeH <- Cursor(mkResizeC(horArrow))
cursors.sizeV <- Cursor(mkResizeC(horArrow, 90))
cursors.sizeDiagLtRb <- Cursor(mkResizeC(horArrow, 45))
cursors.sizeDiagRtLb <- Cursor(mkResizeC(horArrow, 135))

cursors.moveResizeCursors <- {
  [MR_LT] = cursors.sizeDiagLtRb,
  [MR_RB] = cursors.sizeDiagLtRb,
  [MR_LB] = cursors.sizeDiagRtLb,
  [MR_RT] = cursors.sizeDiagRtLb,
  [MR_T]  = cursors.sizeV,
  [MR_B]  = cursors.sizeV,
  [MR_L]  = cursors.sizeH,
  [MR_R]  = cursors.sizeH,
}

return cursors
