from "%darg/ui_imports.nut" import *

let gradientWidth = 15
let progressHeight = 40
let offs = [0, 14, 0, 0]
let screenOffs = [0, 22, 0, 0]


let progressBarProcess = {
  size = flex()
  clipChildren = true
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  children = {
    rendObj = ROBJ_9RECT
    image = Picture($"!ui/progress_bar_gradient.svg:{gradientWidth}:{20}:K?Ac")
    size = [pw(25), progressHeight]
    texOffs = offs
    screenOffs = screenOffs
    children = {
      rendObj = ROBJ_IMAGE
      image = Picture($"!ui/progress_bar_gradient.svg:{gradientWidth}:{20}:K?Ac")
      size = [gradientWidth, progressHeight]
      hplace = ALIGN_RIGHT
      pos = [0, ph(50)]
    }
  }
}

return progressBarProcess