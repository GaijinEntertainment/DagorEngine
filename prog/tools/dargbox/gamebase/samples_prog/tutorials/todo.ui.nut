from "%darg/ui_imports.nut" import *
from "string" import strip

//---------------------------------------------------------------------
// Palette: put all colors in one place to keep styling consistent.
//---------------------------------------------------------------------
let Palette = {
  appBg              = Color(24,26,34)
  panelBg            = Color(32,34,44)
  inputBg            = Color(30,30,35)
  inputBgFocus       = Color(20,20,25)
  labelBg            = Color(28,28,40)
  border             = Color(100,100,120)
  borderHover        = Color(180,180,180)
  btnBg              = Color(40,48,72)
  btnBgHover         = Color(50,60,90)
  btnBgActive        = Color(70,80,110)
  btnBorder          = Color(80,100,160)
  dangerBorder       = Color(190,80,80)
  textPrimary        = Color(240,245,255)
  textMuted          = Color(160,170,180)
  textPlaceholder    = Color(160,160,160)
  checkboxBg         = Color(20,22,30)
  checkboxTick       = Color(120,200,140)
  iconText           = Color(210,210,230)
}

//---------------------------------------------------------------------
// Reactive state
//---------------------------------------------------------------------
let items  = Watched([])       // Array<{ id, text, done }>
let newItemText = Watched("")  // Input value for new task
let filter = Watched("all")    // "all" | "active" | "done"

local nextId = 1               // mutable counter for unique ids

//---------------------------------------------------------------------
// State mutation helpers — ALWAYS set Watched with a NEW array (immutability helps diffs).
//---------------------------------------------------------------------
function addItem() {
  let text = strip(newItemText.get() ?? "")
  if (text.len() == 0)
    return
  let id = nextId
  nextId++
  let arr = clone items.get()
  arr.append({ id, text, done = false })
  items.set(arr)
  newItemText.set("")
}

function toggleItem(id) {
  let arr = items.get()
  let idx = arr.findindex(@(it) it.id == id)
  if (idx == null)
    return
  let it = clone arr[idx]
  it.done = !it.done
  let copy = clone arr
  copy[idx] = it
  items.set(copy)
}

function removeItem(id) {
  items.modify(@(v) v.filter.set(@(it) it.id != id))
}

function clearCompleted() {
  items.modify(@(v) v.filter.set(@(it) !it.done))
}

//---------------------------------------------------------------------
// Derived state — which items should be shown given the current filter.
//---------------------------------------------------------------------
let visibleItems = Computed(function() {
  let f = filter.get()
  let arr = items.get()
  if (f == "active")
    return arr.filter.set(@(it) !it.done)
  else if (f == "done")
    return arr.filter.set(@(it) it.done)
  return arr
})

//---------------------------------------------------------------------
// Primitive controls (implemented from primitives + low-level behaviors only)
//---------------------------------------------------------------------

function TextInput(textState, placeholder="") {
  return watchElemState(@(sf) {
    size = static [pw(100), sh(4)]  // fixed height; fills available width
    rendObj = ROBJ_BOX
    fillColor = (sf & S_KB_FOCUS) ? Palette.inputBgFocus : Palette.inputBg
    borderWidth = 1
    borderColor = (sf & S_HOVER) ? Palette.borderHover : Palette.border
    padding = static [sh(0.5), sh(1)]
    children = @() {
      size = flex()
      rendObj = ROBJ_TEXT
      behavior = Behaviors.TextInput
      color = Palette.textPrimary
      padding = static [0, sh(0.2)]
      watch = textState
      text = textState.get() ?? ""
      onEscape = function() { textState("") }
      onReturn = function() { addItem() }
      onChange = function(t) { textState.set(t) }
      // Placeholder when empty and not focused
      children = (textState.get().len() == 0 && !(sf & S_KB_FOCUS)) ?
        { rendObj = ROBJ_TEXT, color = Palette.textPlaceholder, text = placeholder, padding = static [0, sh(0.2)] }
        : null
    }
  })
}

function Button(label, onClickCb, options = {}) {
  let pad = options?.pad ?? static [sh(0.8), sh(1.2)]
  let minw = options?.minw ?? 0
  let danger = options?.danger ?? false
  return watchElemState(@(sf) {
    rendObj = ROBJ_BOX
    minWidth = minw
    padding = pad
    behavior = Behaviors.Button
    fillColor = (sf & S_ACTIVE) ? Palette.btnBgActive
              : (sf & S_HOVER)  ? Palette.btnBgHover
              :                   Palette.btnBg
    borderWidth = 1
    borderColor = danger ? Palette.dangerBorder : Palette.btnBorder
    onClick = function() { if (onClickCb) onClickCb() }
    children = {
      rendObj = ROBJ_TEXT
      color = danger ? Palette.textMuted : Palette.textPrimary
      text = label
    }
  })
}

