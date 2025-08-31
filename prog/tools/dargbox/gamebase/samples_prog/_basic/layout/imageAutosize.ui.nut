from "%darg/ui_imports.nut" import *

let fillerBlock = {
  rendObj = ROBJ_SOLID
  size = static [flex(), sh(1)]
  color = 0xFFFCC66
}

let imageNet = {
  rendObj = ROBJ_IMAGE
  image = Picture("https://enlisted.net/upload/image/2021/10/squad_prize.jpg")
  keepAspect = KEEP_ASPECT_FIT
}

let imageLocal = {
  rendObj = ROBJ_IMAGE
  image = Picture("ui/ca_cup1")
  keepAspect = KEEP_ASPECT_FIT
}

return {
  size = static [sh(80), flex()]
  hplace = ALIGN_CENTER
  gap = sh(3)
  flow = FLOW_HORIZONTAL
  children = [
    {
      rendObj = ROBJ_FRAME
      size = flex()
      gap = fillerBlock
      flow = FLOW_VERTICAL
      clipChildren = true
      children = [
        // keeping it's own size in pixels
        imageNet.__merge({ size = SIZE_TO_CONTENT })
        // expecting that image should fill all width of available space, but have unneeded blank offsets
        imageNet.__merge({ size = flex() })
        // expecting that image fill all width and fit height with aspect, but it's empty at all
        imageNet.__merge({ size = FLEX_H })
        // image have fixed width and keeping height with aspect, but it's size is totally independent of parent
        imageNet.__merge({ size = static [sh(20), SIZE_TO_CONTENT] })
      ]
    }
    {
      rendObj = ROBJ_FRAME
      size = flex()
      gap = fillerBlock
      flow = FLOW_VERTICAL
      clipChildren = true
      children = [
        imageLocal.__merge({ size = SIZE_TO_CONTENT })
        imageLocal.__merge({ size = flex() })
        imageLocal.__merge({ size = FLEX_H })
        imageLocal.__merge({ size = static [sh(20), SIZE_TO_CONTENT] })
      ]
    }
  ]
}