from "%scripts/ui/ui_library.nut" import gui_scene, mkWatched, Computed, Watched, log

let console_register_command = require("console").register_command
let {set_actions_binding_column_active} = require("dainput2")
let { eventbus_send, eventbus_subscribe } = require("eventbus")
let platform = require("%dngscripts/platform.nut")
let controlsTypes = require("input_types.nut")
let forcedControlsType = mkWatched(persist, "forcedControlsType")
let defRaw = platform.is_pc ? 0 : 1
let lastActiveControlsTypeRaw = mkWatched(persist, "lastActiveControlsTypeRaw", defRaw)
let def = controlsTypes.keyboardAndMouse
let lastActiveControlsType = mkWatched(persist, "lastActiveControlType", def)

const EV_FORCE_CONTROLS_TYPE = "forced_controls_type"
function setForcedControlsType(v){
  forcedControlsType(v)
}

enum ControlsTypes {
  AUTO = 0
  KB_MOUSE = 1
  GAMEPAD = 2
}

console_register_command(@(value) eventbus_send(EV_FORCE_CONTROLS_TYPE, {val=value}), "ui.debugControlsType")
eventbus_subscribe(EV_FORCE_CONTROLS_TYPE, @(msg) setForcedControlsType(msg.val))

const EV_INPUT_USED = "input_dev_used"

function update_input_types(new_val){
  let map = {
    [1] = controlsTypes.keyboardAndMouse,
    [2] = controlsTypes.x1gamepad,
    //[3] = controlsTypes.ds4gamepad, //< no such value sent
  }
  local ctype = map?[new_val] ?? def
  lastActiveControlsTypeRaw.update(new_val ?? defRaw)
  lastActiveControlsType.update(ctype)
}

forcedControlsType.subscribe(function(val) {
  if (val)
    update_input_types(val)
})

eventbus_subscribe(EV_INPUT_USED, function(msg) {
  if ([null, 0].contains(forcedControlsType.value))
    update_input_types(msg.val)
})

let isGamepad = Computed(@() forcedControlsType.get() == ControlsTypes.GAMEPAD )
keepref(isGamepad)

const GAMEPAD_COLUMN = 1
let wasGamepad = mkWatched(persist, "wasGamepad", function() {
  let wasGamepadV = platform.is_pc ? false : true
  gui_scene.setConfigProps({gamepadCursorControl = wasGamepadV})
  return wasGamepadV
}())

let enabledGamepadControls = Watched(!platform.is_pc || isGamepad.value)

if (platform.is_pc){
  wasGamepad.subscribe(@(v) enabledGamepadControls(v))
  let setGamePadActive = @(v) set_actions_binding_column_active(GAMEPAD_COLUMN, v && forcedControlsType.value != ControlsTypes.KB_MOUSE)
  enabledGamepadControls.subscribe(setGamePadActive)
  forcedControlsType.subscribe(@(_v) setGamePadActive(enabledGamepadControls.value))
  setGamePadActive(isGamepad.value)
}

isGamepad.subscribe(function(v) {
  wasGamepad(wasGamepad.value || v)
  log($"isGamepad changed to = {v}")
  gui_scene.setConfigProps({gamepadCursorControl = v})
})


return {
  controlsTypes
  lastActiveControlsType
  lastActiveControlsTypeRaw
  isGamepad
  wasGamepad
  enabledGamepadControls
  forcedControlsType
  ControlsTypes
  setForcedControlsType
}
