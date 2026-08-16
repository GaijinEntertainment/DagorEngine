from "%darg/ui_imports.nut" import *
from "math" import max

let cursors = require("samples_prog/_cursors.nut")

const text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed efficitur felis tortor, sed finibus elit hendrerit et."
const longline = "Llanfairpwllgwyngyllgogerychwyrndrobwllllantysiliogogogoch"

const boxW = hdpx(440)
const fontSize = hdpx(20)

let fmtSize = @(sz) "".concat(sz[0].tointeger(), " x ", sz[1].tointeger())

function mkTextarea(params) {
  return {
    rendObj = ROBJ_TEXTAREA
    behavior = Behaviors.TextArea
    color = Color(220, 220, 180)
    fontSize
    text = params?.text ?? text
  }.__update(params)
}

// A text area smaller than its text: own size is the box, content size is the
// bounding box of the formatted lines.
let clippedTextarea = mkTextarea({
  size = const [boxW, hdpx(50)]
  clipChildren = true
})

// The same text at the same width, but self-sized: both sizes match.
let autoTextarea = mkTextarea({
  size = const [boxW, SIZE_TO_CONTENT]
})

// No width given at all. The temporary scene has no outer size to measure
// against, so the wrap width comes from maxWidth here.
let maxWidthTextarea = mkTextarea({
  size = SIZE_TO_CONTENT
  maxWidth = boxW
})

let longTextarea = mkTextarea({
  size = const [boxW, hdpx(50)]
  text = longline
})

// Content size is a per-axis maximum of child sizes, not a real bounding box:
// the offset from 'pos' is not in it.
const positionedChild = {
  size = [boxW, hdpx(50)]
  children = {
    rendObj = ROBJ_SOLID
    color = Color(120, 60, 60)
    size = [hdpx(50), hdpx(50)]
    pos = [hdpx(150), 0]
  }
}

let cases = [
  { name = "textarea, fixed box (text overflows)" comp = clippedTextarea }
  { name = "textarea, SIZE_TO_CONTENT height" comp = autoTextarea }
  { name = "textarea, width from maxWidth only" comp = maxWidthTextarea }
  { name = "textarea, with long text" comp = longTextarea }
  { name = "child moved by pos (not counted)" comp = positionedChild }
]

let txtStyle = { rendObj = ROBJ_TEXT fontSize color = Color(220, 220, 220) }

function mkCase(c) {
  let size = calc_comp_size(c.comp)
  let content = calc_content_size(c.comp)

  return {
    flow = FLOW_VERTICAL
    gap = hdpx(2)
    children = [
      txtStyle.__merge({ text = c.name })
      txtStyle.__merge({
        text = "".concat("size = ", fmtSize(size), "    content = ", fmtSize(content))
        color = size[1] == content[1] ? Color(140, 200, 140) : Color(230, 180, 100)
      })
      {
        // Reserve the larger of the two extents, so an overflowing content
        // frame does not run over the next case.
        size = [max(size[0], content[0]), max(size[1], content[1])]
        children = [
          // Both markers are siblings of the measured component, so they show
          // the numbers without changing its layout.
          { rendObj = ROBJ_SOLID color = Color(35, 35, 35, 35) size } // premultiplied: a faint white veil
          { rendObj = ROBJ_FRAME borderWidth = hdpx(1) color = Color(80, 200, 80) size = content }
          c.comp
        ]
      }
    ]
  }
}

let root = {
  rendObj = ROBJ_SOLID
  color = Color(10, 30, 50)
  size = flex()
  cursor = cursors.normal
  padding = hdpx(24)
  flow = FLOW_VERTICAL
  gap = hdpx(18)

  children = [
    txtStyle.__merge({
      text = "shaded area = calc_comp_size (the box), green frame = calc_content_size (what is inside it)"
      color = Color(255, 255, 160)
    })
    {
      flow = FLOW_VERTICAL
      gap = hdpx(18)
      children = cases.map(mkCase)
    }
  ]
}

return root
