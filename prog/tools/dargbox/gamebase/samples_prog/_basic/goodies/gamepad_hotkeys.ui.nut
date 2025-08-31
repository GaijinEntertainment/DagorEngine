from "%darg/ui_imports.nut" import *

const JOY_XBOX_REAL_AXIS_L_THUMB_H = 0
const JOY_XBOX_REAL_AXIS_L_THUMB_V = 1
const JOY_XBOX_REAL_AXIS_R_THUMB_H = 2
const JOY_XBOX_REAL_AXIS_R_THUMB_V = 3
const JOY_XBOX_REAL_AXIS_L_TRIGGER = 4
const JOY_XBOX_REAL_AXIS_R_TRIGGER = 5
const JOY_XBOX_REAL_AXIS_LR_TRIGGER = 6
const JOY_AXIS_MASK = 0x0F //MAX_JOY_AXIS_OBSERVABLES = 15
const AXIS_MODIFIEER_LT = 0x100
const AXIS_MODIFIEER_RT = 0x200


let btnSize = static [hdpx(200), hdpx(40)]
let gap = hdpx(10)
let borderWidth = hdpx(2)
let pressedOffset = static [0, hdpx(2)]
let vSliderSize = static [btnSize[1], btnSize[1] * 3 + gap * 2]
let hSliderSize = static [vSliderSize[1], vSliderSize[0]]
let markWidth = hdpx(5)

let isAxisShortcuts = Watched(false)
let stateFlagsLT = Watched(0)
let stateFlagsRT = Watched(0)
let isModActiveLT = Computed(@() (stateFlagsLT.get() & S_ACTIVE) != 0)
let isModActiveRT = Computed(@() (stateFlagsRT.get() & S_ACTIVE) != 0)
let axisModifiers = Computed(@() {
  [AXIS_MODIFIEER_LT] = isModActiveLT.get(),
  [AXIS_MODIFIEER_RT] = isModActiveRT.get(),
})
let axisModifierName = {
  [AXIS_MODIFIEER_LT] = "LT",
  [AXIS_MODIFIEER_RT] = "RT",
}
let axisModListeners = Watched({})

function button(hotkey, stateFlagsExt = null) {
  let stateFlags = stateFlagsExt ?? Watched(0)
  return @() {
    watch = stateFlags
    size = btnSize
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER

    rendObj = ROBJ_BOX
    fillColor = (stateFlags.get() & S_ACTIVE) ? 0xFF000000 : 0xFFC8C8C8
    borderWidth = (stateFlags.get() & S_HOVER) ? borderWidth : 0

    behavior = Behaviors.Button
    onElemState = @(v) stateFlags.set(v)
    onClick = @() dlog("onClick ", hotkey)
    hotkeys = [ hotkey ]

    children = {
      pos = stateFlags.get() & S_ACTIVE ? pressedOffset : null
      rendObj = ROBJ_TEXT
      text = hotkey
      color = (stateFlags.get() & S_ACTIVE) ? 0xFFFFFFFF : 0xFF000000
    }
  }
}

function axisListener(axisWithModifier, action) {
  let axis = axisWithModifier & JOY_AXIS_MASK
  let modifier = axisWithModifier & ~JOY_AXIS_MASK
  let axisWatch = gui_scene.getJoystickAxis(axis)
  let isAllowed = modifier != 0 ? Computed(@() axisModifiers.get()?[modifier] ?? true)
    : Computed(@() (axisModListeners.get()?[axis].findindex(@(_, m) axisModifiers.get()?[m] ?? false)) == null)
  let watch = Computed(@() isAllowed.get() ? axisWatch.get() : 0)
  return watch == null ? {}
    : {
        key = watch
        size = 0 //need only to avoid "invalid component description" error
        function onAttach() {
          watch.subscribe_with_nasty_disregard_of_frp_update(action)
          action(watch.get())
          if (modifier != 0)
            axisModListeners.mutate(@(v) v.$rawset(axis, ((clone v?[axis]) ?? {}).$rawset(modifier, true))) //--unwanted-modification
        }
        function onDetach() {
          watch.unsubscribe(action)
          action(0)
          if (modifier in axisModListeners.get()?[axis])
            axisModListeners.mutate(function(v) {
              let mods = clone v[axis]
              mods.$rawdelete(modifier)
              v[axis] = mods
            })
        }
      }
}

function axisProgress(axis, text1, isVertical) {
  let value = Watched(0)
  return axisListener(axis, @(v) value.set(v))
    .__update({
      size = isVertical ? vSliderSize : hSliderSize
      padding = borderWidth
      rendObj = ROBJ_BOX
      fillColor = 0xFF000000
      borderWidth = borderWidth
      borderColor = 0xFFC8C8C8
      children = [
        @() {
          watch = value
          size = isVertical ? [flex(), markWidth] : [markWidth, flex()]
          pos = isVertical
            ? [0, 0.5 * (value.get() + 1.0) * (vSliderSize[1] - 2 * borderWidth - markWidth)]
            : [0.5 * (value.get() + 1.0) * (hSliderSize[0] - 2 * borderWidth - markWidth), 0]
          rendObj = ROBJ_SOLID
          color = 0xFFFFFF00
        }
        {
          pos = isVertical ? [0, -0.25 * vSliderSize[1]] : [-0.25 * hSliderSize[0], 0]
          vplace = ALIGN_CENTER
          hplace = ALIGN_CENTER
          rendObj = ROBJ_TEXT
          text = text1
          color = 0xFFC8C8C8
          transform = !isVertical ? {} : { rotate = 90 }
        }
        {
          pos = isVertical ? [0, 0.25 * vSliderSize[1]] : [0.25 * hSliderSize[0], 0]
          vplace = ALIGN_CENTER
          hplace = ALIGN_CENTER
          rendObj = ROBJ_TEXT
          text = axisModifierName?[axis & ~JOY_AXIS_MASK]
          color = 0xFFC8C8C8
          transform = !isVertical ? {} : { rotate = 90 }
        }
      ]
    })
}

