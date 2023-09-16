from "%darg/ui_imports.nut" import *

let pic = Picture("ui/ca_cup1.png")

let mkProgress = @(image) {
  size = [hdpx(100),hdpx(100)]
  rendObj = ROBJ_PROGRESS_CIRCULAR
  image
  bgColor = Color(64,64,255)
  fgColor = Color(255,255,255)
  fValue = 0.78
}

return {
  size = flex()
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = sh(1)
  children = [
    { rendObj = ROBJ_TEXT text = "This is circullar robj, with red bgColor and white fgColor, and fValue=78%" }
    {
      flow = FLOW_HORIZONTAL
      gap = sh(1)
      children = [
        mkProgress(pic)
        mkProgress(null)
      ]
    }
  ]
}
