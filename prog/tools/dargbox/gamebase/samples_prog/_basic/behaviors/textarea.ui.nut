from "%darg/ui_imports.nut" import *

/*
  KNOWN ISSUES:
    - ellipsis should be done by character by default (or may be even always)
*/

let cursors = require("samples_prog/_cursors.nut")

let text = @"Lorem ipsum dolor sit amet, consectetur adipiscing elit.
Sed efficitur felis tortor, sed finibus elit hendrerit et. Vivamus in tortor sagittis, tempor turpis a, laoreet augue. Aliquam maximus scelerisque elit, a elementum nulla ullamcorper eget. Sed fermentum sit amet leo eget semper. Sed at turpis sed massa fermentum dignissim. Vestibulum condimentum viverra consectetur. Donec consequat egestas efficitur. Sed id massa ligula. Nulla vel neque rutrum, imperdiet ante at, aliquet nunc. Curabitur faucibus odio sed lorem fermentum consequat. Praesent at vulputate libero. Vestibulum ultrices ligula semper, sagittis nulla scelerisque, finibus lectus. Vivamus non cursus massa, sed placerat massa. Cras a gravida neque. Maecenas tempus ac ante quis aliquet.
Donec urna nisi, cursus eget mauris at, congue bibendum mauris. Nunc porta dui in auctor sagittis. Integer et magna in quam tristique efficitur nec vel mauris. Maecenas lobortis, turpis ut hendrerit posuere, arcu velit euismod mauris, et pretium sem dolor eget nisl. Fusce nisl mauris, varius et iaculis quis, lobortis quis mauris. Mauris dapibus fringilla turpis viverra dignissim. Donec mattis in erat eu ornare. Donec scelerisque et augue pretium lobortis. Pellentesque consectetur nunc magna, vel scelerisque diam dictum id. Nam scelerisque luctus mollis. Curabitur dignissim dolor ut arcu lacinia, id finibus dui tincidunt.
Quisque ipsum purus,  hendrerit eget  ex  eget, tempus  condimentum ligula. Morbi sed urna felis. Integer semper sollicitudin eros at aliquam. Aliquam erat volutpat. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque laoreet est in nisi euismod commodo. Sed at tellus at lacus ultricies tempor nec vel sem. Quisque urna nisi, pretium et dolor ac, tempus laoreet lorem. Aenean magna ante, tempus id lacinia non, faucibus a ante. Integer condimentum et libero at blandit. Aenean in elementum est.
"
let colored_text = "\n".concat(
  $"- hex color: <color=#aa33ff> some text </color> (alfa is optional and have no effect, same as in DaGUI textarea); deciaml: <color={Color(200, 120, 100)}>some text</color>,"
  "named: <color=@red>some text</color>, <header>HEADER</header>"
)

let function sText(txt, params={}) {
  return {
    ellipsis = true
    color = Color(198,198,128)
    textOverflowX = TOVERFLOW_CHAR
    text=txt
  }.__update(params, {rendObj = ROBJ_TEXT})
}

let function textarea(txt, params={}) {
  return {
    rendObj = ROBJ_TEXTAREA
    color = Color(198,198,128)
    textOverflowX = TOVERFLOW_CLIP
    textOverflowY = TOVERFLOW_LINE
    text = txt
    behavior = Behaviors.TextArea
  }.__update(params, {rendObj = ROBJ_TEXTAREA})
}

let textAreaFrameState = Watched({
  size = [sw(40), 60]
  pos  = [0, 0]
})

