from "%darg/ui_imports.nut" import *
let { setInterval, clearTimer } = require("dagor.workcycle")
let { rnd_int } = require("dagor.random")

let appearDelay = 0.043
let duration = 1.8
let rows = 8
let columns = 8
let totalBlocks = rows * columns

let size = hdpx(60)
let gap = hdpx(10)

let mask = Watched(0)
let rndMask = @() mask(mask.value | (1 << rnd_int(0, totalBlocks - 1).tointeger()))

let mkAnimBlock = @(animOvr) {
  size = flex()
  rendObj = ROBJ_SOLID
  color = 0xFFE5E005
  animations = [
    {
      prop = AnimProp.opacity, from = 0.5, to = 1.0, easing = CosineFull,
      duration, play = true, loop = true
    }.__update(animOvr)
  ]
}

let mkAnimsList = @(block) {
  flow = FLOW_VERTICAL
  gap
  children = array(rows).map(@(_, row) {
    flow = FLOW_HORIZONTAL
    gap
    children = array(columns).map(function(_, column) {
      let bit = 1 << (row * columns + column)
      let isVisible = Computed(@() (mask.value & bit) != 0)
      return @() {
        watch = isVisible
        size = [size, size]
        children = !isVisible.value ? null : block
      }
    })
  })
}

return {
  key = {}
  size = flex()
  function onAttach() {
    setInterval(appearDelay, rndMask)
    mask(0)
  }
  onDetach = @() clearTimer(rndMask)

  flow = FLOW_HORIZONTAL
  gap = 2 * gap + size
  children = [
    mkAnimsList(mkAnimBlock({}))
    mkAnimsList(mkAnimBlock({ globalTimer = true }))
  ]
}
