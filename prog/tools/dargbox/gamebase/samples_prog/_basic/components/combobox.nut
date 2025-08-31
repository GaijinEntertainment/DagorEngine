from "%darg/ui_imports.nut" import *
let comboStyle = require("combobox.style.nut")

let ComboPopupLayer = getconsttable()?.Layers.ComboPopup ?? 1

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
    get = @() value
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


function setValueByOptions(opts, wdata, wupdate){
  wupdate(findCurOption(opts, wdata).get())
}


function popupWrapper(popupContent, dropDirDown) {
  let align = dropDirDown ? ALIGN_TOP : ALIGN_BOTTOM
  let children = [
    {size = static [flex(), ph(100)]}
    {size = static [flex(), hdpx(2)]}
    popupContent
  ]

  if (!dropDirDown)
    children.reverse()

  return {
    size = flex()
    flow = FLOW_VERTICAL
    vplace = align
    valign = align
    //rendObj = ROBJ_SOLID
    //color = Color(0,100,0,50)
    children
  }
}


function dropdownBgOverlay(onClick) {
  return {
    pos = [-9000, -9000]
    size = 19999
    behavior = Behaviors.ComboPopup
    eventPassThrough = true
    onClick
  }
}


function combobox(watches, options, combo_style=comboStyle) {
  if (type(options)!="instance")
    options = Watched(options)

  let comboOpen = Watched(false)
  let group = ElemGroup()
  let stateFlags = Watched(0)
  let doClose = @() comboOpen.set(false)
  local wdata, wdisable, wupdate
  let dropDirDown = combo_style?.dropDir != "up"
  let itemHeight = options.get().len() > 0 ? calc_comp_size(combo_style.listItem(options.get()[0], @() null, false))[1] : sh(5)
  let itemGapHt = calc_comp_size(combo_style?.itemGap)[1]
  local changeVarOnListUpdate = true
  let xmbNode = combo_style?.xmbNode ?? XmbNode()

  if (type(watches) == "table") {
    wdata = watches.value
    wdisable = watches?.disable ?? {value=false}
    wupdate =  @(v) wdata.set(v)
    changeVarOnListUpdate = watches?.changeVarOnListUpdate ?? changeVarOnListUpdate
  } else {
    wdata = watches
    wdisable = {value=false}
    wupdate = @(v) wdata.set(v)
  }


  local onAttachRoot, onDetachRoot
  if (changeVarOnListUpdate) {
    function inputWatchesChangeListener(_) {
      setValueByOptions(options.get(), wdata, wupdate)
    }

    onAttachRoot = function onAttachRootImpl() {
      options.subscribe_with_nasty_disregard_of_frp_update(inputWatchesChangeListener)
      wdata.subscribe_with_nasty_disregard_of_frp_update(inputWatchesChangeListener)
    }

    onDetachRoot = function onDetachRootImpl() {
      options.unsubscribe(inputWatchesChangeListener)
      wdata.unsubscribe(inputWatchesChangeListener)
    }
  }


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
      return combo_style.listItem(text, handler, isCurrent, { xmbNode = xmbNodes[idx] })
    })

    local onAttachPopup = null
    local onDetachPopup = null
    if (combo_style?.onOpenDropDown)
      onAttachPopup = @() combo_style.onOpenDropDown(curXmbNode)
    if (combo_style?.onCloseDropDown)
      onDetachPopup = @() combo_style.onCloseDropDown(xmbNode)

    let popupContent = {
      size = FLEX_H
      rendObj = ROBJ_BOX
      fillColor = combo_style?.popupBgColor ?? Color(10,10,10)
      borderColor = combo_style?.popupBdColor ?? Color(80,80,80)
      borderWidth = combo_style?.popupBorderWidth ?? 0
      padding = combo_style?.popupBorderWidth ?? 0
      stopMouse = true
      clipChildren = true
      xmbNode = XmbContainer({scrollToEdge=true})
      children = {
        behavior = Behaviors.WheelScroll
        joystickScroll = true
        flow = FLOW_VERTICAL
        children
        gap = combo_style?.itemGap
        size = FLEX_H
        maxHeight = itemHeight*10.5 + itemGapHt*9 //this is ugly workaround with overflow of combobox size
        //we need something much more clever - we need understand how close we to the bottom\top of the screen and set limit to make all elements visible
      }

      transform = {
        pivot = [0.5, dropDirDown ? 0 : 1]
      }
      animations = popupContentAnim
      onAttach = onAttachPopup
      onDetach = onDetachPopup
    }


    return {
      zOrder = ComboPopupLayer
      size = flex()
      watch = options
      children = [
        dropdownBgOverlay(doClose)
        combo_style.closeButton(doClose)
        popupWrapper(popupContent, dropDirDown)
      ]
      transform = { pivot=[0.5, dropDirDown ? 1.1 : -0.1]}
      animations = popupWrapperAnim
    }
  }


  return function combo() {
    let labelText = findCurOption(options.get(), wdata).text
    let showDropdown = comboOpen.get() && !wdisable.get()
    let children = [
      combo_style.boxCtor({group, stateFlags, disabled=wdisable.get(), comboOpen, text=labelText}),
      showDropdown ? dropdownList : null
    ]

    let onClick = wdisable.get() ? null : @() comboOpen.set(!comboOpen.get())

    return (combo_style?.rootBaseStyle ?? {}).__merge({
      size = flex()
      //behavior = wdisable.get() ? null : Behaviors.Button
      behavior = Behaviors.Button
      watch = [comboOpen, watches?.disable, wdata, options]
      group
      onElemState = @(sf) stateFlags.set(sf)

      children
      onClick
      xmbNode

      onAttach = onAttachRoot
      onDetach = onDetachRoot
    })
  }
}


return combobox
