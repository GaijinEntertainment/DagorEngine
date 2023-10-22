from "daRg" import *
from "%sqstd/frp.nut" import *

let defStyling = {
  Bar = function(has_scroll) {
    if (has_scroll) {
      return class {
        rendObj = ROBJ_SOLID
        color = Color(40, 40, 40, 160)
        _width = sh(1)
        _height = sh(1)
      }
    }
    else return class{
        _width = sh(1)
        _height = sh(1)
    }
  }
  Knob = class {
    rendObj = ROBJ_SOLID
    colorCalc = @(sf) sf & S_ACTIVE ? Color(255,255,255)
      : sf & S_HOVER ? Color(110, 120, 140, 80)
      : Color(110, 120, 140, 160)
  }

  ContentRoot = class {
    size = flex()
  }
}


let function resolveBarClass(bar, has_scroll) {
  if (type(bar) == "function") {
    return bar(has_scroll)
  }
  return bar
}

let function calcBarSize(bar_class, axis) {
  return axis == 0 ? [flex(), bar_class._height] : [bar_class._width, flex()]
}


let function scrollbar(scroll_handler, options={}) {
  let stateFlags = Watched(0)
  let styling   = options?.styling ?? defStyling
  let barClass  = options?.barStyle ?? styling.Bar
  let knobClass = options?.knobStyle ?? styling.Knob

  let orientation = options?.orientation ?? O_VERTICAL
  let axis        = orientation == O_VERTICAL ? 1 : 0

  return function() {
    let elem = scroll_handler.elem

    if (!elem) {
      let cls = resolveBarClass(barClass, false)
      return class extends cls {
        key = scroll_handler
        behavior = Behaviors.Slider
        watch = scroll_handler
        size = (options?.needReservePlace ?? true) ? calcBarSize(cls, axis) : null
      }
    }

    local contentSize, elemSize, scrollPos
    if (axis == 0) {
      contentSize = elem.getContentWidth()
      elemSize = elem.getWidth()
      scrollPos = elem.getScrollOffsX()
    } else {
      contentSize = elem.getContentHeight()
      elemSize = elem.getHeight()
      scrollPos = elem.getScrollOffsY()
    }

    if (contentSize <= elemSize) {
      let cls = resolveBarClass(barClass, false)
      return class extends cls {
        key = scroll_handler
        behavior = Behaviors.Slider
        watch = scroll_handler
        size = (options?.needReservePlace ?? true) ? calcBarSize(cls, axis) : null
      }
    }


    let minV = 0
    let maxV = contentSize - elemSize
    let fValue = scrollPos

    let color = "colorCalc" in knobClass
      ? knobClass.colorCalc(stateFlags.value)
      : knobClass?.color

    let knob = class extends knobClass {
      size = [flex(elemSize), flex(elemSize)]
      color = color
      key = "knob"

      children = "hoverChild" in knobClass ? knobClass.hoverChild(stateFlags.value) : null
    }

    let cls = resolveBarClass(barClass, true)
    return class extends cls {
      key = scroll_handler
      behavior = Behaviors.Slider

      watch = [scroll_handler, stateFlags]
      fValue = fValue

      knob = knob
      min = minV //warning disable : -ident-hides-std-function
      max = maxV //warning disable : -ident-hides-std-function
      unit = 1

      flow = axis == 0 ? FLOW_HORIZONTAL : FLOW_VERTICAL
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER

      pageScroll = (axis == 0 ? -1 : 1) * (maxV - minV) / 100.0 // TODO probably needed sync with container wheelStep option

      orientation = orientation
      size = calcBarSize(cls, axis)

      children = [
        {size=[flex(fValue), flex(fValue)]}
        knob
        {size=[flex(maxV-fValue), flex(maxV-fValue)]}
      ]

      onChange = @(val) axis == 0
        ? scroll_handler.scrollToX(val)
        : scroll_handler.scrollToY(val)

      onElemState = @(sf) stateFlags.update(sf)
    }
  }
}

let DEF_SIDE_SCROLL_OPTIONS = { //const
  styling = defStyling
  rootBase = null
  scrollAlign = ALIGN_RIGHT
  orientation = O_VERTICAL
  size = flex()
  maxWidth = null
  maxHeight = null
  needReservePlace = true //need reserve place for scrollbar when it not visible
  clipChildren  = true
  joystickScroll = true
}

let function makeSideScroll(content, options = DEF_SIDE_SCROLL_OPTIONS) {
  options = DEF_SIDE_SCROLL_OPTIONS.__merge(options)

  let styling = options.styling
  let scrollHandler = options?.scrollHandler ?? ScrollHandler()
  let rootBase = options.rootBase ?? styling.ContentRoot
  let scrollAlign = options.scrollAlign

  let function contentRoot() {
    local bhv = rootBase?.behavior ?? []
    if (type(bhv) != "array")
      bhv = [bhv]
    else
      bhv = clone bhv
    bhv.append(Behaviors.WheelScroll, Behaviors.ScrollEvent)

    return class extends rootBase {
      size = options.size
      behavior = bhv
      scrollHandler = scrollHandler
      wheelStep = 0.8
      orientation = options.orientation
      joystickScroll = options.joystickScroll
      maxHeight = options.maxHeight
      maxWidth = options.maxWidth
      children = content
    }
  }

  let childrenContent = scrollAlign == ALIGN_LEFT || scrollAlign == ALIGN_TOP
    ? [scrollbar(scrollHandler, options), contentRoot]
    : [contentRoot, scrollbar(scrollHandler, options)]

  return {
    size = options.size
    maxHeight = options.maxHeight
    maxWidth = options.maxWidth
    flow = (options.orientation == O_VERTICAL) ? FLOW_HORIZONTAL : FLOW_VERTICAL
    clipChildren = options.clipChildren

    children = childrenContent
  }
}


let function makeHVScrolls(content, options={}) {
  let styling = options?.styling ?? defStyling
  let scrollHandler = options?.scrollHandler ?? ScrollHandler()
  let rootBase = options?.rootBase ?? styling.ContentRoot

  let function contentRoot() {
    local bhv = rootBase?.behavior ?? []
    if (type(bhv) != "array")
      bhv = [bhv]
    else
      bhv = clone bhv
    bhv.append(Behaviors.WheelScroll, Behaviors.ScrollEvent)

    return class extends rootBase {
      behavior = bhv
      scrollHandler = scrollHandler
      joystickScroll = true

      children = content
    }
  }

  return {
    size = flex()
    flow = FLOW_VERTICAL

    children = [
      {
        size = flex()
        flow = FLOW_HORIZONTAL
        clipChildren = true
        children = [
          contentRoot
          scrollbar(scrollHandler, options.__merge({orientation=O_VERTICAL}))
        ]
      }
      scrollbar(scrollHandler, options.__merge({orientation=O_HORIZONTAL}))
    ]
  }
}


let function makeVertScroll(content, options={}) {
  let o = clone options
  o.orientation <- O_VERTICAL
  o.scrollAlign <- o?.scrollAlign ?? ALIGN_RIGHT
  return makeSideScroll(content, o)
}


let function makeHorizScroll(content, options={}) {
  let o = clone options
  o.orientation <- O_HORIZONTAL
  o.scrollAlign <- o?.scrollAlign ?? ALIGN_BOTTOM
  return makeSideScroll(content, o)
}


return {
  styling = defStyling
  scrollbar = scrollbar
  makeHorizScroll
  makeVertScroll
  makeHVScrolls
  makeSideScroll
}