// Checkbox — square toggle; a solid inner square indicates checked.
function Checkbox(checked, onToggle) {
  return watchElemState(@(sf) {
    size = static [sh(2.2), sh(2.2)]
    behavior = Behaviors.Button
    rendObj = ROBJ_BOX
    fillColor = Palette.checkboxBg
    borderWidth = 1
    borderColor = (sf & S_HOVER) ? Palette.borderHover : Palette.border
    onClick = function() { if (onToggle) onToggle() }
    children = checked ? {
      rendObj = ROBJ_BOX
      size = static [pw(60), ph(60)]
      hplace = ALIGN_CENTER
      vplace = ALIGN_CENTER
      fillColor = Palette.checkboxTick
      borderRadius = sh(0.2)
    } : null
  })
}

// IconButton — minimal button for small glyphs like "+" or "✖".
function IconButton(char, onClickCb) {
  return watchElemState(@(sf) {
    behavior = Behaviors.Button
    rendObj = ROBJ_BOX
    padding = static [sh(0.5), sh(0.8)]
    fillColor = (sf & S_HOVER) ? Palette.btnBgHover : Palette.btnBg
    borderWidth = 1
    borderColor = (sf & S_HOVER) ? Palette.borderHover : Palette.border
    onClick = function() { if (onClickCb) onClickCb() }
    children = {
      rendObj = ROBJ_TEXT
      text = char
      color = Palette.iconText
    }
  })
}

//---------------------------------------------------------------------
// Higher-level composition
//---------------------------------------------------------------------

// Single to-do item row.
function TodoItemView(item) {
  let id = item.id
  return {
    flow = FLOW_HORIZONTAL
    gap = sh(1)
    size = static [pw(100), SIZE_TO_CONTENT]
    valign = ALIGN_CENTER
    children = [
      Checkbox(item.done, function(){ toggleItem(id) })
      {
        // Clickable label area: toggles item when clicked
        size = flex()
        behavior = Behaviors.Button
        onClick = function(){ toggleItem(id) }
        rendObj = ROBJ_BOX
        padding = static [sh(0.6), sh(0.8)]
        fillColor = Palette.labelBg
        children = [
          {
            rendObj = ROBJ_TEXT
            color = item.done ? Palette.textMuted : Palette.textPrimary
            text = item.text
          },
          item.done ? {
            rendObj = ROBJ_SOLID
            size = static [pw(100), 1]
            pos = static [0, sh(1.4)]
            color = Palette.textMuted
          } : null
        ]
      }
      IconButton("×", function(){ removeItem(id) })
    ]
  }
}

// List of items; rebuilds whenever `visibleItems` changes.
function TodoListPanel() {
  return {
    rendObj = ROBJ_BOX
    fillColor = Palette.panelBg
    padding = sh(1)
    gap = sh(0.4)
    size = flex()
    flow = FLOW_VERTICAL
    children = visibleItems.get().map(TodoItemView)
    watch = [visibleItems]
  }
}

// Filter bar with three filters and Clear Completed.
function FilterBar() {
  function makeFilterBtn(key, title) {
    return Button(
      (filter.get() == key) ? $"• {title}" : title,
      function(){ filter.set(key) },
      { minw = sh(8), pad = static [sh(0.6), sh(1.0)] }
    )
  }
  return {
    flow = FLOW_HORIZONTAL
    gap = sh(0.6)
    children = [
      makeFilterBtn("all", "All"),
      makeFilterBtn("active", "Active"),
      makeFilterBtn("done", "Done"),
      Button("Clear completed", clearCompleted, { pad = static [sh(0.6), sh(1.0)], danger = true })
    ]
    watch = [filter]
  }
}

//---------------------------------------------------------------------
// Application root — vertical layout: header, input row, list panel, filter bar.
//---------------------------------------------------------------------
function App() {
  return {
    size = flex()
    rendObj = ROBJ_SOLID
    color = Palette.appBg
    padding = static [sh(3), sh(2)]
    gap = sh(2)
    flow = FLOW_VERTICAL
    children = [
      {
        rendObj = ROBJ_TEXT
        text = "To‑Do"
        color = Palette.textPrimary
        fontSize = sh(2.4)
      }
      {
        flow = FLOW_HORIZONTAL
        gap = sh(1)
        size = static [flex(), SIZE_TO_CONTENT]
        children = [
          { size = flex(), children = TextInput(newItemText, "Add a task and press Enter or \"Add\"…") },
          Button("Add", addItem, { minw = sh(8) })
        ]
      }
      // Important: pass components as FUNCTIONS, not evaluated tables
      TodoListPanel
      FilterBar
    ]
  }
}

return App
