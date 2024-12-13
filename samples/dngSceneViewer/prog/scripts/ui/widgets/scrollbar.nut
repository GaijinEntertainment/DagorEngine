from "%darg/ui_imports.nut" import *

let calcKnobColor = @(sf) (sf & S_ACTIVE) ? Color(255,255,255)
                  : (sf & S_HOVER)  ? Color(110, 120, 140, 80)
                                    : Color(110, 120, 140, 160)

let barWidth = sh(1)
let minKnobSizePart = 0.005
let barColor = Color(40, 40, 40, 160)
let calcBarSize = @(isVertical) isVertical ? [barWidth, flex()] : [flex(), barWidth]

let mkScrollbar = function(scroll_handler, orientation=null, needReservePlace=true) {
  let stateFlags = Watched(0)
  let isVertical = orientation == O_VERTICAL
  let elemSize = isVertical
    ? Computed(@() (scroll_handler.elem?.getHeight() ?? 0))
    : Computed(@() (scroll_handler.elem?.getWidth() ?? 0))
  let maxV = isVertical
    ? Computed(@() (scroll_handler.elem?.getContentHeight() ?? 0) - elemSize.get())
    : Computed(@() (scroll_handler.elem?.getContentWidth() ?? 0) - elemSize.get())
  let fValue = isVertical
    ? Computed(@() scroll_handler.elem?.getScrollOffsY() ?? 0)
    : Computed(@() scroll_handler.elem?.getScrollOffsX() ?? 0)
  let isElemFit = Computed(@() maxV.get() <= 0)

  let knob = @() {
    watch = stateFlags
    key = "knob"
    size = [flex(), flex()]
    rendObj = ROBJ_SOLID
    color = calcKnobColor(stateFlags.get())
  }
  function view() {
    let sizeMul = elemSize.get() == 0 || maxV.get() == 0 ? 1
      : elemSize.get() <= minKnobSizePart * maxV.get() ? 1.0 / maxV.get() / minKnobSizePart
      : 1.0 / elemSize.get()
    return {
      watch = [elemSize, maxV, fValue]
      size = flex()
      flow = isVertical ? FLOW_VERTICAL : FLOW_HORIZONTAL
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER

      children = [
        { size = array(2, flex(fValue.get() * sizeMul)) }
        knob
        { size = array(2, flex((maxV.get() - fValue.get()) * sizeMul)) }
      ]
    }
  }

  return {
    isElemFit
    function scrollComp() {
      if (isElemFit.get())
        return {
          watch = isElemFit
          key = scroll_handler
          behavior = Behaviors.Slider
          size = needReservePlace ? calcBarSize(isVertical) : null
        }
      return {
        watch = [isElemFit, maxV, elemSize]
        key = scroll_handler
        size = calcBarSize(isVertical)

        behavior = Behaviors.Slider
        orientation
        fValue = fValue.get()
        knob
        min = 0
        max = maxV.get()
        unit = 1
        pageScroll = (isVertical ? 1 : -1) * maxV.get() / 100.0 // TODO probably needed sync with container wheelStep option
        onChange = @(val) isVertical ? scroll_handler.scrollToY(val)
          : scroll_handler.scrollToX(val)
        onElemState = @(sf) stateFlags.set(sf)

        rendObj = ROBJ_SOLID
        color = barColor

        children = view
      }
    }
  }
}

let makeSideScroll = kwarg(function(content, scrollAlign = ALIGN_RIGHT, orientation = O_VERTICAL,
        size = flex(), maxWidth = null, maxHeight = null, needReservePlace = true, scrollHandler=null, behaviors = null) {
  scrollHandler = scrollHandler ?? ScrollHandler()

  let contentRoot = {
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
  let { isElemFit, scrollComp } = mkScrollbar(scrollHandler, orientation, needReservePlace)

  let children = scrollAlign == ALIGN_LEFT || scrollAlign == ALIGN_TOP
    ? [scrollComp, contentRoot]
    : [contentRoot, scrollComp]

  return @() {
    watch = isElemFit
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
          mkScrollbar(scrollHandler, O_VERTICAL, needReservePlace).scrollComp
        ]
      }
      mkScrollbar(scrollHandler, O_HORIZONTAL, needReservePlace).scrollComp
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
