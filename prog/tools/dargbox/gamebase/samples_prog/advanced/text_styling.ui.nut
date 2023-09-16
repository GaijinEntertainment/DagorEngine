from "%darg/ui_imports.nut" import *

let fa = require("samples_prog/_basic/goodies/fontawesome.map.nut")

let textValue = $"smile face <fa>{fa["smile-o"]}</fa>\n<fa><blue>{fa["snapchat-ghost"]}</blue></fa> boo!"
let textStyles = {
  fa = {
    color = 0xFFFFCC33
    font = Fonts?.fontawesome
  }
  blue = {
    color = 0xFF66BBEE
  }
}
let label = {
  rendObj = ROBJ_TEXTAREA
  behavior = Behaviors.TextArea
  color = 0xFF000000
  text = textValue
  tagsTable = textStyles
}

let stateFlags = Watched(0)
return {
  size = flex()
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = @() {
    watch = stateFlags
    rendObj = ROBJ_SOLID
    behavior = Behaviors.Button
    padding = 15
    color = stateFlags.value & S_HOVER ? 0xCCEE9966 : 0xCCCCCCCC
    children = label
    onElemState = @(sf) stateFlags(sf)
  }
}