let function textAreaContainer(txt, params={}) {
  let ta = {
    rendObj = ROBJ_TEXTAREA
    textOverflowY = TOVERFLOW_LINE
    textOverflowX = TOVERFLOW_CHAR
    halign = ALIGN_LEFT
    ellipsis = true
    ellipsisSepLine = true
    text = txt
    behavior = [Behaviors.TextArea, Behaviors.WheelScroll]
    size = [flex(), flex()]
  }
  return @(){
    rendObj = ROBJ_FRAME
    color = Color(200,100,100)
    children = ta
    padding = hdpx(10)
    size = textAreaFrameState.value.size
    valign = ALIGN_BOTTOM
    behavior = Behaviors.MoveResize
    moveResizeCursors = cursors.moveResizeCursors
    watch = textAreaFrameState
    onMoveResize = function(dx, dy, dw, dh) {
      let w = textAreaFrameState.value
      w.pos = [w.pos[0]+dx, w.pos[1]+dy]
      w.size = [max(5, w.size[0]+dw), max(20, w.size[1]+dh)]
      return w
    }
  }.__update(params, {children=ta})
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
      size = [flex(4),sh(80)]
      gap = 10
      children = [
        sText("Color tags and custom tags in textarea")
        {rendObj = ROBJ_FRAME size = [hdpx(807),hdpx(107)] padding=hdpx(5)
           children = {
             size = flex() rendObj = ROBJ_TEXTAREA text = colored_text
             tagsTable = {
               header = {
                 color = Color(0,200,0)
                 fontSize = hdpx(30)
               }
             }
             colorTable = {
               red = Color(220,40,40)
             }
             behavior = [Behaviors.TextArea, Behaviors.WheelScroll]
          }
        }
        sText("Default+scrollable by wheel text area")
        {rendObj = ROBJ_FRAME size = [hdpx(507),hdpx(107)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = text behavior = [Behaviors.TextArea, Behaviors.WheelScroll] }
        }

        sText("Scrollable text area, with lineSpacing=2px, parSpacing = 10, indent on new paragraph = 10")
        {rendObj = ROBJ_FRAME size = [hdpx(450),hdpx(97)]  padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = text behavior = [Behaviors.TextArea, Behaviors.WheelScroll] lineSpacing=2, parSpacing=10, indent=10 }
        }

        sText("Scrollable text area, with halign=ALIGN_CENTER")
        {rendObj = ROBJ_FRAME size = [hdpx(250),hdpx(100)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = text behavior = [Behaviors.TextArea, Behaviors.WheelScroll] halign=ALIGN_CENTER }
        }
        textarea("Text area with valign=ALIGN_CENTER",{size=[flex(),SIZE_TO_CONTENT]})
        {rendObj = ROBJ_FRAME size = [hdpx(250),hdpx(70)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = "small text" behavior = [Behaviors.TextArea] valign=ALIGN_CENTER}
        }
        sText("Scrollable by wheel text area with split by line")
        {rendObj = ROBJ_FRAME size = [hdpx(707),hdpx(90)] padding=hdpx(5)
           children = { size = flex() rendObj = ROBJ_TEXTAREA text = text behavior = [Behaviors.TextArea, Behaviors.WheelScroll] textOverflowY = TOVERFLOW_CLIP ellipsis = true textOverflowX= TOVERFLOW_CHAR }
        }
        sText("Resizable and Scrollable by wheel text area with split by line")
        textAreaContainer(text)
      ]
    }
    {
      flow = FLOW_VERTICAL
      halign = ALIGN_CENTER
      size = flex(3)
      children = [
        {
          size = flex() rendObj = ROBJ_TEXTAREA  behavior = [Behaviors.TextArea, Behaviors.WheelScroll] color=Color(150,150,120)
          text = @"<color=#99FFFF>Available properties</color>:
          - text = //miltilined text
          - halign = ALIGN_TOP(default) | ALIGN_RIGHT | ALIGN_CENTER
          - valign = ALIGN_TOP(default) | ALIGN_BOTTOM | ALIGN_CENTER
          - textOverflowY = TOVERFLOW_LINE | TOVERFLOW_CLIP //truncate text by lines on vertical overflow or just clip text
          - textOverflowX = TOVERFLOW_CHAR (not working!) | TOVERFLOW_CLIP | TOVERFLOW_WORD //truncate lines on horizontal overflow by words, chars or just in any place
          - ellipsis = true //show ellipsis if text is truncated
          - lineSpacing // extra spacing between lines
          - parSpacing  //spacing between paragraphs
          - indent //indent on new lines
          - color //color
          - padding //padding of textarea. Can be padding = 100 | sh(2), padding=[10,10,10,10] - [top, right, bottom, left]
          - monoWidth //monowidth output. can be character or size monWidth='W' or monowidth=20

          <color=#ff5555>known issues </color>:
          - textOverflowX - not working
          - textOverflowY - works only as TOVERFLOW_LINES

          "
        }
        {
          size = SIZE_TO_CONTENT
          rendObj = ROBJ_FRAME
          hplace = ALIGN_CENTER
          halign = ALIGN_CENTER
          flow = FLOW_VERTICAL
          padding = 2
          gap = { size = [pw(70), 1], rendObj = ROBJ_SOLID, color = Color(110, 110, 1100, 150), margin = 1 }
          children = [
            {
              rendObj = ROBJ_TEXTAREA
              behavior = Behaviors.TextArea
              text = "test of textarea calc"
            }
            {
              maxWidth = hdpx(300)
              rendObj = ROBJ_TEXTAREA
              behavior = Behaviors.TextArea
              text = "text area with fixed maxWidth = hdpx(300)"
            }
            {
              maxWidth = pw(50)
              rendObj = ROBJ_TEXTAREA
              behavior = Behaviors.TextArea
              text = "text area with maxWidth pw(50)"
            }
            {
              rendObj = ROBJ_TEXTAREA
              behavior = Behaviors.TextArea
              text = "text area without size limits but a bit long."
            }
            {
              size = [hdpx(200), SIZE_TO_CONTENT]
              rendObj = ROBJ_TEXTAREA
              behavior = Behaviors.TextArea
              text = "text area with fixed width = hdpx(200)"
            }
            {
              size = [pw(50), SIZE_TO_CONTENT]
              rendObj = ROBJ_TEXTAREA
              behavior = Behaviors.TextArea
              text = "text area with width = pw(50)"
            }
          ]
        }
      ]
    }
  ]
}
