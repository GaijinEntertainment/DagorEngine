from "%darg/ui_imports.nut" import *

let issues = @"
<color=#ffffff>DARG issues</color>:
  - <color=#ff9999>incorrect scrollbar layout calculation</color>
  - <color=#ff9999>incorrect scrollbar behaviour on drag not on knob</color>
  - more gamepad tweaks: remember direction for some seconds, use flow or layout hints
  - text: empty text is incorrect height (0 instead of text height) OR size=[SIZE_BY_CONTENT,fontH(<fontname>)]
  - text: textFitIntoBox - replace with bool property
  - replace last char in input password with delay

  - textArea: ellipsis overflowed textarea not correctly displayed when it is less than one line in height
  - textArea: ellipsis overflow should be truncated by char
  - stext: incorrect text placing in ALIGN_CENTER and ALIGN_BOTTOM, unpredicateble textsize
    (probably it would be better to set what to place where - baseline, H height, x-height, center of H, center of X, center)

  - animprops for coloring and everything

<color=#ffffff> darg features:</color>
   - size=valid value == rectangle (as it works for flex and size_to_content)
   - coloring: hueTint
   - scripted behaviors
   - touchpad gestures (zoom, scroll)
   - rich text (underline, bold, italic, strike, captilize, lowered, color tags, subscript, superscript)
   - Mapped mesh rendObj (minimap)

<color=#ffffff>darg tools</color>:
   - inspector of layout&rendobjs
   - state inspector
   - stack of properties inspector
   - renderproperty inspector
"

let todo = @"
<color=#ffffff>samples</color>:
   <color=#ffffff>simple</color>:
   - (60min) key for diff virtual dom
   - (60min) easing functions
   - input numeric
   - slider
   - simple layouts: table
   - tooltip

  <color=#ffffff>Advanced</color>:
   - (2h) simple HUDs: rtPropertyUpdate, transform
   - layout animations (probably not working?)
   - accordeon/sliding bar/collapsible bar
   - tree control
   - property grid

<color=#ffffff>stdlibrary [+ samples +docs]</color>:
  - tabs
  - radiobutton
  - slider
  - colorpicker
  - propertygrid
  - treecontrol

"
return {
  size =flex()
  rendObj = ROBJ_SOLID
  color=Color(55,50,50)
  valign=ALIGN_TOP
  padding = sh(2)
  gap = sh(2)
  flow = FLOW_HORIZONTAL
  children =  [
    {
      size = flex() rendObj = ROBJ_TEXTAREA behavior = [Behaviors.TextArea, Behaviors.WheelScroll]
      preformatted=FMT_NO_WRAP | FMT_KEEP_SPACES
      color =Color(150,150,150)
      text = issues
    }
    {
      size = flex() rendObj = ROBJ_TEXTAREA behavior = [Behaviors.TextArea, Behaviors.WheelScroll]
      preformatted=FMT_NO_WRAP | FMT_KEEP_SPACES
      color =Color(150,150,150)
      text = todo
    }
  ]
}
