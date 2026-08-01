from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

// warnBuilderAllocations flags any builder that constructs observables per rebuild.
// The naive column trips it on every interaction; the keyed column stays silent.
//require("daRg").gui_scene.setConfigProps({ warnBuilderAllocations = true, kbCursorControl = true })

let items = Watched([{ id = 1 }, { id = 2 }, { id = 3 }])
let selectedId = Watched(1)

// Each card owns a Watched stamped at construction with a monotonic birth id. If the card is
// rebuilt from scratch its birth id jumps; if its state is reused the id stays put - a visible
// proxy for "did we reconstruct".
local nextBirthId = 0

function mkCard(item) {
  nextBirthId += 1
  let birthId = Watched(nextBirthId)
  let isSelected = Computed(@() selectedId.get() == item.id)
  return function() {
    let sel = isSelected.get()
    return {
      watch = [birthId, isSelected]
      rendObj = ROBJ_BOX
      size = [sh(22), sh(9)]
      fillColor = sel ? Color(60, 90, 140) : Color(40, 40, 50)
      borderColor = sel ? Color(120, 160, 220) : Color(70, 70, 80)
      borderWidth = 2
      behavior = Behaviors.Button
      onClick = @() selectedId.set(item.id)
      flow = FLOW_VERTICAL
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      gap = sh(1)
      children = [
        { rendObj = ROBJ_TEXT text = $"item {item.id}" }
        { rendObj = ROBJ_TEXT text = $"birth #{birthId.get()}" color = Color(150, 200, 150) }
      ]
    }
  }
}

// WRONG: the list is a builder over both observables, so every selection change re-maps and
// reconstructs every card (fresh birthId + isSelected). warnBuilderAllocations flags this.
let naiveList = @() {
  watch = [items, selectedId]
  flow = FLOW_VERTICAL
  gap = sh(2)
  children = items.get().map(mkCard)
}

// RIGHT: per-card state built once, keyed by id, off the build path. The builder is a pure
// id -> cached-card lookup, so selecting a card rebuilds only paint; birth ids stay put.
let keyedList = mkKeyedList({
  source = items
  keyOf = @(it) it.id
  mkItem = mkCard
  containerProps = { flow = FLOW_VERTICAL, gap = sh(2) }
})

function column(title, list) {
  return {
    flow = FLOW_VERTICAL
    gap = sh(2)
    halign = ALIGN_CENTER
    children = [
      { rendObj = ROBJ_TEXT text = title color = Color(220, 220, 120) }
      list
    ]
  }
}

let btn = @(text, onClick) watchElemState(@(sf) {
  rendObj = ROBJ_BOX
  size = [sh(30), SIZE_TO_CONTENT]
  padding = sh(1.5)
  fillColor = (sf & S_ACTIVE) ? Color(0, 0, 0) : Color(200, 200, 200)
  behavior = Behaviors.Button
  halign = ALIGN_CENTER
  onClick = onClick
  children = { rendObj = ROBJ_TEXT text = text color = Color(20, 20, 20) }
})

let nextItemId = Watched(4)
function addItem() {
  let id = nextItemId.get()
  nextItemId.set(id + 1)
  items.set((clone items.get()).append({ id })) // fresh array: in-place append keeps the same ref and is not notified
}
function removeLast() {
  items.modify(@(v) v.len() > 0 ? v.slice(0, v.len() - 1) : v)
}

return {
  rendObj = ROBJ_SOLID
  color = Color(25, 28, 34)
  size = flex()
  cursor = cursors.normal
  padding = sh(4)
  flow = FLOW_VERTICAL
  gap = sh(3)
  children = [
    { rendObj = ROBJ_TEXT
      text = "Click a card to select it. Watch the birth ids: naive cards get reborn on every rebuild, keyed cards keep theirs." }
    { flow = FLOW_HORIZONTAL gap = sh(2)
      children = [ btn("Add item", addItem), btn("Remove last", removeLast) ] }
    { flow = FLOW_HORIZONTAL gap = sh(8) size = [SIZE_TO_CONTENT, flex()]
      children = [
        column("naive .map(mkCard)", naiveList)
        column("mkKeyedList", keyedList)
      ] }
  ]
}
