from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let { showPointAction, namePointAction, editorFreeCam } = require("%daeditor/state.nut")

return @(levelLoaded) function() {
  let watch = [levelLoaded, editorFreeCam, showPointAction, namePointAction]

  let isFreeCam = editorFreeCam.get()
  let isShowPoint = showPointAction.get()

  if (!levelLoaded.get() || (!isFreeCam && !isShowPoint))
    return { watch }

  return {
    watch
    pos = [0, hdpx(50)]
    size = flex()
    children = isFreeCam
      ? {
          rendObj = ROBJ_BOX
          fillColor = Color(20,20,20,100)
          borderRadius = hdpx(10)
          padding = hdpx(8)
          hplace = ALIGN_CENTER
          children = txt(isShowPoint
            ? $"Free camera [ {namePointAction.get()} ]"
            : "Free camera mode")
        }
      : {
          rendObj = ROBJ_BOX
          fillColor = Color(20,20,20,100)
          borderRadius = hdpx(10)
          padding = hdpx(8)
          hplace = ALIGN_CENTER
          children = txt(namePointAction.get())
        }
  }
}
