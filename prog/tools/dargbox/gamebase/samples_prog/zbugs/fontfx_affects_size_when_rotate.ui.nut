from "%darg/ui_imports.nut" import *
from "math" import PI, sin, cos

let text = {
  rendObj = ROBJ_TEXT
  text = 1
  transform = {}
}

let buggedText = {
  rendObj = ROBJ_TEXT
  text = 1
  transform = {}
  fontFx = FFT_GLOW
}

let R = 100

function mkSample() {
  local children = []

  for (local i = 0; i < 8; i++) {
    let angle = 2 * PI / 8 * i
    let rotate = angle * 180 / PI

    let translate = [R * cos(angle), R * sin(angle)]
    let buggedTranslate = [R * cos(angle) + 250, R * sin(angle)]
    children.append(text.__merge({ transform = {rotate, translate} }))
    children.append(buggedText.__merge({ transform = {rotate, translate = buggedTranslate} }))
  }

  return {
    children
  }
}

return mkSample()
