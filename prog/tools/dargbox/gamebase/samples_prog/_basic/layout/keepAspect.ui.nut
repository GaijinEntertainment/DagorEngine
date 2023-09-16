from "%darg/ui_imports.nut" import *

let function img(size, keepAspect, imgOverride={}) {
  return {
    size = size
    rendObj = ROBJ_SOLID
    color = 0xFF2020D0
    clipChildren = true
    children = {
      size = flex()
      rendObj = ROBJ_IMAGE
      keepAspect = keepAspect
      image = Picture("ui/ca_cup1")
    }.__update(imgOverride)
  }
}

return {
  size = flex()
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  flow = FLOW_HORIZONTAL
  gap = hdpx(10)
  children = [
    {
      flow = FLOW_VERTICAL
      gap = hdpx(10)
      children = [
        img([hdpx(400), hdpx(100)], KEEP_ASPECT_NONE)
        img([hdpx(400), hdpx(100)], KEEP_ASPECT_FIT)
        img([hdpx(400), hdpx(100)], KEEP_ASPECT_FIT, { imageHalign = ALIGN_LEFT })
        img([hdpx(400), hdpx(100)], KEEP_ASPECT_FIT, { imageHalign = ALIGN_RIGHT })
        img([hdpx(400), hdpx(100)], KEEP_ASPECT_FILL)
      ]
    }
    {
      flow = FLOW_HORIZONTAL
      gap = hdpx(10)
      children = [
        img([hdpx(100), hdpx(540)], KEEP_ASPECT_NONE)
        img([hdpx(100), hdpx(540)], KEEP_ASPECT_FIT)
        img([hdpx(100), hdpx(540)], KEEP_ASPECT_FIT, { imageValign = ALIGN_TOP })
        img([hdpx(100), hdpx(540)], KEEP_ASPECT_FIT, { imageValign = ALIGN_BOTTOM })
        img([hdpx(100), hdpx(540)], KEEP_ASPECT_FILL)
        img([hdpx(100), hdpx(540)], KEEP_ASPECT_FILL, {imageHalign = ALIGN_LEFT})
      ]
    }
  ]
}
