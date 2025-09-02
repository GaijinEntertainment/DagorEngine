from "%darg/ui_imports.nut" import *

return {
  rendObj = ROBJ_FRAME
  size = static [sh(80), SIZE_TO_CONTENT]
  borderWidth = 2
  color = 0xFFCC00CC
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  minHeight = sh(3)
  flow = FLOW_HORIZONTAL
  children = [
    {
      rendObj = ROBJ_TEXTAREA
      size = static [sh(50), SIZE_TO_CONTENT]
      behavior = Behaviors.TextArea
      text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer vel suscipit justo. Nullam eleifend eget metus accumsan pharetra. Donec ac imperdiet enim, vel vehicula quam."
    }
    {
      rendObj = ROBJ_FRAME
      size = flex()
      borderWidth = 1
      color = 0xFFFFCC00
      halign = ALIGN_RIGHT

      // uncomment to see that 'place' attributes are ignored when 'flow' is set
      // container is keeping it's flex size, but all flow elements are placed strictly one by one
      flow = FLOW_VERTICAL

      children = [
        {
          rendObj = ROBJ_IMAGE
          size = sh(3)
          image = Picture("!ui/atlas#ca_cup1")
        }
        {
          rendObj = ROBJ_TEXT
          vplace = ALIGN_BOTTOM
          text = "Continue"
        }
      ]
    }
  ]
}