from "%darg/ui_imports.nut" import *

// Combinations priority demo

let cursors = require("samples_prog/_cursors.nut")

function button(params) {
  let text = params?.text ?? ""
  let onClick = params?.onClick
  let onHover = params?.onHover

  return watchElemState(function(sf) {
    return{
      rendObj = ROBJ_BOX
      size = [sh(20),SIZE_TO_CONTENT]
      padding = sh(2)
      fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
      borderWidth = (sf & S_HOVER) ? 2 : 0
      behavior = [Behaviors.Button]
      halign = ALIGN_CENTER
      onClick = onClick
      onHover = onHover
      hotkeys = params?.hotkeys
      children = {rendObj = ROBJ_TEXT text = text color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0) pos = (sf & S_ACTIVE) ? [0,2] : [0,0] }
    }
  })
}


function makeBtn(hk, params={}) {
  return button({text=hk, onClick = @() vlog(hk), hotkeys=[hk]}.__merge(params))
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(5)
  cursor = cursors.normal
  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = [
    {
      flow = FLOW_HORIZONTAL
      gap = sh(5)
      children = [
        makeBtn("O")
        makeBtn("L.Ctrl O")
        makeBtn("L.Ctrl L.Shift O")
      ]
    }
    {
      flow = FLOW_HORIZONTAL
      gap = sh(5)
      children = [
        makeBtn("^K")
        makeBtn("^L.Ctrl K")
        makeBtn("^L.Ctrl L.Shift K")
      ]
    }
/*
    // Press vs Release
    {
      flow = FLOW_HORIZONTAL
      gap = sh(5)
      children = [
        makeBtn("K")
        makeBtn("L.Ctrl K")
        makeBtn("L.Ctrl L.Shift K")
      ]
    }
*/
    {
      flow = FLOW_HORIZONTAL
      gap = sh(5)
      children = [
        makeBtn("^L.Ctrl L.Shift L")
        makeBtn("^L.Ctrl L")
        makeBtn("^L")
      ]
    }
    {
      flow = FLOW_HORIZONTAL
      gap = sh(5)
      children = [
        makeBtn("L.Ctrl L.Shift M")
        makeBtn("L.Ctrl M")
        makeBtn("M")
      ]
    }
  ]
}
