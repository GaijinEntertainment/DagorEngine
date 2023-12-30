from "%darg/ui_imports.nut" import *

function watchElems(elems, params = {}) {
  if (typeof(elems) != "array")
    elems = [elems]
  local { watch = null } = params
  if (watch == null)
    watch = []
  else if (typeof(watch) != "array")
    watch = [watch]
  watch.extend(elems.filter(@(v) v instanceof Watched))
  return @() {
    children = elems
      .map(@(v) v instanceof Watched ? v.value : v)
      .filter(@(v) v != null)
  }.__update(params, { watch })
}

let extCounter = Watched(0)

let btnFlags = Watched(0)

let btnControl = @() {
  watch = [extCounter, btnFlags]
  rendObj = ROBJ_SOLID
  size = [sh(5), sh(5)]
  behavior = Behaviors.Button
  onElemState = @(sf) btnFlags(sf)
  onClick = @() extCounter(extCounter.value + 1)
  color = btnFlags.value & S_ACTIVE ? 0xFF999999
    : btnFlags.value & S_HOVER ? 0xFFAAAAAA
    : 0xFF666666
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  children = {
    rendObj = ROBJ_TEXT
    text = $"({extCounter.value})"
  }
}

let mkRoller = @(step, offset = 0) Computed(@()
  (extCounter.value + offset) % step == 0 ? null : {
    rendObj = ROBJ_SOLID
    size = [sh(5), sh(5)]
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    color = 0xFFFFCC33
    children = {
      rendObj = ROBJ_TEXT
      text = $"{step}:{offset}"
      color = 0xFF000000
    }
  })

let uiFreq = mkRoller(2)
let uiFreqInv = mkRoller(2, 1)
let uiRare = mkRoller(5)
let uiEpic = mkRoller(13)

let uiSpacer = {
  rendObj = ROBJ_SOLID
  size = [sh(5), sh(5)]
  color = 0xFF556677
}

let uiFunc = @() {
  rendObj = ROBJ_SOLID
  size = [sh(5), sh(5)]
  color = 0xFF557766
}

let uiWatched = @() {
  watch = extCounter
  rendObj = ROBJ_SOLID
  size = [sh(5), sh(5)]
  color = extCounter.value & 1 ? 0xFFFF0000 : 0xFF00FFFF
}

let compList = watchElems([
  uiFreq, uiSpacer, uiFreqInv, uiRare, uiEpic, uiRare, uiFunc, uiFreqInv, uiWatched
], {
  rendObj = ROBJ_FRAME
  flow = FLOW_HORIZONTAL
  padding = sh(1)
  gap = sh(1)
})

return {
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  halign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = sh(1)
  children = [
    btnControl
    compList
  ]
}