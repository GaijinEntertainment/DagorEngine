from "%darg/ui_imports.nut" import *

let mkIcon = @() {
  rendObj = ROBJ_IMAGE
  size = [hdpx(40), hdpx(40)]
  keepAspect = true
  image = Picture("ui/ca_cup1")
}

let item = @(text, hasIcon = false) {
  rendObj = ROBJ_SOLID
  size = [flex(), SIZE_TO_CONTENT]
  maxWidth = SIZE_TO_CONTENT
  valign = ALIGN_CENTER
  flow = FLOW_HORIZONTAL // remove this and text will appear
  gap = hdpx(5)
  padding = hdpx(5)
  color = Color(0, 0, 0)
  children = [
    hasIcon ? mkIcon() : null
    {
      rendObj = ROBJ_TEXTAREA
      size = [flex(), SIZE_TO_CONTENT]
      maxWidth = SIZE_TO_CONTENT
      behavior = Behaviors.TextArea
      text
    }
  ]
}

// this workaround only shows approach to make task done but it isn't solve
// problem and even doesn't expand external container to the self size
let function itemTrick(text, hasIcon = false) {
  let textArea = {
    rendObj = ROBJ_TEXTAREA
    size = [flex(), SIZE_TO_CONTENT]
    maxWidth = SIZE_TO_CONTENT
    valign = ALIGN_CENTER
    behavior = Behaviors.TextArea
    text
  }
  if (hasIcon) {
    let icon  = mkIcon()
    let iconSize = calc_comp_size(icon)[0] + hdpx(5)
    icon.pos <- [-iconSize, 0]
    textArea.children <- icon
    textArea.pos <- [iconSize / 2, 0]
  }
  return {
    rendObj = ROBJ_SOLID
    size = [flex(), SIZE_TO_CONTENT]
    maxWidth = SIZE_TO_CONTENT
    padding = hdpx(5)
    color = Color(0, 0, 0)
    children = textArea
  }
}

return {
  rendObj = ROBJ_SOLID
  size = [sw(25), SIZE_TO_CONTENT]
  vplace = ALIGN_CENTER
  hplace = ALIGN_CENTER
  halign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = 10
  padding = 10
  color = Color(55, 88, 55)
  children = [
    item("Simple text")
    item("This text should wrap taking up the available space specified by the outer container")
    item("A little bit longer text with icon", true) // BUG: text is hiding by icon element
    item("One more long line to make sure the text block is sized by its content and not stretched to the width of the outer container")
    itemTrick("This is tricky workaround to append an icon to the long multilined text. Icon have vertical alignment to the text field", true)
    itemTrick("Simple text looks the same")
    itemTrick("But icon size doesn't affect to the text alignment", true)
  ]
}