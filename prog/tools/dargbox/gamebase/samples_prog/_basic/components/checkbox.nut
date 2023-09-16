from "%darg/ui_imports.nut" import *

let defStyle = {}
let defTextStyle = {}
let defBoxStyle = {
  padding = hdpx(3)
  borderWidth = hdpx(1)
  borderRadius = hdpx(2)
  color = Color(150,150,150)
  hoverColor = Color(250,250,250)
}

let function defMkText(text, state=null, stateFlags=null, style = null){
  return @(){
    watch = [state, stateFlags]
    rendObj = ROBJ_TEXT
    text = text
  }.__update(style ?? {})
}

let function defMkBox(size, state=null, stateFlags=null, boxStyle=null){
  let color = boxStyle?.color ?? defBoxStyle.color
  let hoverColor = boxStyle?.hoverColor ?? defBoxStyle.hoverColor
  let borderRadius = boxStyle?.borderRadius ?? defBoxStyle.borderRadius
  return function() {
    let sf = stateFlags?.value ?? 0
    return {
      rendObj = ROBJ_BOX
      size = size
      watch = stateFlags
      borderColor = sf ? hoverColor : color
      children = @(){
        watch = state
        rendObj = ROBJ_BOX
        size = flex()
        borderColor = 0
        fillColor = !state?.value
          ? sf & S_HOVER ? color : 0
          : sf & S_HOVER ? hoverColor : color
        borderRadius = borderRadius
      }
    }.__update(boxStyle ?? {})
  }
}

let function checkboxCtor(state, text = null, style = defStyle, textCtor = defMkText, textStyle = defTextStyle, boxCtor = defMkBox, boxStyle = defBoxStyle){
  let stateFlags = Watched(0)
  let h =calc_comp_size(textCtor("h"))
  let hWidth = h[0]
  let hHeight = h[1]
  let box = boxCtor([hHeight, hHeight], state, stateFlags, boxStyle)
  let label = textCtor(text, state, stateFlags, textStyle )
  return function checkbox(){
    return {
      behavior = Behaviors.Button
      onClick = @() state(!state.value)
      onElemState = @(sf) stateFlags(sf)
      flow = FLOW_HORIZONTAL
      gap = hWidth
      children = [box, label]
    }.__update(style)
  }
}

return {
  checkbox = kwarg(checkboxCtor)
  defStyle
  defTextStyle
  defBoxStyle
}