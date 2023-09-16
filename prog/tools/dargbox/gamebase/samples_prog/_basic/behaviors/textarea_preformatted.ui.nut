from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let text = @"    Lorem   ipsum   dolor sit amet, consectetur adipiscing elit.
Sed efficitur felis tortor, sed finibus elit hendrerit et. Vivamus in tortor sagittis, tempor turpis a, laoreet augue. Aliquam maximus scelerisque elit, a elementum nulla ullamcorper eget. Sed fermentum sit amet leo eget semper. Sed at turpis sed massa fermentum dignissim. Vestibulum condimentum viverra consectetur. Donec consequat egestas efficitur. Sed id massa ligula. Nulla vel neque rutrum, imperdiet ante at, aliquet nunc. Curabitur faucibus odio sed lorem fermentum consequat. Praesent at vulputate libero. Vestibulum ultrices ligula semper, sagittis nulla scelerisque, finibus lectus. Vivamus non cursus massa, sed placerat massa. Cras a gravida neque. Maecenas tempus ac ante quis aliquet.
     Donec urna nisi, cursus eget mauris at, congue bibendum mauris. Nunc porta dui in auctor sagittis. Integer et magna in quam tristique efficitur nec vel mauris. Maecenas lobortis, turpis ut hendrerit posuere, arcu velit euismod mauris, et pretium sem dolor eget nisl. Fusce nisl mauris, varius et iaculis quis, lobortis quis mauris. Mauris dapibus fringilla turpis viverra dignissim. Donec mattis in erat eu ornare. Donec scelerisque et augue pretium lobortis. Pellentesque consectetur nunc magna, vel scelerisque diam dictum id. Nam scelerisque luctus mollis. Curabitur dignissim dolor ut arcu lacinia, id finibus dui tincidunt.
Quisque ipsum purus,  hendrerit eget  ex  eget, tempus  condimentum ligula. Morbi sed urna felis. Integer semper sollicitudin eros at aliquam. Aliquam erat volutpat. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque laoreet est in nisi euismod commodo. Sed at tellus at lacus ultricies tempor nec vel sem. Quisque urna nisi, pretium et dolor ac, tempus laoreet lorem. Aenean magna ante, tempus id lacinia non, faucibus a ante. Integer condimentum et libero at blandit. Aenean in elementum est.
"
let colored_text = "\n".concat(
  "- hex color: <color=#aa33ff> some text </color> (alfa is optional and have no effect, same as in DaGUI textarea)"
  $"- deciaml: <color={Color(200, 120, 100)}>some text</color>"
)

let function sText(txt, params={}) {
  return {
    rendObj = ROBJ_INSCRIPTION
    ellipsis = true
    color = Color(198,198,128)
    textOverflowX = TOVERFLOW_CHAR
    text=txt
  }.__update(params, {rendObj = ROBJ_INSCRIPTION})
}

return {
  rendObj = ROBJ_SOLID
  color = Color(80,80,80)
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  cursor = cursors.normal
  size = flex()
  flow = FLOW_HORIZONTAL
  padding = sh(2)
  gap = sh(2)
  children = [
    {
      flow = FLOW_VERTICAL
      halign = ALIGN_LEFT
      valign = ALIGN_CENTER
      size = [flex(4),flex()]
      gap = 10
      children = [
        sText("Default")
        {rendObj = ROBJ_FRAME size = [hdpx(507),hdpx(107)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = text behavior = Behaviors.TextArea }
        }

        sText("preformatted=true (All flags)")
        {rendObj = ROBJ_FRAME size = [hdpx(507),hdpx(107)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = text behavior = Behaviors.TextArea preformatted=true}
        }
      ]
    }
    {
      flow = FLOW_VERTICAL
      halign = ALIGN_LEFT
      valign = ALIGN_CENTER
      size = [flex(4),flex()]
      gap = 10
      children = [
        sText("preformatted=FMT_IGNORE_TAGS")
        {rendObj = ROBJ_FRAME size = [hdpx(807),hdpx(107)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = colored_text behavior = Behaviors.TextArea preformatted=FMT_IGNORE_TAGS}
        }

        sText("preformatted=FMT_KEEP_SPACES")
        {rendObj = ROBJ_FRAME size = [hdpx(807),hdpx(107)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = text behavior = Behaviors.TextArea preformatted=FMT_KEEP_SPACES}
        }

        sText("preformatted=FMT_HIDE_ELLIPSIS")
        {rendObj = ROBJ_FRAME size = [hdpx(807),hdpx(107)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = text behavior = Behaviors.TextArea preformatted=FMT_HIDE_ELLIPSIS}
        }

        sText("preformatted=FMT_NO_WRAP")
        {rendObj = ROBJ_FRAME size = [hdpx(807),hdpx(107)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = text behavior = Behaviors.TextArea preformatted=FMT_NO_WRAP}
        }

        sText("preformatted=FMT_AS_IS")
        {rendObj = ROBJ_FRAME size = [hdpx(807),hdpx(107)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = text behavior = Behaviors.TextArea preformatted=FMT_AS_IS}
        }
      ]
    }
  ]
}
