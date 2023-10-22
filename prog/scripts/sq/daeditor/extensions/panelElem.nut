from "%darg/ui_imports.nut" import *

let textButton = require("%daeditor/components/textButton.nut")
let {msgboxComponent=null, showMsgbox=null} = require_optional("%daeditor/components/mkMsgbox.nut")("daeditorx_")

let panelElemsButtonStyle = { boxStyle = { normal = { fillColor = Color(0,0,0,80) } } }

return {
  panelElemsLayout = @(elems) { size = SIZE_TO_CONTENT, hplace = ALIGN_RIGHT, flow = FLOW_HORIZONTAL,
                                children = elems }

  mkPanelElemsButton = @(name, cb) textButton(name, cb, panelElemsButtonStyle)
  panelElemsButtonStyle

  panelElemsMsgBox = showMsgbox
  panelElemsMsgBoxComponent = msgboxComponent
}
