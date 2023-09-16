from "%darg/ui_imports.nut" import *

let cursorC = Color(180,180,180,180)

let styling = {
  cursor = Cursor({
    rendObj = ROBJ_VECTOR_CANVAS
    size = [sh(2), sh(2)]
    commands = [
      [VECTOR_WIDTH, hdpx(1)],
      [VECTOR_FILL_COLOR, cursorC],
      [VECTOR_COLOR, Color(20, 40, 70, 250)],
      [VECTOR_POLY, 0,0, 100,50, 56,56, 50,100],
    ]
  })

  Root = {
    rendObj = ROBJ_SOLID
    color = Color(30,30,30,250)
    size = [sw(100), sh(50)]
    vplace = ALIGN_CENTER
    padding = sh(2)
  }

  closeKeys = "Esc"
  leftKeys = "Left"
  rightKeys = "Right | Tab"
  activateKeys = "Space | Enter"
  closeTxt = "Close"
  maskKeys = ""
  BgOverlay = {
    rendObj = ROBJ_SOLID
    size = [sw(100), sh(100)]
    color = Color(0, 0, 0, 200)
    behavior = Behaviors.Button
    transform = {}
    animations = [
      { prop=AnimProp.opacity, from=0, to=1, duration=0.32, play=true, easing=OutCubic }
      { prop=AnimProp.scale,  from=[1, 0], to=[1,1], duration=0.25, play=true, easing=OutQuintic }

      { prop=AnimProp.opacity, from=1, to=0, duration=0.25, playFadeOut=true, easing=OutCubic }
      { prop=AnimProp.scale,  from=[1, 1], to=[1,0.5], duration=0.15, playFadeOut=true, easing=OutQuintic }
    ]
  }

  button = function(desc, on_click) {
    let buttonGrp = ElemGroup()
    let stateFlags = Watched(0)
    return function(){
      let sf = stateFlags.value
      return {
        key = desc
        behavior = Behaviors.Button
        //focusOnClick = true
        group = buttonGrp

        rendObj = ROBJ_BOX
        onElemState = @(v) stateFlags(v)
        fillColor = sf & S_ACTIVE ? Color(0,0,0,255)
          : sf & S_HOVER ? Color(90, 90, 80, 250)
          : Color(30, 30, 30, 200)

        borderWidth = sf & S_KB_FOCUS ? hdpx(2) : hdpx(1)
        onHover = function(on) { if(on) set_kb_focus(desc)}
        borderRadius = hdpx(4)
        borderColor = sf & S_KB_FOCUS
          ? Color(255,255,200,120)
          : Color(120,120,120,120)

        size = SIZE_TO_CONTENT
        margin = [sh(0.5), sh(1)]
        watch = stateFlags

        children = {
          rendObj = ROBJ_TEXT
          margin = sh(1)
          text = desc?.text ?? "???"
          group = buttonGrp
        }

        onClick = on_click
      }
    }
  }

  messageText = function(params) {
    return {
      size = flex()
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      padding = [sh(2), 0]
      children = {
        size = [flex(), SIZE_TO_CONTENT]
        rendObj = ROBJ_TEXTAREA
        behavior = Behaviors.TextArea
        halign = ALIGN_CENTER
        text = params?.text
      }
    }
  }

}

return styling
