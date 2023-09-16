let block = {
  rendObj = ROBJ_SOLID
  color = 0xFFCC33
  size = [100, 20]
}

let empty = @() {
  rendObj = ROBJ_SOLID
}

return {
  rendObj = ROBJ_FRAME
  gap = 10
  flow = FLOW_VERTICAL
  children = [
    null
    block
    block
    null
    null
    empty
    null
    block
    null
    block
    empty
  ]
}