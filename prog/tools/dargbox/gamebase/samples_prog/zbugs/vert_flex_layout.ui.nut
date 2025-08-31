from "%darg/ui_imports.nut" import *

/*
Task: display content with a title in a window that has a fixed
minimum height and stretches when the content size increases

Expected behavior: content is centered vertically in the window
and as it gets larger the window size increases

In fact: as long as the elements do not reach the size of the
window, it retains its minimum size. When the window is
automatically stretched by internal elements, insufficient
height is used and the elements do not fit completely into
the window. Increasing gap and padding in the layout increases
the error size
*/
let offsets = sh(1) // change it to view overflow difference

let mkPanel = @(text, content = null) {
  rendObj = ROBJ_FRAME
  size = static [sw(12), flex()]
  minHeight = SIZE_TO_CONTENT
  flow = FLOW_VERTICAL
  gap = offsets
  padding = offsets
  borderWidth = 2
  color = 0xFFCCFF33
  children = [
    {
      rendObj = ROBJ_FRAME
      size = flex()
      minHeight = SIZE_TO_CONTENT
      valign = ALIGN_CENTER
      flow = FLOW_VERTICAL
      gap = offsets
      padding = offsets
      borderWidth = 1
      color = 0xFFCC66FF
      children = [
        {
          rendObj = ROBJ_TEXT
          size = FLEX_H
          halign = ALIGN_CENTER
          text
        }
        content == null ? null : {
          rendObj = ROBJ_FRAME
          size = FLEX_H
          borderWidth = 1
          color = 0xFFFF6600
          children = content
        }
      ]
    }
    {
      rendObj = ROBJ_TEXT
      size = FLEX_H
      halign = ALIGN_CENTER
      text = "BUTTONS"
    }
  ]
}

let mkContent = @(text) {
  rendObj = ROBJ_TEXTAREA
  size = FLEX_H
  behavior = Behaviors.TextArea
  halign = ALIGN_CENTER
  text
}

return {
  rendObj = ROBJ_FRAME
  minHeight = sh(25)
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  flow = FLOW_HORIZONTAL
  gap = sw(2)
  borderWidth = 3
  color = 0xFF33CCFF
  children = [
    mkPanel("no content")
    mkPanel("small content", mkContent("short content"))
    // hide any overflowing content to see correct minimum window size
    mkPanel("bigger content", mkContent("Vivamus aliquam metus ut tempus porta. Aenean quis felis ex. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas."))
    mkPanel("large content", mkContent("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed a lacinia odio, sed bibendum ligula. Nunc eu enim egestas, euismod felis vitae, dapibus ante. Vestibulum eu ex ligula. Sed cursus ligula a porta congue. Proin euismod finibus leo eu fermentum."))
  ]
}