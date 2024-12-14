from "%scripts/ui/ui_library.nut" import *
from "style.nut" import *
import "math" as math
//let {sound_play} = require("%dngscripts/sound_system.nut")

let calcFrameColor = @(sf) (sf & S_KB_FOCUS)
    ? BtnBgFocused
    : (sf & S_HOVER) ? BtnBdSelected : CheckBoxContentDefault

let opaque = Color(0,0,0,255)
let calcKnobColor =  @(sf) (sf & S_KB_FOCUS) ? (TextActive | opaque)
                           : (sf & S_HOVER)    ? (TextHover | opaque)
                                               : (TextNormal | opaque)

let scales = {
  linear = {
    to = @(value, minv, maxv) (value.tofloat() - minv) / (maxv - minv)
    from = @(factor, minv, maxv) factor.tofloat() * (maxv - minv) + minv
  }
  logarithmic = {
    to = @(value, minv, maxv) value == 0 ? 0 : math.log(value.tofloat() / minv) / math.log(maxv / minv)
    from = @(factor, minv, maxv) minv * math.pow(maxv / minv, factor)
  }
}
scales.logarithmicWithZero <- {
  to = @(value, minv, maxv) scales.logarithmic.to(value.tofloat(), minv, maxv)
  from = @(factor, minv, maxv) factor == 0 ? 0 : scales.logarithmic.from(factor, minv, maxv)
}

let sliderLeftLoc = loc("slider/reduce", "Reduce value")
let sliderRightLoc = loc("slider/increase", "Increase value")

function slider(orient, var, options={}) {
  let minval = options?.min ?? 0
  let maxval = options?.max ?? 1
  let group = options?.group ?? ElemGroup()
  let rangeval = maxval-minval
  let scaling = options?.scaling ?? scales.linear
  let step = options?.step
  let unit = options?.unit && options?.scaling!=scales.linear
    ? options?.unit
    : step ? step/rangeval : 0.01
  let pageScroll = options?.pageScroll ?? step ?? 0.05
  let ignoreWheel = options?.ignoreWheel ?? true
  let knobContent = options?.knobContent ?? {}
  let bgColor = options?.bgColor ?? ControlBgOpaque

  let knobStateFlags = Watched(0)
  let sliderStateFlags = Watched(0)

  function knob() {
    return {
      rendObj = ROBJ_SOLID
      size  = [fsh(1), fsh(2)]
      group = group
      color = calcKnobColor(knobStateFlags.value)
      watch = knobStateFlags
      onElemState = @(sf) knobStateFlags.update(sf)
      children = knobContent
    }
  }

  let setValue = options?.setValue ?? @(v) var.set(v)
  function onChange(factor){
    let value = orient == O_HORIZONTAL ? scaling.from(factor, minval, maxval) : scaling.from(factor, maxval, minval)
//    let oldValue = var.value
    setValue(value)
//    if (oldValue != var.value)
//      sound_play("ui/timer_tick")
  }

  let hotkeysElem = {
    key = "hotkeys"
    hotkeys = [
      ["Left | J:D.Left", sliderLeftLoc, function() {
        let delta = maxval > minval ? -pageScroll : pageScroll
        onChange(clamp(scaling.to(var.value + delta, minval, maxval), 0, 1))
      }],
      ["Right | J:D.Right", sliderRightLoc, function() {
        let delta = maxval > minval ? pageScroll : -pageScroll
        onChange(clamp(scaling.to(var.value + delta, minval, maxval), 0, 1))
      }],
    ]
  }

  return function() {
    let factor = clamp(scaling.to(var.get(), minval, maxval), 0, 1)
    return {
      size = flex()
      behavior = Behaviors.Slider
      watch = [var, sliderStateFlags]
      orientation = orient

      min = 0
      max = 1
      unit
      pageScroll
      ignoreWheel

      fValue = factor
      knob

      onChange = onChange
      onElemState = @(sf) sliderStateFlags.update(sf)

      valign = ALIGN_CENTER
      flow = orient == O_HORIZONTAL ? FLOW_HORIZONTAL : FLOW_VERTICAL

      xmbNode = options?.xmbNode

      children = [
        {
          group = group
          rendObj = ROBJ_SOLID
          color = orient == O_HORIZONTAL ? (sliderStateFlags.value & S_HOVER) ? BtnBdHover : TextNormal
            : bgColor
          size = orient == O_HORIZONTAL ? [flex(factor), fsh(1)] : [fsh(1), flex(1.0 - factor)]

          children = {
            rendObj = ROBJ_FRAME
            color = calcFrameColor(sliderStateFlags.value)
            borderWidth = orient == O_HORIZONTAL ? [hdpx(1),0,hdpx(1),hdpx(1)] : 0
            size = flex()
          }
        }
        knob
        {
          group = group
          rendObj = ROBJ_SOLID
          color = bgColor
          size =  orient == O_HORIZONTAL ? [flex(1.0 - factor), fsh(1)] : [fsh(1), flex(factor)]

          children = {
            rendObj = ROBJ_FRAME
            color = calcFrameColor(sliderStateFlags.value)
            borderWidth = orient == O_HORIZONTAL ? [1,1,1,0] : 0
            size = flex()
          }
        }
        sliderStateFlags.value & S_HOVER ? hotkeysElem  : null
      ]
    }
  }
}

let slider_value_width = calc_str_box("200%")[0]
let HorizSlider = @(var, options={}) slider(O_HORIZONTAL, var, options)
let VertSlider  = @(var, options={}) slider(O_VERTICAL, var, options)

function sliderWithText(opt, _group=null, xmbNode=null, ) {
  let valWatch = opt.var
  let morphText = opt?.morphText ?? @(val) val
  return {
    size = flex()
    flow = FLOW_HORIZONTAL
    valign = ALIGN_CENTER
    gap = fsh(1)

    children = [
      HorizSlider(valWatch, opt.__merge({xmbNode = xmbNode ?? XmbNode()}))
      @() {
        watch = valWatch
        size = [slider_value_width, SIZE_TO_CONTENT]
        rendObj = ROBJ_TEXT
        color = TextNormal
        text = morphText(valWatch.get())
      }
    ]
  }
}

return {
  scales
  sliderWithText
  HorizSlider
  VertSlider
}
