from "%darg/ui_imports.nut" import *
from "math" import max

let {tostring_r} = require("%sqstd/string.nut")
let cursor = Cursor({ rendObj = ROBJ_IMAGE size = [32, 32] image = Picture("!ui/atlas#cursor.svg:{0}:{0}:K".subst(hdpx(32))) })

let bbb = {rendObj = ROBJ_TEXT text = "BBB"}
let aaa = { padding = [20,0,0,50] children = {rendObj = ROBJ_TEXT text = "AAA"}}
let DemoBox = {
  flow    = FLOW_VERTICAL
  size    = SIZE_TO_CONTENT

  children = [
    aaa
    bbb
  ]
}

let sz = calc_comp_size(DemoBox)

vlog($"demobox: {tostring_r(sz)}")
let bsz = calc_comp_size(bbb)
let asz = calc_comp_size(aaa)

vlog("".concat("bbb + aaa: [ max(", bsz[0], ",", asz[0], ") ,", bsz[1], " + ", asz[1], "]"))

let DebugFrame = {
  rendObj = ROBJ_FRAME
  padding = 1
  color = Color(255,0,0)
  size = sz
}

let function Root() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(10,30,50)
    cursor = cursor
    padding = sh(10)
    size = flex()
    flow = FLOW_VERTICAL
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    gap = 20

    children = [
      {rendObj = ROBJ_TEXT text="This demo shows how to get sizes of objects in layout - frame around text is separate element, not a holder of texts"}
      { children = [
          DemoBox
          DebugFrame
        ]
      }
    ]
  }
}


return Root
