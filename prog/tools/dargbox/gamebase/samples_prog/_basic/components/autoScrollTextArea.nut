from "daRg" import *
from "%darg/darg_library.nut" import hdpx, Behaviors, FLEX_H

let autoScrollTextArea = @(params) {
  size = FLEX_H
  clipChildren = true

  children = {
    size = FLEX_H
    maxHeight = hdpx(400)
    rendObj = ROBJ_TEXTAREA
    behavior = static [Behaviors.TextArea, Behaviors.Marquee]

    delay = static [3.0,2.5]
    speed = static [hdpx(10), 1]
    orientation = O_VERTICAL
  }.__update(type(params) == "table" ? params : { text = params })
}

return autoScrollTextArea