from "%darg/ui_imports.nut" import *


let calcColor = @(sf) (sf & S_HOVER) ? 0xFFFFFFFF : 0xA0A0A0A0

let lineWidth = hdpx(2)
let boxSize = hdpx(20)

function box(isSelected, sf) {
  let color = calcColor(sf)
  return {
    size = [boxSize, boxSize]
    rendObj = ROBJ_BOX
    fillColor = 0xFF000000
    borderWidth = lineWidth
    borderColor = color
    padding = 2 * lineWidth
    children = isSelected
      ? {
          size = flex()
          rendObj = ROBJ_SOLID
          color
        }
      : null
  }
}

let label = @(text, sf) {
  size = [flex(), SIZE_TO_CONTENT]
  rendObj = ROBJ_TEXT
  color = calcColor(sf)
  text = text
}

function optionCtor(option, isSelected, onClick) {
  let stateFlags = Watched(0)
  return function() {
    let sf = stateFlags.value

    return {
      size = [flex(), SIZE_TO_CONTENT]
      watch = stateFlags
      behavior = Behaviors.Button
      onElemState = @(s) stateFlags(s)
      onClick
      stopHover = true
      flow = FLOW_HORIZONTAL
      valign = ALIGN_CENTER
      gap = hdpx(5)
      children = [
        box(isSelected, sf)
        label(option.text, sf)
      ]
    }
  }
}

let baseStyle = {
  root = {
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_VERTICAL
    gap = hdpx(5)
  }
  optionCtor
}

let mkMultiselect = @(selected /*Watched({ <key> = true })*/, options /*[{ key, text }, ...]*/, minOptions = 0, maxOptions = 0, rootOverride = {}, style = baseStyle)
  function() {
    let numSelected = Computed(@() selected.value.filter(@(v) v).len())
    let mkOnClick = @(option) function() {
      let curVal = selected.value?[option.key] ?? false
      let resultNum = numSelected.value + (curVal ? -1 : 1)
      if ((minOptions == 0 || resultNum >= minOptions) //result num would
          && (maxOptions==0 || resultNum <= maxOptions))
        selected.mutate(function(s) { s[option.key] <- !curVal })

    }
    return style.root.__merge({
      watch = selected
      children = options.map(@(option) style.optionCtor(option, selected.value?[option.key] ?? false, mkOnClick(option)))
    })
    .__merge(rootOverride)
 }

return kwarg(mkMultiselect)
