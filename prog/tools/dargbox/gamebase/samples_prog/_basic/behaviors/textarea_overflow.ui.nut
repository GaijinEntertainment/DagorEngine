from "%darg/ui_imports.nut" import *
from "math" import max

let cursors = require("samples_prog/_cursors.nut")

let BOX_W = hdpx(300)
let PAD = hdpx(4)
let LABEL_COLOR = Color(160,160,160)
let TEXT_COLOR = Color(198,198,128)

let loremText = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed efficitur felis tortor, sed finibus elit hendrerit et. Vivamus in tortor sagittis, tempor turpis a, laoreet augue."
let loremTextWithTag = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. <color=#88ff88>Sed efficitur felis tortor, sed finibus elit hendrerit et.</color>"

function textHeight(probe_text, params = {}) {
  return calc_comp_size({ size = [BOX_W, SIZE_TO_CONTENT], ...params,
    rendObj = ROBJ_TEXTAREA, behavior = Behaviors.TextArea, text = probe_text })[1]
}

let lineH = textHeight("A")
let defHeight = 3.5 * lineH

function textarea(params) {
  return {
    size = flex()
    rendObj = ROBJ_TEXTAREA
    behavior = Behaviors.TextArea
    color = TEXT_COLOR
    text = loremText
    textOverflow = TOVERFLOW_ELLIPSIS
    ...params
  }
}

function sample(title, params, height = defHeight) {
  return {
    flow = FLOW_VERTICAL
    gap = PAD
    children = [
      { rendObj = ROBJ_TEXT, text = title, color = LABEL_COLOR }
      {
        rendObj = ROBJ_FRAME
        color = Color(110,110,110)
        size = [BOX_W + 2 * PAD, height + 2 * PAD]
        padding = PAD
        children = textarea(params)
      }
    ]
  }
}

let resizableSize = Watched([BOX_W + 2 * PAD, defHeight + 2 * PAD])
let resizableText = textarea({ text = " ".join(array(4, loremText)), behavior = [Behaviors.TextArea, Behaviors.WheelScroll] })

let resizable = {
  flow = FLOW_VERTICAL
  gap = PAD
  children = [
    { rendObj = ROBJ_TEXT, text = "Resizable, scrollable by wheel", color = LABEL_COLOR }
    @() {
      watch = resizableSize
      rendObj = ROBJ_FRAME
      color = Color(200,100,100)
      size = resizableSize.get()
      padding = PAD
      behavior = Behaviors.MoveResize
      moveResizeCursors = cursors.moveResizeCursors
      function onMoveResize(_dx, _dy, dw, dh) {
        let [w, h] = resizableSize.get()
        resizableSize.set([max(hdpx(20), w + dw), max(hdpx(10), h + dh)])
      }
      children = resizableText
    }
  ]
}

let lineSpacing = hdpx(6)
let parSpacing = hdpx(10)
let paragraphs = "One paragraph per line.\nThe second paragraph.\nThe third one.\nThe fourth one is hidden."
let afterEmptyLine = "Line one.\nLine two.\n\nLine four, after an empty line."
let longWord = "Short words, then Donaudampfschifffahrtsgesellschaftskapitaenswitwenrentenversicherungsantragsformular and more."
let wideLines = "A short line.\nA line that is much wider than the box, so it is cut at the right edge.\nAnother short one."
let bigTags = { big = { fontSize = hdpx(26) } }
let bigText = $"<big>{loremText}</big>"

let column = @(children) { flow = FLOW_VERTICAL, gap = hdpx(14), children }

return {
  rendObj = ROBJ_SOLID
  color = Color(80,80,80)
  cursor = cursors.normal
  size = flex()
  padding = sh(2)
  flow = FLOW_HORIZONTAL
  gap = hdpx(30)
  children = [
    column([
      sample("textOverflow = TOVERFLOW_CLIP (default)", { textOverflow = TOVERFLOW_CLIP })
      sample("textOverflow = TOVERFLOW_ELLIPSIS", {})
      sample("The text fits", { text = "A short text that fits." })
      sample("The box is lower than one line", {}, 0.5 * lineH)
      sample("FMT_NO_WRAP: a wide line is cut", { text = wideLines, preformatted = FMT_NO_WRAP })
    ])
    column([
      sample("halign = ALIGN_CENTER", { halign = ALIGN_CENTER })
      sample("halign = ALIGN_RIGHT", { halign = ALIGN_RIGHT })
      sample("ALIGN_RIGHT, indent and hangingIndent", { halign = ALIGN_RIGHT, indent = hdpx(30), hangingIndent = hdpx(15) })
      sample("lineSpacing", { lineSpacing }, 3 * lineH + 2 * lineSpacing + 0.5 * lineH)
      sample("parSpacing", { text = paragraphs, parSpacing }, 3 * lineH + 2 * parSpacing + 0.5 * lineH)
    ])
    column([
      sample("An empty line follows the last shown line", { text = afterEmptyLine }, 2.5 * lineH)
      sample("A long word broken across lines", { text = longWord }, 2.5 * lineH)
      sample("breakLongWords = false: the long word is cut", { text = longWord, breakLongWords = false }, 2.5 * lineH)
      sample("Color tag: the ellipsis keeps the element color", { text = loremTextWithTag })
      sample("Bigger tag font: the ellipsis keeps the element font",
        { text = bigText, tagsTable = bigTags }, 2.5 * textHeight("<big>A</big>", { tagsTable = bigTags }))
      sample("valign = ALIGN_CENTER", { valign = ALIGN_CENTER })
      resizable
    ])
  ]
}
