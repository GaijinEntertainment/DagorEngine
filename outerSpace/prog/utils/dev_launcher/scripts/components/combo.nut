from "%darg/ui_imports.nut" import *
import "style.nut" as style

let {setTooltip} = require("cursors.nut")

let defHeight = style.defControlHeight

function listItem(text, action, is_current, _params={}) {
  let stateFlags = Watched(0)
  let xmbNode = XmbNode()

  return function() {
    let sf = stateFlags.get()
    let textColor = sf & S_HOVER ? style.BTN_TXT_HOVER : (is_current ? Color(255,255,235) : style.BTN_TXT_NORMAL)

    return {
      behavior = [Behaviors.Button, Behaviors.Marquee]
      size = [flex(), SIZE_TO_CONTENT]
      xmbNode
      watch = stateFlags

      onClick = action
      onElemState = @(s) stateFlags.set(s)
      rendObj = ROBJ_SOLID
      color = sf & S_HOVER ? style.BTN_BG_HOVER : style.BTN_BG_NORMAL

      children = {
        rendObj = ROBJ_TEXT
        margin = sh(0.5)
        text
        color = textColor
      }
    }
  }
}


function closeButton(action) {
  return {
    size = flex()
    behavior = Behaviors.Button
    onClick = action
    hotkeys = [["Esc"]]
    rendObj = ROBJ_FRAME
    borderWidth = hdpx(1)
    color = Color(250,200,50,200)
  }
}


let boxCtor = kwarg(function(text, stateFlags=null, disabled=false, group=null) {
  stateFlags = stateFlags ?? Watched(0)
  return function(){
    let color = stateFlags.get() & S_HOVER ? style.BTN_TXT_HOVER : (disabled ? style.BTN_TXT_DISABLED : style.BTN_TXT_NORMAL)
    let labelText = {
      group
      rendObj = ROBJ_TEXT
      behavior = Behaviors.Marquee
      text
      key = text
      color
      size = [flex(), SIZE_TO_CONTENT]
    }


    let popupArrow = {
      rendObj = ROBJ_VECTOR_CANVAS
      size = [defHeight/3,defHeight/3]
      margin = sh(0.5)
      color
      commands = [
        [VECTOR_FILL_COLOR, color],
        [VECTOR_WIDTH, hdpx(1)],
        [VECTOR_POLY, 0,0, 50,100, 100,0],
      ]
    }

    return {
      size = [flex(), SIZE_TO_CONTENT]
      flow = FLOW_HORIZONTAL
      padding = [style.NORMAL_FONT_SIZE/5, style.NORMAL_FONT_SIZE/2]
      valign = ALIGN_CENTER
      watch = stateFlags
      children = [
        labelText
        popupArrow
      ]
    }
  }
})


function onOpenDropDown(_itemXmbNode) {
  gui_scene.setXmbFocus(null)
}


let comboStyle = {
  popupBgColor = Color(20, 30, 36)
  popupBdColor = Color(90,90,90)
  popupBorderWidth = hdpx(1)
  itemGap = {rendObj=ROBJ_FRAME size=[flex(),hdpx(1)] color=Color(90,90,90)}
}

let ComboPopupLayer = getconsttable()?["ComboPopup"] ?? 1

let popupContentAnim = [
  { prop=AnimProp.opacity, from=0, to=1, duration=0.12, play=true, easing=InOutQuad }
  { prop=AnimProp.scale, from=[1,0], to=[1,1], duration=0.12, play=true, easing=InOutQuad }
]

let popupWrapperAnim = [
  { prop=AnimProp.opacity, from=1, to=0, duration=0.15, playFadeOut=true}
  { prop=AnimProp.scale, from=[1,1], to=[1,0], duration=0.15, playFadeOut=true, easing=OutQuad}
]


function itemToOption(item, wdata){
  let tp = type(item)
  local value
  local text
  local isCurrent
  if (tp == "array") {
    value = item[0]
    text  = item[1]
    isCurrent = wdata.get()==value
  } else if (tp == "instance") {
    value = item.value()
    text  = item.tostring()
    isCurrent = item.isCurrent()
  } else if (item == null) {
    value = null
    text = "NULL"
    isCurrent = false
  } else {
    value = item
    text = value.tostring()
    isCurrent = wdata.get()==value
  }
  return {
    value
    isCurrent
    text
  }
}


function findCurOption(opts, wdata){
  local found
  foreach (item in opts) {
    let f = itemToOption(item, wdata)
    let {value, isCurrent} = f
    if (wdata.get() == value || isCurrent) {
      found = f
      break
    }
  }
  return found!=null ? found : itemToOption(opts?[0], wdata)
}


function popupWrapper(popupContent, dropDirDown) {
  let align = dropDirDown ? ALIGN_TOP : ALIGN_BOTTOM
  let children = [
    {size = [flex(), ph(100)]}
    //{size = [flex(), hdpx(20)]}
    popupContent
  ].append({rendObj = ROBJ_BOX borderRadius = [0,0,hdpx(6),hdpx(6)] fillColor = Color(0,0,0,80), size = [flex(), hdpx(10)]})

  if (!dropDirDown)
    children.reverse()
  return {
    size = flex()
    flow = FLOW_VERTICAL
    vplace = align
    valign = align
    transform = {
      translate = [hdpx(20), -hdpx(10)]
    }
    //rendObj = ROBJ_SOLID
    //color = Color(0,100,0,50)
    children
  }
}


