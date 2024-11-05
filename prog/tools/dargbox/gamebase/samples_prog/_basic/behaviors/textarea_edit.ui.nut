from "%darg/ui_imports.nut" import *
from "math" import max

let cursors = require("samples_prog/_cursors.nut")

/*
let editableText = EditableText(@"Lorem ipsum

dolor")
*/


let editableText = EditableText(@"Lorem ipsum
dolorsitametconsecteturadipiscingelitSedefficitur
felis tortor, sed finibus elit hendrerit et. Vivamus in tortor sagittis, tempor turpis a, laoreet augue. Aliquam maximus scelerisque elit, a elementum nulla ullamcorper eget. Sed fermentum sit amet leo eget semper. Sed at turpis sed massa fermentum dignissim. Vestibulum condimentum viverra consectetur. Donec consequat egestas efficitur. Sed id massa ligula. Nulla vel neque rutrum, imperdiet ante at, aliquet nunc. Curabitur faucibus odio sed lorem fermentum consequat. Praesent at vulputate libero. Vestibulum ultrices ligula semper, sagittis nulla scelerisque, finibus lectus. Vivamus non cursus massa, sed placerat massa. Cras a gravida neque. Maecenas tempus ac ante quis aliquet.
Donec urna nisi, cursus eget mauris at, congue bibendum mauris. Nunc porta dui in auctor sagittis. Integer et magna in quam tristique efficitur nec vel mauris. Maecenas lobortis, turpis ut hendrerit posuere, arcu velit euismod mauris, et pretium sem dolor eget nisl. Fusce nisl mauris, varius et iaculis quis, lobortis quis mauris. Mauris dapibus fringilla turpis viverra dignissim. Donec mattis in erat eu ornare. Donec scelerisque et augue pretium lobortis. Pellentesque consectetur nunc magna, vel scelerisque diam dictum id. Nam scelerisque luctus mollis. Curabitur dignissim dolor ut arcu lacinia, id finibus dui tincidunt.
Quisque ipsum purus,  hendrerit eget  ex  eget, tempus  condimentum ligula. Morbi sed urna felis. Integer semper sollicitudin eros at aliquam. Aliquam erat volutpat. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque laoreet est in nisi euismod commodo. Sed at tellus at lacus ultricies tempor nec vel sem. Quisque urna nisi, pretium et dolor ac, tempus laoreet lorem. Aenean magna ante, tempus id lacinia non, faucibus a ante. Integer condimentum et libero at blandit. Aenean in elementum est.")


let lenWatched = Watched(editableText.text.len())



return {
  rendObj = ROBJ_SOLID
  color = Color(80,80,80)
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  cursor = cursors.normal
  size = flex()
  flow = FLOW_VERTICAL
  padding = sh(2)
  gap = sh(2)
  children = [
    {
      size = [sh(80), sh(50)]
      //size = [sh(30), sh(30)]
      rendObj = ROBJ_BOX
      borderWidth = 2
      padding = hdpx(10)
      borderColor = Color(200, 200, 200)
      fillColor = Color(30, 40, 50)

      children = {
        size = flex()
        rendObj = ROBJ_TEXTAREA
        behavior = [Behaviors.TextAreaEdit, Behaviors.WheelScroll]
        color = Color(150,150,120)

        editableText

        function onChange(etext) {
          let s = etext.text
          lenWatched.set(s.len())
        }
      }
    }
    @() {
      watch = lenWatched
      rendObj = ROBJ_TEXT
      text = $"Text length = {lenWatched.value}"
    }
  ]

  hotkeys = [
    ["F3", @() editableText.text = "Brand new text"]
  ]
}
