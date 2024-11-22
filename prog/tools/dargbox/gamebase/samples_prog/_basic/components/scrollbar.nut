from "%darg/ui_imports.nut" import *

let calcKnobColor = @(sf) (sf & S_ACTIVE) ? Color(255,255,255)
                  : (sf & S_HOVER)  ? Color(110, 120, 140, 80)
                                    : Color(110, 120, 140, 160)

let barWidth = sh(1)

let barColor = Color(40, 40, 40, 160)
let calcBarSize = @(axis) axis == 0 ? [flex(), barWidth] : [barWidth, flex()]

let mkScrollbar = function(scroll_handler, orientation=null, needReservePlace=true) {
  let stateFlags = Watched(0)
  let axis        = orientation == O_VERTICAL ? 1 : 0

  return function() {
    let elem = scroll_handler.elem

    if (!elem) {
      return {
        key = scroll_handler
        behavior = Behaviors.Slider
        watch = scroll_handler
        size = needReservePlace ? calcBarSize(axis) : null
      }
    }

    local contentSize, elemSize, scrollPos
    if (axis == 0) {
      contentSize = elem.getContentWidth()
      elemSize = elem.getWidth()
      scrollPos = elem.getScrollOffsX()
    }
    else {
      contentSize = elem.getContentHeight()
      elemSize = elem.getHeight()
      scrollPos = elem.getScrollOffsY()
    }

    if (contentSize <= elemSize) {
      return {
        key = scroll_handler
        behavior = Behaviors.Slider
        watch = scroll_handler
        size = needReservePlace ? calcBarSize(axis) : null
      }
    }


    let minV = 0
    let maxV = contentSize - elemSize
    let fValue = scrollPos

    let knob = {
      size = [flex(elemSize), flex(elemSize)]
      color = calcKnobColor(stateFlags.get())
      key = "knob"
      rendObj = ROBJ_SOLID
    }

    return {
      key = scroll_handler
      behavior = Behaviors.Slider
      color = barColor
      rendObj = ROBJ_SOLID

      watch = [scroll_handler, stateFlags]
      fValue

      knob
      min = minV
      max = maxV
      unit = 1

      flow = axis == 0 ? FLOW_HORIZONTAL : FLOW_VERTICAL
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER

      pageScroll = (axis == 0 ? -1 : 1) * (maxV - minV) / 100.0 // TODO probably needed sync with container wheelStep option

      orientation
      size = calcBarSize(axis)

      children = [
        {size=[flex(fValue), flex(fValue)]}
        knob
        {size=[flex(maxV-fValue), flex(maxV-fValue)]}
      ]

      onChange = @(val) axis == 0
        ? scroll_handler.scrollToX(val)
        : scroll_handler.scrollToY(val)

      onElemState = @(sf) stateFlags.set(sf)
    }
  }
}

let makeSideScroll = kwarg(function(content, scrollAlign = ALIGN_RIGHT, orientation = O_VERTICAL,
        size = flex(), maxWidth = null, maxHeight = null, needReservePlace = true, scrollHandler=null, behaviors = null) {
  scrollHandler = scrollHandler ?? ScrollHandler()

  function contentRoot() {
    return {
      size = flex()
      behavior = [].extend(behaviors ?? []).append(Behaviors.WheelScroll, Behaviors.ScrollEvent)
      scrollHandler
      wheelStep = 0.8
      orientation
      joystickScroll = true
      maxHeight
      maxWidth
      children = content
    }
  }

  let children = scrollAlign == ALIGN_LEFT || scrollAlign == ALIGN_TOP
    ? [mkScrollbar(scrollHandler, orientation, needReservePlace), contentRoot]
    : [contentRoot, mkScrollbar(scrollHandler, orientation, needReservePlace)]

  return {
    size
    maxHeight
    maxWidth
    flow = (orientation == O_VERTICAL) ? FLOW_HORIZONTAL : FLOW_VERTICAL
    clipChildren  = true
    children
  }
})


let makeHVScrolls = function(content, options = null ) {
  let {needReservePlace=true, size=flex(), behaviors=null} = options
  let scrollHandler = options?.scrollHandler ?? ScrollHandler()

  let contentRoot = {
    behavior = [].extend(behaviors ?? []).append(Behaviors.WheelScroll, Behaviors.ScrollEvent)
    scrollHandler
    joystickScroll = true
    size = flex()
    children = content
  }

  return {
    size
    flow = FLOW_VERTICAL

    children = [
      {
        size = flex()
        flow = FLOW_HORIZONTAL
        clipChildren = true
        children = [
          contentRoot
          mkScrollbar(scrollHandler, O_VERTICAL, needReservePlace)
        ]
      }
      mkScrollbar(scrollHandler, O_HORIZONTAL, needReservePlace)
    ]
  }
}


let makeVertScroll = function(content, options = null) {
  let {scrollAlign=ALIGN_RIGHT, size=flex(), needReservePlace=true, maxHeight=null, maxWidth=null, behaviors=null} = options
  return makeSideScroll({content, scrollAlign, size, needReservePlace, maxHeight, maxWidth, orientation=O_VERTICAL, behaviors})
}


let makeHorizScroll = function(content, options=null) {
  let {scrollAlign=ALIGN_RIGHT, size=flex(), needReservePlace=true, maxHeight=null, maxWidth=null, behaviors=null} = options
  return makeSideScroll({content, scrollAlign, size, needReservePlace, maxHeight, maxWidth, orientation=O_HORIZONTAL, behaviors})
}


return {
  mkScrollbar
  makeHorizScroll
  makeVertScroll
  makeHVScrolls
  makeSideScroll
}