let columnsRows = @(arr2) {
  flow = FLOW_HORIZONTAL
  gap
  children = arr2.map(@(children) {
    flow = FLOW_VERTICAL
    gap
    children
  })
}

function axisToggleButton() {
  let stateFlags = Watched(0)
  return @() {
    watch = [stateFlags, isAxisShortcuts]
    size = btnSize
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER

    rendObj = ROBJ_BOX
    fillColor = (stateFlags.get() & S_ACTIVE) ? 0xFF000000 : 0xFFC8C8C8
    borderWidth = (stateFlags.get() & S_HOVER) ? borderWidth : 0

    behavior = Behaviors.Button
    onElemState = @(v) stateFlags.set(v)
    onClick = @() isAxisShortcuts.set(!isAxisShortcuts.get())

    children = {
      pos = stateFlags.get() & S_ACTIVE ? pressedOffset : null
      rendObj = ROBJ_TEXT
      text = isAxisShortcuts.get() ? "axis mods" : "hotkeys mods"
      color = (stateFlags.get() & S_ACTIVE) ? 0xFFFFFFFF : 0xFF000000
    }
  }
}

return @() {
  watch = isAxisShortcuts
  children = columnsRows([
    [
      axisToggleButton()
    ],
    [
      button("^J:A")
      button("^J:B")
      button("^J:X")
      button("^J:Y")
      button("^J:D.Up")
      button("^J:D.Down")
      button("^J:D.Left")
      button("^J:D.Right")
      button("^J:Start")
      button("^J:Back")
      button("^J:LB")
      button("^J:RB")
      button("^J:LT", stateFlagsLT)
      button("^J:RT", stateFlagsRT)
      button("^J:LS")
      button("^J:RS")
    ],
    [
      button("^J:LS.Right")
      button("^J:LS.Left")
      button("^J:LS.Up")
      button("^J:LS.Down")
      button("^J:RS.Right")
      button("^J:RS.Left")
      button("^J:RS.Up")
      button("^J:RS.Down")
    ],
    [
      button("^J:LT J:A")
      button("^J:LT J:B")
      button("^J:LT J:X")
      button("^J:LT J:Y")
      button("^J:LT J:D.Up")
      button("^J:LT J:D.Down")
      button("^J:LT J:D.Left")
      button("^J:LT J:D.Right")
      button("^J:LT J:Start")
      button("^J:LT J:Back")
      button("^J:LT J:LB")
      button("^J:LT J:RB")
      button("^J:LT J:RT")
      button("^J:LT J:LS")
      button("^J:LT J:RS")
    ],
    !isAxisShortcuts.get() ? null
      : [
          button("^J:LT J:LS.Right")
          button("^J:LT J:LS.Left")
          button("^J:LT J:LS.Up")
          button("^J:LT J:LS.Down")
          button("^J:LT J:RS.Right")
          button("^J:LT J:RS.Left")
          button("^J:LT J:RS.Up")
          button("^J:LT J:RS.Down")
        ],
    [
      axisProgress(JOY_XBOX_REAL_AXIS_R_THUMB_H, "RS_H", false)
      axisProgress(JOY_XBOX_REAL_AXIS_R_THUMB_V, "RS_V", true)
      axisProgress(JOY_XBOX_REAL_AXIS_L_THUMB_H, "LS_H", false)
      axisProgress(JOY_XBOX_REAL_AXIS_L_THUMB_V, "LS_V", true)

      axisProgress(JOY_XBOX_REAL_AXIS_L_TRIGGER, "LT", false)
      axisProgress(JOY_XBOX_REAL_AXIS_R_TRIGGER, "RT", false)
      axisProgress(JOY_XBOX_REAL_AXIS_LR_TRIGGER, "LRT", false)
    ],
    isAxisShortcuts.get() ? null
      : [
          axisProgress(JOY_XBOX_REAL_AXIS_R_THUMB_H | AXIS_MODIFIEER_LT, "RS_H", false)
          axisProgress(JOY_XBOX_REAL_AXIS_R_THUMB_V | AXIS_MODIFIEER_LT, "RS_V", true)
          axisProgress(JOY_XBOX_REAL_AXIS_L_THUMB_H | AXIS_MODIFIEER_LT, "LS_H", false)
          axisProgress(JOY_XBOX_REAL_AXIS_L_THUMB_V | AXIS_MODIFIEER_LT, "LS_V", true)
        ],
    isAxisShortcuts.get() ? null
      : [
          axisProgress(JOY_XBOX_REAL_AXIS_R_THUMB_H | AXIS_MODIFIEER_RT, "RS_H", false)
          axisProgress(JOY_XBOX_REAL_AXIS_R_THUMB_V | AXIS_MODIFIEER_RT, "RS_V", true)
          axisProgress(JOY_XBOX_REAL_AXIS_L_THUMB_H | AXIS_MODIFIEER_RT, "LS_H", false)
          axisProgress(JOY_XBOX_REAL_AXIS_L_THUMB_V | AXIS_MODIFIEER_RT, "LS_V", true)
        ],
  ].filter(@(v) v != null))
}
