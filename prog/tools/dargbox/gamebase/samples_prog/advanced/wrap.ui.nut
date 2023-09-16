from "%darg/ui_imports.nut" import *

let cursor = Cursor({ rendObj = ROBJ_IMAGE size = [32, 32] image = Picture("!ui/atlas#cursor.svg:{0}:{0}:K".subst(hdpx(32))) })

local elems = [].resize(12,1)
foreach (i, _ in elems)
  elems[i]=i

elems = elems.map(@(i) {rendObj = ROBJ_TEXT text = i key=i})
let wrapped = {
  size = [hdpx(100),hdpx(200)]
  rendObj=ROBJ_FRAME
  children = wrap(elems, {
    width=hdpx(100)
    hGap=hdpx(5)
    height=hdpx(140)
    flow=FLOW_HORIZONTAL
    halign=ALIGN_CENTER
    valign=ALIGN_CENTER
    vGap={size=flex() maxHeight=hdpx(50) minHeight=hdpx(20)}
    flowElemProto={halign=ALIGN_CENTER valign=ALIGN_CENTER rendObj=ROBJ_FRAME}
  })
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
    gap = hdpx(20)

    children = [
      {rendObj = ROBJ_TEXT text="This demo shows autowrap, made in main library script, with the help of calc size of objects"}
      {size = SIZE_TO_CONTENT children = elems flow=FLOW_HORIZONTAL gap=hdpx(5)}
      wrapped
    ]
  }
}


return Root
