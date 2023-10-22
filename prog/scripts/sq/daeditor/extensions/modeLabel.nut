from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *

let {showPointAction, namePointAction, editorFreeCam} = require("%daeditor/state.nut")

return @(levelLoaded) @() {
  watch = [
    levelLoaded
    editorFreeCam
    showPointAction
    namePointAction
  ],
  size = flex(),
  children = [
    !levelLoaded.value ? null : (
      editorFreeCam.value ? { rendObj = ROBJ_BOX, fillColor = Color(20,20,20,100), borderRadius = hdpx(10), padding = hdpx(8), hplace = ALIGN_CENTER,
                              children = txt(showPointAction.value ? $"Free camera [ {namePointAction.value} ]" : "Free camera mode") } :
      showPointAction.value ? { rendObj = ROBJ_BOX, fillColor = Color(20,20,20,100), borderRadius = hdpx(10), padding = hdpx(8), hplace = ALIGN_CENTER,
                                children = txt(namePointAction.value) } :
      null
    )
  ]
}
