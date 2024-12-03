from "%darg/ui_imports.nut" import *

let {makeVertScroll} = require("scrollbar.nut")
let {addModalWindow, removeModalWindow, modalWindowsComponent} = require("modalWindows.nut")
let {mkButton} = require("button.nut")
let {dtext, textarea, ta} = require("text.nut")
let {mkTextInput} = require("textInput.nut")
let {mkCombo} = require("combo.nut")
let {mkCheckbox} = require("checkbox.nut")
let cursors = require("cursors.nut")
let {mkSelect} = require("mkSelect.nut")
let {flatten} = require("%sqstd/underscore.nut")

let hflow = @(...) {flow = FLOW_HORIZONTAL children = flatten(vargv) gap = hdpx(10) valign = ALIGN_CENTER}
let vflow = @(...) {flow = FLOW_VERTICAL children = flatten(vargv) gap = hdpx(5) valign = ALIGN_CENTER}

return {
  modalWindowsComponent, addModalWindow, removeModalWindow,
  mkTextInput, dtext, textarea, mkCombo, ta, mkCheckbox,
  mkButton, cursors,
  makeVertScroll, mkSelect,
  hflow, vflow
}