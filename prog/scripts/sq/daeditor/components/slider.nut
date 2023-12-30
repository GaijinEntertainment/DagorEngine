from "%darg/ui_imports.nut" import *
let {colors} = require("style.nut")

let math = require("math")

let calcFrameColor = @(sf) sf & S_KB_FOCUS ? colors.TextActive
  : sf & S_HOVER ? colors.TextHover
  : colors.comboboxBorderColor

let opaque = Color(0,0,0,255)
let calcKnobColor =  @(sf) sf & S_KB_FOCUS ? (colors.TextActive | opaque)
  : sf & S_HOVER ? (colors.TextHover | opaque)
  : (colors.TextDefault | opaque)

let scales = {}
scales.linear <- {
  to = @(value, minv, maxv) (value.tofloat() - minv) / (maxv - minv)
  from = @(factor, minv, maxv) factor.tofloat() * (maxv - minv) + minv
}
scales.logarithmic <- {
  to = @(value, minv, maxv) value == 0 ? 0 : math.log(value.tofloat() / minv) / math.log(maxv / minv)
  from = @(factor, minv, maxv) minv * math.pow(maxv / minv, factor)
}
scales.logarithmicWithZero <- {
  to = @(value, minv, maxv) scales.logarithmic.to(value.tofloat(), minv, maxv)
  from = @(factor, minv, maxv) factor == 0 ? 0 : scales.logarithmic.from(factor, minv, maxv)
}
let sliderLeftLoc = "Reduce value"
let sliderRightLoc = "Increase value"

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
    }
  }

  let setValue = options?.setValue ?? @(v) var(v)
  function onChange(factor){
    let value = scaling.from(factor, minval, maxval)
    setValue(value)
  }

  let hotkeysElem = {
    key = "hotkeys"
    hotkeys = [
      ["Left | J:D.Left", sliderLeftLoc, function() {
        let delta = maxval > minval ? -pageScroll : pageScroll
        onChange(math.clamp(scaling.to(var.value + delta, minval, maxval), 0, 1))
      }],
      ["Right | J:D.Right", sliderRightLoc, function() {
        let delta = maxval > minval ? pageScroll : -pageScroll
        onChange(math.clamp(scaling.to(var.value + delta, minval, maxval), 0, 1))
      }],
    ]
  }

  return function() {
    let factor = math.clamp(scaling.to(var.value, minval, maxval), 0, 1)
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
      flow = FLOW_HORIZONTAL

      xmbNode = options?.xmbNode

      children = [
        {
          group = group
          rendObj = ROBJ_SOLID
          color = (sliderStateFlags.value & S_HOVER) ? colors.TextHighlight : colors.TextDefault
          size = [flex(factor), fsh(1)]

          children = {
            rendObj = ROBJ_FRAME
            color = calcFrameColor(sliderStateFlags.value)
            borderWidth = [hdpx(1),0,hdpx(1),hdpx(1)]
            size = flex()
          }
        }
        knob
        {
          group = group
          rendObj = ROBJ_SOLID
          color = colors.ControlBgOpaque
          size = [flex(1.0 - factor), fsh(1)]

          children = {
            rendObj = ROBJ_FRAME
            color = calcFrameColor(sliderStateFlags.value)
            borderWidth = [1,1,1,0]
            size = flex()
          }
        }
        sliderStateFlags.value & S_HOVER ? hotkeysElem  : null
      ]
    }
  }
}


return {
  slider
  scales
}
