from "daRg" import *
from "%darg/darg_library.nut" import hdpx

let autoScrollTextArea = @(params) {
  size = [flex(), SIZE_TO_CONTENT]
  clipChildren = true

  children = {
    size = [flex(), SIZE_TO_CONTENT]
    maxHeight = hdpx(400)
    rendObj = ROBJ_TEXTAREA
    behavior = [Behaviors.TextArea, Behaviors.Marquee]

    delay = [3.0,2.5]
    speed = [hdpx(10), 1]
    orientation = O_VERTICAL
  }.__update(type(params) == "table" ? params : { text = params })
}

return autoScrollTextArea