function dropdownBgOverlay(onClick) {
  return {
    pos = [-9000, -9000]
    size = [19999, 19999]
    behavior = Behaviors.ComboPopup
    color = Color(0,0,0, 100)
    rendObj = ROBJ_SOLID
    eventPassThrough = true
    onClick
  }
}

function combobox(watches, options, tooltip=null) {
  if (!(options instanceof Watched))
    options = Watched(options)
  let dropDirDown = Watched(true)//update onAttach from pos on screen
  let comboOpen = Watched(false)
  let group = ElemGroup()
  let stateFlags = Watched(0)
  let doClose = @() comboOpen.set(false)
  local wdata, wdisable, wupdate
  let itemHeight = options.get().len() > 0 ? calc_comp_size(listItem(options.get()[0], @() null, false))[1] : sh(5)
  let itemGapHt = calc_comp_size(comboStyle?.itemGap)[1]
  let xmbNode = comboStyle?.xmbNode ?? XmbNode()

  wdata = watches.value
  wdisable = watches?.disable ?? Watched(false)
  wupdate = watches?.update ?? @(v) wdata(v)


  function dropdownList() {
    let xmbNodes = options.get().map(@(_) XmbNode())
    local curXmbNode = xmbNodes?[0]
    let children = options.get().map(function(item, idx) {
      let {value, text, isCurrent} = itemToOption(item, wdata)
      if (isCurrent)
        curXmbNode = xmbNodes[idx]

      function handler() {
        wupdate(value)
        comboOpen.set(false)
      }
      return listItem(text, handler, isCurrent, { xmbNode = xmbNodes[idx] })
    })

    let onAttachPopup = @() onOpenDropDown(curXmbNode)
    //let onDetachPopup = @() onCloseDropDown(xmbNode)

    let popupContent = {
      size = [flex(), SIZE_TO_CONTENT]
      rendObj = ROBJ_BOX
      fillColor = comboStyle.popupBgColor
      borderColor = comboStyle.popupBdColor
      borderWidth = comboStyle.popupBorderWidth
      padding = comboStyle.popupBorderWidth
      stopMouse = true
      clipChildren = true
      xmbNode = XmbContainer({scrollToEdge=true})
      children = {
        behavior = Behaviors.WheelScroll
        joystickScroll = true
        flow = FLOW_VERTICAL
        children
        gap = comboStyle?.itemGap
        size = [flex(), SIZE_TO_CONTENT]
        maxHeight = itemHeight*10.5 + itemGapHt*9 //this is ugly workaround with overflow of combobox size
        //we need something much more clever - we need understand how close we to the bottom\top of the screen and set limit to make all elements visible
      }

      transform = {
        pivot = [0.5, 0]
      }
      animations = popupContentAnim
      onAttach = onAttachPopup
      //onDetach = onDetachPopup
    }


    return {
      zOrder = ComboPopupLayer
      size = flex()
      watch = [options, dropDirDown]
      children = [
        dropdownBgOverlay(doClose)
        closeButton(doClose)
        popupWrapper(popupContent, dropDirDown.get())
      ]
      transform = { pivot=[0.5, dropDirDown.get() ? 1.1 : -0.1]}
      animations = popupWrapperAnim
    }
  }


  return function() {
    let labelText = findCurOption(options.get(), wdata).text
    let showDropdown = comboOpen.get() && !wdisable.get()
    let children = [
      boxCtor({group, stateFlags, disabled=wdisable.get(), text=labelText}),
      showDropdown ? dropdownList : null
    ]

    let onClick = wdisable.get() ? null : @() comboOpen.modify(@(v) !v)

    return {
      size = [style.SelectWidth, SIZE_TO_CONTENT]
      //behavior = wdisable.value ? null : Behaviors.Button
      behavior = Behaviors.Button
      onHover = tooltip != null ? @(on) setTooltip(on ? tooltip : null) : null
      watch = [stateFlags, comboOpen, watches?.disable, wdata, options]
      rendObj = ROBJ_BOX
      borderWidth = hdpx(1)
      borderColor = style.BTN_BD_NORMAL
      fillColor = stateFlags.get() & S_HOVER ? style.BTN_BG_HOVER : style.BTN_BG_NORMAL
      group
      onElemState = @(sf) stateFlags.set(sf)
      children
      onClick
      xmbNode

//      onAttach
//      onDetach
    }
  }
}


function mkCombo(value, allValues, title, tooltip = null) {
  let toolTip = tooltip ?? value?.tooltip
  return function() {
    return {
      flow = FLOW_HORIZONTAL
      size = [flex(), SIZE_TO_CONTENT]
      valign = ALIGN_CENTER
      children = [
        title == null ? null :
        {
          rendObj = ROBJ_TEXT
          color = style.LABEL_TXT
          text = title
        }
        {size = [flex(),0]}
        combobox({value}, allValues, toolTip)
      ]
    }
  }
}
return {mkCombo}