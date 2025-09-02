let block = @(margin) {
  rendObj = ROBJ_FRAME
  borderWidth = 1
  color = 0xFF990099
  size = 100
  margin
}

let blocks = @(margin, selfMargin = 0) {
  rendObj = ROBJ_FRAME
  borderWidth = 3
  color = 0xFF00CC00
  flow = FLOW_HORIZONTAL
  margin = selfMargin
  children = [
    block(margin)
    block(margin)
    block(margin)
    block(0)
  ]
}

return {
  rendObj = ROBJ_FRAME
  borderWidth = 1
  color = 0xFFCC9900
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  children = {
    halign = ALIGN_CENTER
    flow = FLOW_VERTICAL
    gap = 20
    children = [
      blocks(-20, -20)
      blocks(-20)
      blocks(0)
      blocks(20)
      blocks(20, 20)
    ]
  }
}