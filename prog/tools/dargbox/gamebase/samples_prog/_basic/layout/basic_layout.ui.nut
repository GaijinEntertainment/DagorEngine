from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let box = {
  size = [SIZE_TO_CONTENT, sh(6)]
  color = Color(128,128,128)
  rendObj = ROBJ_SOLID
  margin = 7
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = {rendObj = ROBJ_TEXT text= "margin = 7"}
}

let box2 = {
  size = [SIZE_TO_CONTENT, sh(6)]
  color = Color(128,128,128)
  rendObj = ROBJ_SOLID
  margin = 0
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = {rendObj = ROBJ_TEXT text= "margin = 0"}
}

let flexbox = {
  size = [flex(), sh(6)]
  color = Color(128,128,128)
  rendObj = ROBJ_SOLID
  margin = 0
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = {rendObj = ROBJ_TEXT text= "flex() width"}
}

let halignToText = {
    [ALIGN_CENTER] = "center"
  , [ALIGN_LEFT] = "left"
  , [ALIGN_RIGHT] = "right"
}

let valignToText = {
    [ALIGN_CENTER] = "middle"
  , [ALIGN_TOP] = "top"
  , [ALIGN_BOTTOM] = "bottom"
}

let function alignShow (style={}){
  let halign = style?.halign ?? ALIGN_LEFT
  let valign = style?.valign ?? ALIGN_TOP
  return {
    halign = ALIGN_CENTER
    size = SIZE_TO_CONTENT
    flow = FLOW_VERTICAL
    gap=sh(1)
    children = [
      {rendObj = ROBJ_TEXT text=$"halign: {halignToText[halign]}"}
      {rendObj = ROBJ_TEXT text=$"valign: {valignToText[valign]}"}
      {
        flow = FLOW_VERTICAL
        gap = 5
        padding = 2
        clipChildren = true
        halign = halign
        valign = valign
        rendObj = ROBJ_SOLID
        color = Color(0,0,0)
        size = [sh(16), sh(35)]
        children = [box,flexbox,box2]
      }
    ]
  }
}

let horContainer = @(children) {
  flow = FLOW_HORIZONTAL
  gap = sh(4)
  children
}

return {
  rendObj = ROBJ_SOLID
  size = flex()
  color = Color(30,40,50)
  cursor = cursors.normal
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = sh(4)
  children = [
    horContainer([
      alignShow({halign = ALIGN_CENTER valign = ALIGN_CENTER})
      alignShow({halign = ALIGN_CENTER valign = ALIGN_BOTTOM})
      alignShow({halign = ALIGN_CENTER valign = ALIGN_TOP})
      alignShow({halign = ALIGN_RIGHT valign = ALIGN_CENTER})
      alignShow({halign = ALIGN_RIGHT valign = ALIGN_BOTTOM})
    ])
    horContainer([
      alignShow({halign = ALIGN_RIGHT valign = ALIGN_TOP})
      alignShow({halign = ALIGN_LEFT valign = ALIGN_CENTER})
      alignShow({halign = ALIGN_LEFT valign = ALIGN_BOTTOM})
      alignShow({halign = ALIGN_LEFT valign = ALIGN_TOP})
    ])
  ]
}
