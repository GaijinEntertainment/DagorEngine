from "%darg/ui_imports.nut" import *

let barInvisible = freeze({
  color = Color(40, 40, 40, 160)
  _width = sh(1)
  _height = sh(1)
})

let barVisible = freeze(barInvisible.__merge({rendObj = ROBJ_SOLID}))

function calcBarSize(bar_class, axis) {
  return axis == 0 ? [flex(), bar_class._height] : [bar_class._width, flex()]
}


let knobColorCalc = @(sf) (sf & S_ACTIVE) ? Color(255,255,255)
                  : (sf & S_HOVER)  ? Color(110, 120, 140, 80)
                                    : Color(110, 120, 140, 160)
function scrollbar(scroll_handler, options={}) {
  let stateFlags = Watched(0)

  let orientation = options?.orientation ?? O_VERTICAL
  let axis        = orientation == O_VERTICAL ? 1 : 0

  return function() {
    let elem = scroll_handler.elem

    if (!elem) {
      return barInvisible.__merge({
        key = scroll_handler
        behavior = Behaviors.Slider
        watch = scroll_handler
        size = (options?.needReservePlace ?? true) ? calcBarSize(barInvisible, axis) : null
      })
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
      return barInvisible.__merge({
        key = scroll_handler
        behavior = Behaviors.Slider
        watch = scroll_handler
        size = (options?.needReservePlace ?? true) ? calcBarSize(barInvisible, axis) : null
      })
    }


    let minV = 0
    let maxV = contentSize - elemSize
    let fValue = scrollPos

    let knob = {
      rendObj = ROBJ_SOLID
      size = [flex(elemSize), flex(elemSize)]
      color = knobColorCalc(stateFlags.get())
      key = "knob"
    }

    return barVisible.__merge({
      key = scroll_handler
      behavior = Behaviors.Slider
      watch = [scroll_handler, stateFlags]
      fValue

      knob
      min = minV //warning disable : -ident-hides-std-function
      max = maxV //warning disable : -ident-hides-std-function
      unit = 1

      flow = axis == 0 ? FLOW_HORIZONTAL : FLOW_VERTICAL
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER

      pageScroll = (axis == 0 ? -1 : 1) * (maxV - minV) / 100.0 // TODO probably needed sync with container wheelStep option

      orientation = orientation
      size = calcBarSize(barVisible, axis)

      children = [
        {size=[flex(fValue), flex(fValue)]}
        knob
        {size=[flex(maxV-fValue), flex(maxV-fValue)]}
      ]

      onChange = @(val) axis == 0
        ? scroll_handler.scrollToX(val)
        : scroll_handler.scrollToY(val)

      onElemState = @(sf) stateFlags.update(sf)
    })
  }
}

let DEF_SIDE_SCROLL_OPTIONS = { //const
  scrollAlign = ALIGN_RIGHT
  orientation = O_VERTICAL
  size = flex()
  maxWidth = null
  maxHeight = null
  needReservePlace = true //need reserve place for scrollbar when it not visible
  clipChildren  = true
  joystickScroll = true
}

function makeSideScroll(content, options = DEF_SIDE_SCROLL_OPTIONS) {
  options = DEF_SIDE_SCROLL_OPTIONS.__merge(options)

  let scrollHandler = options?.scrollHandler ?? ScrollHandler()
  let scrollAlign = options.scrollAlign

  function contentRoot() {
    return {
      size = flex()
      behavior = [Behaviors.WheelScroll, Behaviors.ScrollEvent]
      scrollHandler = scrollHandler
      wheelStep = 0.8
      orientation = options.orientation
      joystickScroll = options.joystickScroll
      maxHeight = options.maxHeight
      maxWidth = options.maxWidth
      children = content
    }
  }
  let sb = scrollbar(scrollHandler, options)

  let childrenContent = scrollAlign == ALIGN_LEFT || scrollAlign == ALIGN_TOP
    ? [sb, contentRoot]
    : [contentRoot, sb]

  return {
    size = options.size
    maxHeight = options.maxHeight
    maxWidth = options.maxWidth
    flow = (options.orientation == O_VERTICAL) ? FLOW_HORIZONTAL : FLOW_VERTICAL
    //gap = hdpx(10)
    clipChildren = options.clipChildren

    children = childrenContent
  }
}


function makeHVScrolls(content, options={}) {
  let scrollHandler = options?.scrollHandler ?? ScrollHandler()

  function contentRoot() {

    return {
      behavior = [Behaviors.WheelScroll, Behaviors.ScrollEvent]
      scrollHandler = scrollHandler
      joystickScroll = true
      size = flex()
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


function makeVertScroll(content, options={}) {
  let o = clone options
  o.orientation <- O_VERTICAL
  o.scrollAlign <- o?.scrollAlign ?? ALIGN_RIGHT
  return makeSideScroll(content, o)
}


function makeHorizScroll(content, options={}) {
  let o = clone options
  o.orientation <- O_HORIZONTAL
  o.scrollAlign <- o?.scrollAlign ?? ALIGN_BOTTOM
  return makeSideScroll(content, o)
}


return {
  scrollbar
  makeHorizScroll
  makeVertScroll
  makeHVScrolls
  makeSideScroll
}
