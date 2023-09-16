from "%darg/ui_imports.nut" import *
let {select} = require("select.nut")

let selected = Watched("medium")

return {
  rendObj = ROBJ_SOLID
  color = Color(0,0,0)
  size = flex()
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  cursor = Cursor({ rendObj = ROBJ_IMAGE size = [32, 32] image = Picture("!ui/atlas#cursor.svg:{0}:{0}:K".subst(hdpx(32))) })
  children = @() {
    valign  = ALIGN_CENTER
    halign  = ALIGN_CENTER
    flow    = FLOW_VERTICAL
    gap  = sh(2)
    size = flex()
    padding= sh(5)
    children = [
      select({state=selected, options = ["low","medium", "high"]})
    ]
  }
}

