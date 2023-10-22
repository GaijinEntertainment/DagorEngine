from "%darg/ui_imports.nut" import *
let {colors} = require("style.nut")
let scrollbar = require("%daeditor/components/scrollbar.nut")
let textButton = require("textButton.nut")


let helpText = @"
Tab - Find entity (by name, by group, select/deselect)
P - Property panel

Q - Select mode
W - Move mode (Shift+move to clone)
E - Rotate mode (hold Alt for snapping)
R - Scale mode
T - Create entity mode

Z - Zoom to selected
X - Local/global gizmo

Click - Select object
Ctrl+Click - Select multiple
Ctrl+A - Select all
Ctrl+D - Deselect all

Del - Delete selected

Ctrl+Z - Undo
Ctrl+Y - Redo
Ctrl+S - Save scene

Ctrl+Alt+D - Drop objects
Ctrl+Alt+E - Drop objects on normal
Ctrl+Alt+W - Surf over ground mode
Ctrl+Alt+R - Reset scale
Ctrl+Alt+T - Reset rotation

Space - Free camera mode (WASD + mouse, E up, C down, LShift faster, Q/Z focus)

Middle Mouse Button - Canned camera pan
Alt + Middle Mouse Button - Canned camera rotate (around selected entity)
Mouse Wheel (Ctrl faster, Alt slower) - Canned camera move forward/backward

F1 - Help
"


let help = @(showHelp) function(){
  let btnClose = {
    hplace = ALIGN_RIGHT
    size = SIZE_TO_CONTENT
    children = textButton("X", function() {
      showHelp.update(false)
    }, {hotkeys = [["Esc"]]})
  }

  let caption = {
    size = [flex(), SIZE_TO_CONTENT]
    rendObj = ROBJ_SOLID
    color = Color(50, 50, 50, 50)

    children = [
      {
        rendObj = ROBJ_TEXT
        text = "Keyboard help"
        margin = fsh(0.5)
      }
      btnClose
    ]
  }

  let textContent = {
    rendObj = ROBJ_TEXTAREA
    behavior = Behaviors.TextArea
    text = helpText
    size = [flex(), SIZE_TO_CONTENT]
    margin = fsh(0.5)
  }


  return {
    hplace = ALIGN_CENTER
    vplace = ALIGN_CENTER
    size = [sw(50), sh(80)]
    watch = showHelp
    rendObj = ROBJ_SOLID
    color = colors.ControlBg
    behavior = Behaviors.Button

    flow = FLOW_VERTICAL

    children = [
      caption
      scrollbar.makeVertScroll(textContent)
    ]
  }
}


return help
