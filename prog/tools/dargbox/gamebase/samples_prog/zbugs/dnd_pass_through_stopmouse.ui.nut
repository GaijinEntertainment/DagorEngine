from "%darg/ui_imports.nut" import *

let cursor = Cursor(@() {
  rendObj = ROBJ_VECTOR_CANVAS
  size = [32, 32]
  commands = [
    [VECTOR_WIDTH, 2],
    [VECTOR_COLOR, 0xFFCC33],
    [VECTOR_FILL_COLOR, 0xFF557755],
    [VECTOR_POLY, 0,0, 75,75, 25,100, 0,0]
  ]
})

let decodeBeh = {
  [Behaviors.Button] = "Behaviors.Button",
  [Behaviors.Pannable] = "Behaviors.Pannable",
  [Behaviors.DragAndDrop] = "Behaviors.DragAndDrop",
}

function mkBtn(behavior) {
  let stateFlags = Watched(0)
  let text = ",".join((typeof behavior == "array" ? behavior : [behavior]).map(@(b) decodeBeh?[b] ?? b.tostring()))
  return @() {
    watch = stateFlags
    rendObj = ROBJ_SOLID
    size = [flex(), SIZE_TO_CONTENT]
    minWidth = SIZE_TO_CONTENT
    halign = ALIGN_CENTER
    behavior
    padding = 10
    color = 0x557755
    onElemState = @(sf) stateFlags(sf)
    cursor
    children = {
      rendObj = ROBJ_TEXT
      text
      color = stateFlags.value & S_ACTIVE ? 0xFFFFFF
        : stateFlags.value & S_HOVER ? 0xFFCC33
        : 0xFF000000
    }
  }
}

return {
  size = flex()
  valign = ALIGN_CENTER
  children = [
    {
      hplace = ALIGN_CENTER
      flow = FLOW_VERTICAL
      gap = 10
      children = [
        mkBtn(Behaviors.Button)
        // pannable catches active event on mouse click even under shield pane
        mkBtn(Behaviors.Pannable)
        mkBtn(Behaviors.DragAndDrop)
      ]
    }
    // front pane that overlapping left part of active elements to compare event shielding effect
    {
      rendObj = ROBJ_SOLID
      size = [pw(50), flex()]
      color = 0x22222222
      // cursors of bottom active elements should not override default one!
      stopMouse = true
      stopHover = true
      padding = 10
      children = {
        rendObj = ROBJ_TEXT
        text = "Shielding pane"
      }
    }
  ]
}
