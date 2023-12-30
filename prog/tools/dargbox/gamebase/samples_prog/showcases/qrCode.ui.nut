from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")
let {generateQrArray} = require("%sqstd/qrCode.nut")


let qrCanvas = {
  rendObj = ROBJ_VECTOR_CANVAS
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = [hdpx(300), hdpx(300)]
  lineWidth = hdpx(1.0)
  color = Color(0, 0, 0)
  fillColor = Color(0, 0, 0)
  commands = [
  ]
}


function basicsRoot() {

  let qr = generateQrArray("https://gaijin.team/ABCD&=456345635635")
  qrCanvas.commands = []
  let qrSize = qr.len();
  let cellSize = 100.0 / qrSize;
  for (local y = 0; y < qrSize; y++)
    for (local x = 0; x < qrSize; x++)
      if (qr[y][x])
        qrCanvas.commands.append([VECTOR_RECTANGLE, x * cellSize, y * cellSize, cellSize, cellSize])


  return {
    rendObj = ROBJ_SOLID
    color = Color(200, 200, 200)
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
        children = [
          qrCanvas
        ]
      }
    ]
  }
}

return basicsRoot