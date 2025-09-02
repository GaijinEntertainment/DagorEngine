from "%darg/ui_imports.nut" import *

function watchElems(elems, params = {}) {
  if (typeof(elems) != "array")
    elems = [elems]
  local { watch = null } = params
  if (watch == null)
    watch = []
  else if (typeof(watch) != "array")
    watch = [watch]
  watch.extend(elems.filter(@(v) isObservable(v)))
  return @() {
    children = elems
      .map(@(v) isObservable(v) ? v.get() : v)
      .filter(@(v) v != null)
  }.__update(params, { watch })
}

let extCounter = Watched(0)

let btnFlags = Watched(0)

let btnControl = @() {
  watch = [extCounter, btnFlags]
  rendObj = ROBJ_SOLID
  size = sh(5)
  behavior = Behaviors.Button
  onElemState = @(sf) btnFlags.set(sf)
  onClick = @() extCounter.set(extCounter.get() + 1)
  color = btnFlags.get() & S_ACTIVE ? 0xFF999999
    : btnFlags.get() & S_HOVER ? 0xFFAAAAAA
    : 0xFF666666
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  children = {
    rendObj = ROBJ_TEXT
    text = $"({extCounter.get()})"
  }
}

let mkRoller = @(step, offset = 0) ComputedImmediate(@()
  (extCounter.get() + offset) % step == 0 ? null : {
    rendObj = ROBJ_SOLID
    size = sh(5)
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
  size = sh(5)
  color = 0xFF556677
}

let uiFunc = @() {
  rendObj = ROBJ_SOLID
  size = sh(5)
  color = 0xFF557766
}

let uiWatched = @() {
  watch = extCounter
  rendObj = ROBJ_SOLID
  size = sh(5)
  color = extCounter.get() & 1 ? 0xFFFF0000 : 0xFF00FFFF
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