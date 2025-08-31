from "%darg/ui_imports.nut" import *

let defStyle = {
  elemStyle = {
    textCommonColor = Color(200,200,200),
    textActiveColor = Color(0,0,0),
    textHoverColor = Color(0,0,0),
    borderColor = Color(120,120,120),
    borderRadius = hdpx(5),
    borderWidth=hdpx(1),
    bkgActiveColor = Color(255,255,255),
    bkgHoverColor=Color(200,200,200),
    bkgNormalColor = Color(0,0,0,255)
    padding = hdpx(5)
  }
  rootStyle = {
    flow = FLOW_HORIZONTAL
    size = SIZE_TO_CONTENT
  }
}


let mkSelItem = @(state, onClickCtor=null, isCurrent=null, textCtor=null, elemCtor = null, style=null) elemCtor==null ? function selItem(p, idx, list){
  let stateFlags = Watched(0)
  isCurrent = isCurrent ?? @(pp, _idx) pp==state.get()
  let onClick = onClickCtor!=null ? onClickCtor(p, idx) : @() state.set(p)
  let text = textCtor != null ? textCtor(p, idx, stateFlags) : p
  let {textOvr = {}, textCommonColor, textActiveColor, textHoverColor, borderColor, borderRadius, borderWidth,
        bkgActiveColor, bkgHoverColor, bkgNormalColor, padding} = defStyle.elemStyle.__merge(style ?? {})
  return function(){
    let selected = isCurrent(p, idx)
    local nBw = borderWidth
    if (list.len() > 2) {
      if (idx != list.len()-1 && idx != 0)
        nBw = [borderWidth,0,borderWidth,borderWidth]
      if (idx == 1)
        nBw = [borderWidth,0,borderWidth,0]
    }
    return {
      size = SIZE_TO_CONTENT
      rendObj = ROBJ_BOX
      onElemState = @(sf) stateFlags.set(sf)
      behavior = Behaviors.Button
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      padding
      stopHover = true
      watch = [stateFlags, state]
      children = {
        rendObj = ROBJ_TEXT, text=text,
        color = (stateFlags.get() & S_HOVER)
          ? textHoverColor
          : selected
            ? textActiveColor
            : textCommonColor,
        padding = borderRadius
      }.__update(textOvr)
      onClick
      borderColor
      borderWidth = nBw
      borderRadius = list.len()==1 || (borderRadius ?? 0)==0
        ? borderRadius
        : idx==0
          ? [borderRadius, 0, 0, borderRadius]
          : idx==list.len()-1
            ? [0,borderRadius, borderRadius, 0]
            : 0
      fillColor = stateFlags.get() & S_HOVER
        ? bkgActiveColor
        : selected
          ? bkgHoverColor
          : bkgNormalColor
      xmbNode = XmbNode()
    }
  }
}  : elemCtor

let select = kwarg(function selectImpl(state, options, onClickCtor=null, isCurrent=null, textCtor=null, elemCtor=null, elem_style=null, root_style=null, flow = FLOW_HORIZONTAL){
  let selItem = mkSelItem(state, onClickCtor, isCurrent, textCtor, elemCtor, elem_style)
  return function(){
    return {
      size = SIZE_TO_CONTENT
      flow = flow
      children = options.map(selItem)
      xmbNode = XmbNode()
    }.__update(root_style ?? defStyle.rootStyle)
  }
})

return {select}