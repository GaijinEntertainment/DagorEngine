from "%darg/ui_imports.nut" import *

let function mkCursor(fillColor, borderColor){
  return {
    rendObj = ROBJ_VECTOR_CANVAS
    size = [sh(3), sh(3)]

    commands = [
      [VECTOR_WIDTH, hdpx(1.4)],
      [VECTOR_FILL_COLOR, fillColor],
      [VECTOR_COLOR, borderColor],
      [VECTOR_POLY, 0,0, 80,65, 46,65, 35,100],
    ]
  }
}

let function mkResizeCursor(rotate=0){
  return {
    transform = {pivot = [0.5,0.5], rotate=rotate}
    rendObj = ROBJ_VECTOR_CANVAS
    size = [sh(3), sh(3)]
    hotspot = [sh(1.5), sh(1.5)]

    commands = [
      [VECTOR_WIDTH, hdpx(1.4)],
      [VECTOR_FILL_COLOR, Color(180,180,180,180)],
      [VECTOR_COLOR, Color(20, 40, 70, 250)],
      [VECTOR_POLY, 50,0, 70,33, 58,33, 58,67, 70,67, 50,100, 30,67, 43,67, 43,33, 30,33],
    ]
  }
}

let cursors = {
  normal = Cursor(mkCursor(Color(180,180,180,180), Color(20, 40, 70, 250)))

  active = Cursor(mkCursor(Color(40,40,10,180), Color(180, 180, 180, 50)))

  sizeDiagLtRb = Cursor(mkResizeCursor(-45))

  sizeDiagRtLb = Cursor(mkResizeCursor(45))

  sizeH = Cursor(mkResizeCursor(90))

  sizeV = Cursor(mkResizeCursor())

}
cursors["moveResizeCursors"] <- {
  [MR_LT] = cursors.sizeDiagLtRb,
  [MR_RB] = cursors.sizeDiagLtRb,
  [MR_LB] = cursors.sizeDiagRtLb,
  [MR_RT] = cursors.sizeDiagRtLb,
  [MR_T] = cursors.sizeV,
  [MR_B] = cursors.sizeV,
  [MR_L] = cursors.sizeH,
  [MR_R] = cursors.sizeH,
}


return cursors
