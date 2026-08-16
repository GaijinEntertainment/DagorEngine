from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

// Two columns render the same list: the naive one recreates card state on
// every rebuild, the stateful one constructs it once per card.

let items = Watched([{ id = 1, label = "alpha" }, { id = 2, label = "beta" }, { id = 3, label = "gamma" }])
let selectedId = Watched(1)
let ticker = Watched(0)
gui_scene.setInterval(1.0, @() ticker.set(ticker.get() + 1))

// Shown on a card: a birth id that jumps means the card's state was rebuilt.
local nextBirthId = 0

// Runs once per mounted card. `item` arrives as an observable that the
// reconciler writes when the parent rebuilds.
function cardCtor(item) {
  nextBirthId += 1
  let birthId = Watched(nextBirthId)
  let isSelected = Computed(@() selectedId.get() == item.get().id)
  let tickAtMount = ticker.get() // deliberate bare get: mount-time snapshot
  let ticks = Computed(@() ticker.get() - tickAtMount)
  return function() {
    let sel = isSelected.get()
    return {
      watch = [item, birthId, isSelected, ticks]
      rendObj = ROBJ_BOX
      size = [sh(24), sh(11)]
      fillColor = sel ? Color(60, 90, 140) : Color(40, 40, 50)
      borderColor = sel ? Color(120, 160, 220) : Color(70, 70, 80)
      borderWidth = 2
      behavior = Behaviors.Button
      onClick = @() selectedId.set(item.get().id)
      flow = FLOW_VERTICAL
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      children = [
        { rendObj = ROBJ_TEXT text = $"{item.get().label} (id {item.get().id})" }
        { rendObj = ROBJ_TEXT text = $"birth #{birthId.get()}" color = Color(150, 200, 150) }
        { rendObj = ROBJ_TEXT text = $"ticks {ticks.get()}" color = Color(200, 180, 120) }
      ]
    }
  }
}

let Card = StatefulComp(cardCtor, @(item) item.id)

// Same card, but built from a plain factory called inside a builder, so every
// change of items or selectedId recreates all of its state.
function mkCard(item) {
  nextBirthId += 1
  let birthId = Watched(nextBirthId)
  let isSelected = Computed(@() selectedId.get() == item.id)
  return @() {
    watch = [birthId, isSelected]
    rendObj = ROBJ_BOX
    size = [sh(24), sh(11)]
    fillColor = isSelected.get() ? Color(60, 90, 140) : Color(40, 40, 50)
    borderColor = isSelected.get() ? Color(120, 160, 220) : Color(70, 70, 80)
    borderWidth = 2
    behavior = Behaviors.Button
    onClick = @() selectedId.set(item.id)
    flow = FLOW_VERTICAL
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    children = [
      { rendObj = ROBJ_TEXT text = $"{item.label} (id {item.id})" }
      { rendObj = ROBJ_TEXT text = $"birth #{birthId.get()}" color = Color(150, 200, 150) }
    ]
  }
}

let naiveList = @() {
  watch = [items, selectedId]
  flow = FLOW_VERTICAL
  gap = sh(1.5)
  children = items.get().map(mkCard)
}

let statefulList = @() {
  watch = items
  flow = FLOW_VERTICAL
  gap = sh(1.5)
  children = items.get().map(@(it) Card(it))
}

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
  size = [sh(22), SIZE_TO_CONTENT]
  padding = sh(1.2)
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
  items.set((clone items.get()).append({ id, label = $"item {id}" }))
}
function removeLast() {
  items.modify(@(v) v.len() > 0 ? v.slice(0, v.len() - 1) : v)
}
function reverseItems() {
  let r = clone items.get()
  r.reverse()
  items.set(r)
}
// Same ids, flipped labels: new data arriving at unchanged keys.
function relabel() {
  items.set(items.get().map(@(it) {
    id = it.id
    label = it.label == it.label.toupper() ? it.label.tolower() : it.label.toupper()
  }))
}

return {
  rendObj = ROBJ_SOLID
  color = Color(25, 28, 34)
  size = flex()
  cursor = cursors.normal
  padding = sh(3)
  flow = FLOW_VERTICAL
  gap = sh(2)
  children = [
    { rendObj = ROBJ_TEXT
      text = "Naive cards are reborn on every rebuild; stateful cards keep their birth id and tick on." }
    { flow = FLOW_HORIZONTAL gap = sh(2)
      children = [ btn("Add", addItem), btn("Remove last", removeLast), btn("Reverse", reverseItems), btn("Relabel", relabel) ] }
    { flow = FLOW_HORIZONTAL gap = sh(8) size = [SIZE_TO_CONTENT, flex()]
      children = [
        column("naive .map(mkCard)", naiveList)
        column("StatefulComp", statefulList)
      ] }
  ]
}
