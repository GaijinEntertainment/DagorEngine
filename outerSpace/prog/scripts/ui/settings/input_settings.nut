from "%scripts/ui/ui_library.nut" import *
import "dainput2" as dainput
from "%scripts/ui/widgets/style.nut" import *

let { eventbus_send_foreign } = require("eventbus")
let {CONTROLS_SETUP_CHANGED_EVENT_ID} = require("input_generation.nut")
let {round_by_value} = require("%sqstd/math.nut")
let {showMsgbox} = require("%scripts/ui/widgets/msgbox.nut")
let textButton = require("%scripts/ui/widgets/simpleComponents.nut").menuBtn
let console = require("console")
let { buildElems, buildDigitalBindingText, isValidDevice, eventTypeLabels,
      textListFromAction, getSticksText } = require("formatInputBinding.nut")
let {platformId} = require("%dngscripts/platform.nut")
let {format_ctrl_name} = dainput
//let {get_action_handle} = dainput
let { BTN_pressed, BTN_pressed_long, BTN_pressed2, BTN_pressed3,
          BTN_released, BTN_released_long, BTN_released_short } = dainput
let control = require("control")
let DataBlock = require("DataBlock")
let { isGamepad } = require("active_input.nut")

let {
  importantGroups, generation, nextGeneration, availablePresets, haveChanges, Preset,
  getActionsList, getActionTags, mkSubTagsFind
} = require("input_state.nut")
//let { makeVertScroll } = require("%scripts/ui/widgets/components/scrollbar.nut")
let { mkCombo } = require("%scripts/ui/widgets/simpleComponents.nut")
let checkbox = @(params) mkCombo(params.__update({values=[true, false]}))
let {sliderWithText} = require("%scripts/ui/widgets/slider.nut")

let makeVertScroll = @(children) {size = flex() children }
let h2_txt = {fontSize = hdpx(40)}
let body_txt = {fontSize = hdpx(20)}
let dtext = @(text, params = null) {text, rendObj = ROBJ_TEXT}.__update(params ?? {})

let MenuRowBgHover = mul_color(BtnBgHover, 0.3)

function menuRowColor(sf, isOdd) {
  return (sf & S_HOVER)
         ? MenuRowBgHover
         : isOdd ? MenuRowBgOdd : MenuRowBgEven
}

console.register_command(function(){
  let origBlk = DataBlock()
  origBlk.setStr("preset", "content/outer_space/config/outer_space.default")
  dainput.load_user_config(origBlk)
  control.save_config()
  haveChanges(true)
}, "input.init_pc_preset")

let findSelectedPreset = function(selected) {
  let defaultPreset = dainput.get_user_config_base_preset()
  return availablePresets.value.findvalue(@(v) selected.indexof(v.preset)!=null) ??
         availablePresets.value.findvalue(@(v) ($"{defaultPreset}~{platformId}").indexof(v.preset)!=null) ??
         Preset(defaultPreset)
}
let selectedPreset = Watched(findSelectedPreset(dainput.get_user_config_base_preset()))
let currentPreset =  Watched(selectedPreset.value)
currentPreset.subscribe(@(v) selectedPreset(findSelectedPreset(v.preset)))
let updateCurrentPreset = @() currentPreset(findSelectedPreset(dainput.get_user_config_base_preset()))

let actionRecording = Watched(null)
let configuredAxis = Watched(null)
let configuredButton = Watched(null)

let showControlsMenu = mkWatched(persist, "showControlsMenu", false)
//gui_scene.xmbMode.subscribe(@(v) vlog($"XMB mode = {v}"))

function isGamepadColumn(col) {
  return col == 1
}

function locActionName(name){
  return name!= null ? loc("/".concat("controls", name), name) : null
}

function doesDeviceMatchColumn(dev_id, col) {
  if (dev_id==dainput.DEV_kbd || dev_id==dainput.DEV_pointing)
    return !isGamepadColumn(col)
  if (dev_id==dainput.DEV_gamepad || dev_id==dainput.DEV_joy)
    return isGamepadColumn(col)
  return false
}

let btnEventTypesMap = {
  [BTN_pressed] = "pressed",
  [BTN_pressed_long] = "pressed_long",
  [BTN_pressed2] = "pressed2",
  [BTN_pressed3] = "pressed3",
  [BTN_released] = "released",
  [BTN_released_long] = "released_long",
  [BTN_released_short] = "released_short",
}

let findAllowBindingsSubtags = mkSubTagsFind("allowed_bindings=")
let getAllowedBindingsTypes = memoize(function(ah) {
  let allowedbindings = findAllowBindingsSubtags(ah)
  if (allowedbindings==null)
    return btnEventTypesMap.keys()
  return btnEventTypesMap
    .filter(@(name, _eventType) allowedbindings==null || allowedbindings.indexof(name)!=null)
    .keys()
})


let isActionDisabledToCustomize = memoize(
  @(action_handler) getActionTags(action_handler).indexof("disabled") != null)

let selectedBindingCell = mkWatched(persist, "selectedBindingCell")
let selectedAxisCell = mkWatched(persist, "selectedAxisCell")

function isEscape(blk) {
  return blk.getInt("dev", dainput.DEV_none) == dainput.DEV_kbd
      && blk.getInt("btn", 0) == 1
}

let blkPropRemap = {
  minXBtn = "xMinBtn", maxXBtn = "xMaxBtn", minYBtn = "yMinBtn", maxYBtn = "yMaxBtn"
}

let pageAnim =  [
  { prop=AnimProp.opacity, from=0, to=1, duration=0.2, play=true, easing=InOutCubic}
  { prop=AnimProp.opacity, from=1, to=0, duration=0.2, playFadeOut=true, easing=InOutCubic}
]

function startRecording(cell_data) {
  if (cell_data.singleBtn || cell_data.tag == "modifiers")
    dainput.start_recording_bindings_for_single_button()
  else
    dainput.start_recording_bindings(cell_data.ah)
  actionRecording(cell_data)
}

function makeBgToggle(initial=true) {
  local showBg = !initial
  function toggleBg() {
    showBg = !showBg
    return showBg
  }
  return toggleBg
}

function set_single_button_analogue_binding(ah, col, actionProp, blk) {
  let stickBinding = dainput.get_analog_stick_action_binding(ah, col)
  let axisBinding = dainput.get_analog_axis_action_binding(ah, col)
  let binding = stickBinding ?? axisBinding
  if (binding != null) {
    binding[actionProp].devId = blk.getInt("dev", 0)
    binding[actionProp].btnId = blk.getInt("btn", 0)
    if (blk.paramCount()+blk.blockCount() == 0) {
      // empty blk, so clear bindings
      binding.devId = dainput.DEV_none
      if (axisBinding != null)
        axisBinding.axisId = 0
      if (stickBinding != null) {
        stickBinding.axisXId = 0
        stickBinding.axisYId = 0
      }
    } else if (binding.devId == dainput.DEV_none && (axisBinding != null || stickBinding != null))
      binding.devId = dainput.DEV_nullstub

    if (binding.devId != dainput.DEV_none && binding.maxVal == 0) // restore maxVal when needed
      binding.maxVal = 1
  }
}

function loadOriginalBindingParametersTo(blk, ah, col) {
  let origBlk = DataBlock()
  dainput.get_action_binding(ah, col, origBlk)

  let actionType = dainput.get_action_type(ah)

  blk.setReal("dzone", origBlk.getReal("dzone", 0.0))
  blk.setReal("nonlin", origBlk.getReal("nonlin", 0.0))
  blk.setReal("maxVal", origBlk.getReal("maxVal", 1.0))
  if ((actionType & dainput.TYPEGRP__MASK) == dainput.TYPEGRP_STICK)
    blk.setReal("sensScale", origBlk.getReal("sensScale", 1.0))
}

function loadPreviousBindingParametersTo(blk, ah, col) {
  let prevBinding = dainput.get_digital_action_binding(ah, col)

  let actionType = dainput.get_action_type(ah)

  if ((actionType & dainput.TYPEGRP__MASK) == dainput.TYPEGRP_DIGITAL)
    blk.setBool("stickyToggle", prevBinding.stickyToggle)
}

function checkRecordingFinished() {
  if (dainput.is_recording_complete()) {
    let cellData = actionRecording.value
    actionRecording(null)
    let ah = cellData?.ah

    let blk = DataBlock()
    let ok = dainput.finish_recording_bindings(blk)

    let devId = blk.getInt("dev", dainput.DEV_none)
    if (ok && ah!=null && devId!=dainput.DEV_none && !isEscape(blk)) {
      let col = cellData?.column
      if (doesDeviceMatchColumn(devId, col)) {
        gui_scene.clearTimer(callee())

        local checkConflictsBlk
        if (cellData.singleBtn) {
          checkConflictsBlk = DataBlock()
          dainput.get_action_binding(ah, col, checkConflictsBlk)
          let btnBlk = checkConflictsBlk.addBlock(blkPropRemap?[cellData.actionProp] ?? cellData.actionProp)
          btnBlk.setParamsFrom(blk)
        }
        else {
          checkConflictsBlk = blk
        }

        function applyBinding() {
          if (cellData.singleBtn) {
            set_single_button_analogue_binding(ah, col, cellData.actionProp, blk)
          }
          else if (cellData.tag == "modifiers") {
            let binding = dainput.get_analog_stick_action_binding(ah, cellData.column)
                          ?? dainput.get_analog_axis_action_binding(ah, cellData.column)

            let btn = dainput.SingleButtonId()
            btn.devId = devId
            btn.btnId = blk.getInt("btn", 0)
            binding.mod = [btn]
          }
          else {
            loadOriginalBindingParametersTo(blk, ah, col)
            loadPreviousBindingParametersTo(blk, ah, col)
            dainput.set_action_binding(ah, col, blk)
            let binding = dainput.get_digital_action_binding(ah, col)
            if (binding?.eventType)
              binding.eventType = getAllowedBindingsTypes(ah)[0]
          }
          nextGeneration()
          haveChanges(true)
        }

        let conflicts = dainput.check_bindings_conflicts(ah, checkConflictsBlk)
        if (conflicts == null) {
          applyBinding()
        } else {
          let actionNames = conflicts.map(@(a) dainput.get_action_name(a.action))
          let localizedNames = actionNames.map(@(a) loc($"controls/{a}"))
          let actionsText = ", ".join(localizedNames)
          let messageText = loc("controls/binding_conflict_prompt", "This conflicts with {actionsText}. Bind anyway?", {
            actionsText = actionsText
          })
          showMsgbox({
            text = messageText
            buttons = [
              { text = loc("Yes"), action = applyBinding }
              { text = loc("No") }
            ]
          })
        }
      } else {
        startRecording(cellData)
      }
    }
  }
}


function cancelRecording() {
  gui_scene.clearTimer(checkRecordingFinished)
  actionRecording(null)

  let blk = DataBlock()
  dainput.finish_recording_bindings(blk)
}



let mediumText = @(text, params={}) dtext(text, {color = TextNormal,}.__update(body_txt, params))

function recordingWindow() {
  //local text = loc("controls/recording", "Press a button (or move mouse / joystick axis) to bind action to")
  local text
  let cellData = actionRecording.value
  let name = cellData?.name
  if (cellData) {
    let actionType = dainput.get_action_type(cellData.ah)
    if ( (actionType & dainput.TYPEGRP__MASK) == dainput.TYPEGRP_DIGITAL
          || cellData.actionProp!=null || cellData.tag == "modifiers") {
      if (isGamepadColumn(cellData.column))
        text = loc("controls/recording_digital_gamepad", "Press a gamepad button to bind action to")
      else
        text = loc("controls/recording_digital_keyboard", "Press a button on keyboard to bind action to")
    }
    else if (isGamepadColumn(cellData.column)) {
      text = loc("controls/recording_analogue_joystick", "Move stick or press button to bind action to")
    } else {
      text = loc("controls/recording_analogue_mouse", "Move mouse to bind action")
    }
  }
  return {
    size = flex()
    rendObj = ROBJ_WORLD_BLUR_PANEL
    fillColor = Color(0,0,0,225)
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    stopHotkeys = true
    stopMouse = true
    cursor = null
    hotkeys = [["^Esc", cancelRecording]]
    onDetach = cancelRecording
    watch = actionRecording
    flow = FLOW_VERTICAL
    gap = fsh(8)
    children = [
      {size=[0, flex(3)]}
      dtext(locActionName(name), {color = Color(100,100,100)}.__update(h2_txt))
      mediumText(text, {
        function onAttach() {
          gui_scene.clearTimer(checkRecordingFinished)
          gui_scene.setInterval(0.1, checkRecordingFinished)
        }
        function onDetach() {
          gui_scene.clearTimer(checkRecordingFinished)
        }
      })
      {size=[0, flex(5)]}
    ]
  }
}

function saveChanges() {
  control.save_config()
  haveChanges(false)
}

function applyPreset(text, target=null) {
  function doReset() {
    if (target)
      dainput.reset_user_config_to_preset(target.preset, false)
    else
      control.reset_to_default()
    saveChanges()
    nextGeneration()
  }

  showMsgbox({
    text
    children = dtext(loc("controls/preset", {
      preset = target?.name ?? selectedPreset.value.name
    }), {margin = hdpx(50)})
    buttons = [
      { text = loc("Yes"), action = doReset }
      { text = loc("No") }
    ]
  })
}

function resetToDefault() {
  let text = loc("controls/resetToDefaultsConfirmation", "Confirm reset")
  applyPreset(text)
}

/*
function changePreset(target) {
  let text = loc("controls/changeControlsPresetConfirmation")
  applyPreset(text, target)
}
*/
function clearBinding(cellData){
  haveChanges(true)
  if (cellData.singleBtn) {
    set_single_button_analogue_binding(cellData.ah, cellData.column, cellData.actionProp, DataBlock())
  } else if (cellData.tag == "modifiers") {
    let binding = dainput.get_analog_stick_action_binding(cellData.ah, cellData.column)
                  ?? dainput.get_analog_axis_action_binding(cellData.ah, cellData.column)
    binding.mod = []
  } else {
    let blk = DataBlock()
    loadOriginalBindingParametersTo(blk, cellData.ah, cellData.column)
    dainput.set_action_binding(cellData.ah, cellData.column, blk)
  }
}

function discardChanges() {
  if (!haveChanges.value)
    return
  control.restore_saved_config()
  nextGeneration()
  haveChanges(false)
}

function actionButtons() {
  local children = null
  let cellData = selectedBindingCell.value

  if (cellData != null) {
    let actionType = dainput.get_action_type(cellData.ah)
    let actionTypeGroup = actionType & dainput.TYPEGRP__MASK

    if (!isActionDisabledToCustomize(cellData.ah)) {
      children = [
        textButton(loc("controls/clearBinding", "Clear"), function() {
          clearBinding(cellData)
          nextGeneration()
        }, { hotkeys = [["^J:X"]], skipDirPadNav = true })
      ]

      if (actionTypeGroup == dainput.TYPEGRP_AXIS || actionTypeGroup == dainput.TYPEGRP_STICK) {
        children.append(
          textButton(loc("controls/axisSetup", "Axis setup"),
            function() { configuredAxis(cellData) })
        )
      }
      else if (actionTypeGroup == dainput.TYPEGRP_DIGITAL) {
        children.append(
          textButton(loc("controls/buttonSetup", "Button setup"),
            function() { configuredButton(cellData) })

          textButton(loc("controls/bindBinding", "Bind"),
            function() { startRecording(cellData) })
        )
      }
    }
  }
  return {
    watch = selectedBindingCell
    children
    flow = FLOW_HORIZONTAL
  }
}

function collectBindableColumns() {
  let nColumns = dainput.get_actions_binding_columns()
  let colRange = []
  for (local i=0; i<nColumns; ++i) {
    colRange.append(i)
  }
  return colRange
}

function getNotBoundActions() {
  let importantTabs = importantGroups.value

  let colRange = collectBindableColumns()
  let notBoundActions = {}
  for (local i = 0; i < dainput.get_actions_count(); ++i) {
    let ah = dainput.get_action_handle_by_ord(i)
    if (dainput.is_action_internal(ah))
      continue

    let actionGroups = getActionTags(ah)
    local isActionInImportantGroup = false
    foreach (actionGroup in actionGroups) {
      if (importantTabs.indexof(actionGroup) != null && !isActionDisabledToCustomize(ah))
        isActionInImportantGroup = true
    }

    if (actionGroups.indexof("not_important") != null)
      isActionInImportantGroup = false

    if (actionGroups.indexof("important") != null)
      isActionInImportantGroup = true

    if (!isActionInImportantGroup)
      continue

    local someBound = false
    let atFlag = dainput.get_action_type(ah) & dainput.TYPEGRP__MASK
    if (atFlag == dainput.TYPEGRP_DIGITAL) {
      let bindings = colRange.map(@(col, _) dainput.get_digital_action_binding(ah, col))
      foreach (val in bindings)
        if (isValidDevice(val.devId) || val.devId == dainput.DEV_nullstub) {
          someBound = true
          break
        }
    }
    else if (dainput.TYPEGRP_AXIS == atFlag) {
      let axisBinding = colRange.map(@(col, _) dainput.get_analog_axis_action_binding(ah, col))
      foreach (val in axisBinding) {
        if (val.devId == dainput.DEV_pointing || val.devId == dainput.DEV_joy || val.devId == dainput.DEV_gamepad || val.devId == dainput.DEV_nullstub) {
          // using device axis
          someBound = true
          break
        }

        // using 2 digital buttons
        if (isValidDevice(val.minBtn.devId) && isValidDevice(val.maxBtn.devId)) {
          someBound = true
          break
        }
      }
    }
    else if (dainput.TYPEGRP_STICK == atFlag) {
      let stickBinding = colRange.map(@(col, _) dainput.get_analog_stick_action_binding(ah, col))
      foreach (val in stickBinding) {
        if (val.devId == dainput.DEV_pointing || val.devId == dainput.DEV_joy || val.devId == dainput.DEV_gamepad || val.devId == dainput.DEV_nullstub) {
          someBound = true
          break
        }
        if (isValidDevice(val.maxXBtn.devId) && isValidDevice(val.minXBtn.devId)
          && isValidDevice(val.maxYBtn.devId) && isValidDevice(val.minYBtn.devId)) {
          someBound = true
          break
        }
      }
    }

    if (!someBound) {
      let actionName = dainput.get_action_name(ah)
      let actionGroup = actionGroups?[0]
      if (actionGroup==null)
        continue
      if (!notBoundActions?[actionGroup])
        notBoundActions[actionGroup] <- { header = loc($"controls/tab/{actionGroup}"), controls = [] }

      notBoundActions[actionGroup].controls.append(loc($"controls/{actionName}", actionName))
    }
  }

  if (notBoundActions.len() == 0)
    return null

  let ret = []
  foreach (action in notBoundActions) {
    ret.append($"\n\n{action.header}:\n")
    ret.append(", ".join(action.controls))
  }

  return "".join(ret)
}

function onDiscardChanges() {
  showMsgbox({
    text = loc("settings/onCancelChangingConfirmation")
    buttons = [
      { text=loc("Yes"), action = discardChanges }
      { text=loc("No") }
    ]
  })
}

let skipDirPadNav = { skipDirPadNav = true }
let applyHotkeys = { hotkeys = [["^Esc"]] }

let onClose = @() showControlsMenu(false)

function mkWindowButtons(width) {
  function onApply() {
    let notBoundActions = getNotBoundActions()
    if (notBoundActions == null) {
      saveChanges()
      onClose()
      return
    }

    showMsgbox({
      text = "".concat(loc("controls/warningUnmapped"), notBoundActions)
      buttons = [
        { text=loc("Ok"),
          action = function() {
            saveChanges()
            onClose()
          }
        }
        { text = loc("Cancel"), action = @() null }
      ]
    })
  }

  return @() {
    watch = haveChanges
    size = [width, SIZE_TO_CONTENT]
    vplace = ALIGN_BOTTOM
    hplace = ALIGN_RIGHT
    flow = FLOW_HORIZONTAL
    valign = ALIGN_CENTER
    rendObj = ROBJ_SOLID
    color = ControlBg
    children = wrap([
      textButton(loc("controls/btnResetToDefaults", "Reset to default"), resetToDefault, skipDirPadNav)
      haveChanges.value ? textButton(loc("mainmenu/btnDiscard", "Discard changes"), onDiscardChanges, skipDirPadNav) : null
      {size = [flex(), 0]}
      actionButtons
      textButton(loc(haveChanges.get() ? "Apply" : "Ok"), onApply, skipDirPadNav.__merge(applyHotkeys, {hplace = ALIGN_RIGHT}))
    ], {width, flowElemProto = {size = [flex(), SIZE_TO_CONTENT] halign = ALIGN_RIGHT, padding = fsh(0.5), gap = hdpx(10)}})
  }
}


function bindingTextFunc(text) {
  return {
    text
    color = TextNormal
    rendObj = ROBJ_TEXT
    padding = hdpx(4)
  }.__update(body_txt)
}


function mkActionRowLabel(name, group=null){
  return {
    rendObj = ROBJ_TEXT
    color = TextNormal
    text = locActionName(name)
    margin = [0, fsh(1), 0, 0]
    size = [flex(1.5), SIZE_TO_CONTENT]
    halign = ALIGN_RIGHT
    group
  }.__update(body_txt)
}

function mkActionRowCells(label, columns){
  let children = [label].extend(columns)
  if (columns.len() < 2)
    children.append({size=[flex(0.75), 0]})
  return children
}

function makeActionRow(_ah, name, columns, xmbNode, showBgGen) {
  let group = ElemGroup()
  let isOdd = showBgGen()
  let label = mkActionRowLabel(name, group)
  let children = mkActionRowCells(label, columns)
  return watchElemState(@(sf) {
    xmbNode
    key = name
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_HORIZONTAL
    valign = ALIGN_CENTER
    behavior = Behaviors.Button
    skipDirPadNav = true
    children
    rendObj = ROBJ_BOX
    fillColor = menuRowColor(sf, isOdd)
    borderWidth = (sf & S_HOVER) ? [hdpx(2), 0] : 0
    borderColor = BtnBdHover
    group
  })
}


let bindingColumnCellSize = [flex(1), fontH(240)]

function isCellSelected(cell_data, selection) {
  let selected = selection.value
  return (selected!=null) && selected.column==cell_data.column && selected.ah==cell_data.ah
    && selected.actionProp==cell_data.actionProp && selected.tag==cell_data.tag
}

function showDisabledMsgBox(){
  return showMsgbox({
    text = loc("controls/bindingDisabled")
    buttons = [
      { text = loc("Ok")}
    ]
  })
}


function bindedComp(elemList, group=null){
  return {
     group = group ?? ElemGroup()
     behavior = Behaviors.Marquee
     scrollOnHover = true
     size = SIZE_TO_CONTENT
     maxWidth = pw(100)
     flow = FLOW_HORIZONTAL
     valign = ALIGN_CENTER
     children = buildElems(elemList, {textFunc = bindingTextFunc, eventTypesAsTxt = true})
  }
}


function bindingCell(ah, column, action_prop, list, tag, selection, name=null, xmbNode=null) {
  let singleBtn = action_prop!=null
  let cellData = {
    ah=ah, column=column, actionProp=action_prop, singleBtn=singleBtn, tag=tag, name=name
  }

  let group = ElemGroup()
  let isForGamepad = isGamepadColumn(column)

  return watchElemState(function(sf) {
    let hovered = (sf & S_HOVER)
    let selected = isCellSelected(cellData, selection)
    let isBindable = isForGamepad || !isGamepad.value
    return {
      watch = [selection, isGamepad]
      size = bindingColumnCellSize

      behavior = isBindable ? Behaviors.Button : null
      group
      xmbNode = isBindable ? xmbNode : null
      padding = fsh(0.5)

      children = {
        rendObj = ROBJ_BOX
        fillColor = selected ? Color(0,0,0,255)
                  : hovered ? Color(40, 40, 40, 80)
                  : Color(0, 0, 0, 40)
        borderWidth = selected ? hdpx(2) : 0
        borderColor = TextNormal
        halign = ALIGN_CENTER
        valign = ALIGN_CENTER
        clipChildren = true

        size = flex()
        children = bindedComp(list, group)
      }

      function onDoubleClick() {
        let actionType = dainput.get_action_type(ah)
        if (isActionDisabledToCustomize(ah))
          return showDisabledMsgBox()

        if ((actionType & dainput.TYPEGRP__MASK) == dainput.TYPEGRP_DIGITAL
          || cellData.singleBtn || cellData.tag == "modifiers" || cellData.tag == "axis")
          startRecording(cellData)
        else
          configuredAxis(cellData)
      }

      onClick = isGamepad.value ? null : isActionDisabledToCustomize(ah) ? showDisabledMsgBox : @() selection(cellData)
      onHover = isGamepad.value ? @(on) selection(on ? cellData : null) : null

      function onDetach() {
        if (isCellSelected(cellData, selection))
          selection(null)
      }
    }
  })
}

let colorTextHdr = Color(120,120,120)
function bindingColHeader(typ){
  return {
    size = bindingColumnCellSize
    rendObj = ROBJ_TEXT
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    text = loc("/".concat("controls/type", platformId, typ),typ)
    color = colorTextHdr
  }
}


function mkBindingsHeader(colRange){
  let cols = colRange.map(@(v) bindingColHeader(isGamepadColumn(v)
                                                  ? loc("controls/type/pc/gamepad")
                                                  : loc("controls/type/pc/keyboard")))
  return {
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_HORIZONTAL
    children = mkActionRowCells(mkActionRowLabel(null), cols)
    valign = ALIGN_CENTER
    gap = fsh(2)
    rendObj = ROBJ_SOLID
    color = Color(0,0,0)
  }
}

function bindingsPage(_section_name=null) {
  let scrollHandler = ScrollHandler()
  let filteredActions = getActionsList()//.filter(@(ah) getActionTags(ah).indexof(section_name) != null)
  let xmbRootNode = XmbContainer({wrap=true})


  return function() {
    let colRange = collectBindableColumns()
    let toggleBg = makeBgToggle()

    let actionRows = []
    let header = mkBindingsHeader(colRange)
    foreach (_aidx, ah in filteredActions) {
      let actionName = dainput.get_action_name(ah)
      let actionType = dainput.get_action_type(ah)
      let atFlag = actionType & dainput.TYPEGRP__MASK
      if (dainput.TYPEGRP_DIGITAL == atFlag) {
        let bindings = colRange.map(@(col) dainput.get_digital_action_binding(ah, col))
        let colTexts = bindings.map(buildDigitalBindingText)
        let colComps = colTexts.map(@(col_text, idx) bindingCell(ah, colRange[idx], null, col_text, null, selectedBindingCell, actionName, XmbNode()))
        actionRows.append(makeActionRow(ah, actionName, colComps, XmbNode({isGridLine=true}), toggleBg))
      }
      else if (dainput.TYPEGRP_AXIS == atFlag || dainput.TYPEGRP_STICK ==atFlag) {
        let colTexts = colRange.map(@(col, _) textListFromAction(actionName, col))
        let colComps = colTexts.map(@(col_text, idx) bindingCell(ah, colRange[idx], null, col_text, null, selectedBindingCell, actionName, XmbNode()))
        actionRows.append(makeActionRow(ah, actionName, colComps, XmbNode({isGridLine=true}), toggleBg))
      }
    }

    let bindingsArea = makeVertScroll({
      xmbNode = xmbRootNode
      flow = FLOW_VERTICAL
      size = [flex(), SIZE_TO_CONTENT]
      clipChildren = true
      children = actionRows
      scrollHandler
    })

    return {
      size = flex()
      padding = [fsh(1), 0]
      flow = FLOW_VERTICAL
      key = {}
      animations = pageAnim
      children = [header, bindingsArea]
    }
  }
}

function optionRowContainer(children, isOdd, params) {
  return watchElemState(@(sf) params.__merge({
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_HORIZONTAL
    valign = ALIGN_CENTER
    //behavior = Behaviors.Button
    skipDirPadNav = true
    children = children
    rendObj = ROBJ_SOLID
    color = menuRowColor(sf, isOdd)
    gap = fsh(2)
    padding = [0, fsh(2)]
  }))
}

function optionRow(labelText, comp, isOdd) {
  let label = {
    rendObj = ROBJ_TEXT
    color = TextNormal
    text = labelText
    margin = fsh(1)
    size = [flex(1), SIZE_TO_CONTENT]
    //halign = ALIGN_CENTER
    halign = ALIGN_RIGHT
  }.__update(body_txt)

  let children = [
    label
    {
      size = [flex(), fontH(200)]
      //halign = ALIGN_CENTER
      halign = ALIGN_LEFT
      valign = ALIGN_CENTER
      children = comp
    }
  ]

  return optionRowContainer(children, isOdd, {key=labelText})
}

let invertFields = {
  [0] = "axisXinv",
  [1] = "axisYinv",
  [-1] = "invAxis",
}

function invertCheckbox(action_names, column, axis) {
  if (type(action_names) != "array")
    action_names = [action_names]

  let bindings = action_names.map(function(aname) {
    let ah = dainput.get_action_handle(aname, 0xFFFF)
    return dainput.get_analog_stick_action_binding(ah, column)
        ?? dainput.get_analog_axis_action_binding(ah, column)
  }).filter(@(b) b!=null)

  if (!bindings.len())
    return null

  let curInverses = bindings.map(@(b) b[invertFields[axis]])

  let valAnd = curInverses.reduce(@(a,b) a && b, true)
  let valOr = curInverses.reduce(@(a,b) a || b, false)

  let val = Watched((valAnd == valOr) ? valAnd : null)

  function setValue(new_val) {
    val.set(new_val)
    foreach (b in bindings)
      b[invertFields[axis]] = new_val
    haveChanges(true)
  }

  return checkbox({ var = val, setValue, xmbNode=XmbNode()})
}
let mkRounded = @(val) round_by_value(val, 0.01)

function axisSetupSlider(action_names, column, prop, params) {
  let group = ElemGroup()
  if (type(action_names) != "array")
    action_names = [action_names]

  let bindings = action_names.map(function(aname) {
    let ah = dainput.get_action_handle(aname, 0xFFFF)
    return dainput.get_analog_stick_action_binding(ah, column)
        ?? dainput.get_analog_axis_action_binding(ah, column)
  }).filter(@(binding) binding!=null)

  if (!bindings.len())
    return null

  let curSens = bindings.map(@(b) b[prop]).filter(@(prp) prp!=null)
  if (!curSens.len())
    return null
  let val = Watched(curSens[0]) // take the first one, because they will all be equal
  let opt = params.__merge({
    var = val,
    function setValue(new_val) {
      val(new_val)
      foreach (b in bindings)
        b[prop] = new_val
      haveChanges(true)
    }
  })
  return sliderWithText(opt.__merge({morphText= params?.morphText ?? mkRounded}, group))
}


let sensRanges = [
  {min = 0.05, max = 5.0, step = 0.05} // mouse
  {min = 0.05, max = 5.0, step = 0.05} // gamepad
  {min = 0.05, max = 5.0, step = 0.05} // mouse/kbd aux
]

function sensitivitySlider(action_names, column) {
  let params = sensRanges[column].__merge({
//    scaling = slider.scales.logarithmic
  })
  return axisSetupSlider(action_names, column, "sensScale", params)
}



function showDeadZone(val){
  return "{0}%".subst(round_by_value(val*100, 0.5))
}

let showRelScale = @(val) "{0}%".subst(round_by_value(val*10, 0.5))

/*
const minDZ = 0.0
const maxDZ = 0.4
const stepDZ = 0.01
function deadZoneScaleSlider(val, setVal){
  let opt = {var = val, min = minDZ, max = maxDZ, step=stepDZ, scaling = slider.scales.linear, setValue = setVal, morphText = showDeadZone}
  return sliderWithText(opt)
}
*/
let isUserConfigCustomized = Watched(false)
function checkUserConfigCustomized(){
  isUserConfigCustomized(dainput.is_user_config_customized())
}
function updateAll(...) { updateCurrentPreset(); checkUserConfigCustomized()}
generation.subscribe(updateAll)
haveChanges.subscribe(updateAll)

function sectionHeader(text) {
  return optionRowContainer({
    rendObj = ROBJ_TEXT
    text
    color = TextNormal
    padding = [fsh(3), fsh(1), fsh(1)]
  }.__update(h2_txt), false, {
    halign = ALIGN_CENTER
  })
}


function axisSetupWindow() {
  let cellData = configuredAxis.value
  let stickBinding = dainput.get_analog_stick_action_binding(cellData.ah, cellData.column)
  let axisBinding = dainput.get_analog_axis_action_binding(cellData.ah, cellData.column)
  let binding = stickBinding ?? axisBinding
  let actionTags = getActionTags(cellData.ah)

  let actionName = dainput.get_action_name(cellData.ah)
  let actionType = dainput.get_action_type(cellData.ah)

  let title = {
    rendObj = ROBJ_TEXT
    color = TextNormal
    text = loc($"controls/{actionName}", actionName)
    margin = fsh(2)
  }.__update(h2_txt)

  function buttons() {
    let children = []
    if (selectedAxisCell.value)
      children.append(
        textButton(loc("controls/bindBinding", "Bind Axis"),
          function() { startRecording(selectedAxisCell.value) })
        textButton(loc("controls/clearBinding", "Clear Binding"),
          function() {
            clearBinding(selectedAxisCell.value)
            nextGeneration()
          })
       )

    children.append(textButton(loc("mainmenu/btnOk", "OK"),
      function() { configuredAxis(null) },
      { hotkeys = [["^Esc"]] }))

    return {
      watch = selectedAxisCell

      size = [flex(), SIZE_TO_CONTENT]
      flow = FLOW_HORIZONTAL
      halign = ALIGN_RIGHT
      gap = hdpx(10)
      children
    }
  }

  let modifiersList = []
  for (local i=0, n=binding.modCnt; i<n; ++i) {
    let mod = binding.mod[i]
    modifiersList.append(format_ctrl_name(mod.devId, mod.btnId))
    if (i < n-1)
      modifiersList.append("+")
  }

  local toggleBg = makeBgToggle()
  let rows = [
    optionRow(loc("Modifiers"), bindingCell(cellData.ah, cellData.column, null, modifiersList, "modifiers", selectedAxisCell), toggleBg())
  ]

  local axisBindingTextList
  if (stickBinding) {
    axisBindingTextList = getSticksText(stickBinding)
  } else if (axisBinding) {
    axisBindingTextList = format_ctrl_name(axisBinding.devId, axisBinding.axisId, false)
  }

  toggleBg = makeBgToggle()

  rows.append(
    sectionHeader(loc("controls/analog-axis-section", "Analog axis"))

    optionRow(loc("controls/analog/axis", "Axis"), bindingCell(cellData.ah, cellData.column, null, axisBindingTextList ? [axisBindingTextList] : [], "axis", selectedAxisCell), toggleBg())

    stickBinding != null
      ? optionRow(loc("controls/analog/sensitivity", "Sensitivity"), sensitivitySlider(actionName, cellData.column), toggleBg())
      : null

    dainput.get_action_type(cellData.ah) != dainput.TYPE_STICK_DELTA
      ? optionRow(loc("controls/analog/deadzoneThres", "Deadzone"), axisSetupSlider(actionName, cellData.column, "deadZoneThres", {min=0, max=0.4,step=0.01, morphText=showDeadZone}), toggleBg())
      : null
    dainput.is_action_stateful(cellData.ah)
      ? optionRow(loc("controls/analog/relIncScale", "changeStep"), axisSetupSlider(actionName, cellData.column, "relIncScale", {min=0.1, max=10.0, step=0.1, morphText=showRelScale}), toggleBg())
      : null
    dainput.get_action_type(cellData.ah) != dainput.TYPE_STICK_DELTA
      ? optionRow(loc("controls/analog/nonlinearity", "Nonlinearity"), axisSetupSlider(actionName, cellData.column, "nonLin", {min=0, max=4, step=0.5}), toggleBg())
      : null

    dainput.get_action_type(cellData.ah) != dainput.TYPE_STICK_DELTA && stickBinding != null
      ? optionRow(loc("controls/analog/axisSnapAngK", "Axis snap factor"), axisSetupSlider(actionName, cellData.column, "axisSnapAngK", {min=0, max=1, step=0.025, morphText=showDeadZone}), toggleBg())
      : null

    stickBinding
      ? optionRow(loc("controls/analog/isInvertedX", "Invert X"), invertCheckbox(actionName, cellData.column, 0), toggleBg())
      : null
    stickBinding
      ? optionRow(loc("controls/analog/isInvertedY", "Invert Y"), invertCheckbox(actionName, cellData.column, 1), toggleBg())
      : null
    axisBinding
      ? optionRow(loc("controls/analog/isInverted", "Inverted"), invertCheckbox(actionName, cellData.column, -1), toggleBg())
      : null
  )

  toggleBg = makeBgToggle()

  if (!actionTags.contains("_noDigitalButtons_")){
    rows.append(sectionHeader(loc("controls/digital-buttons-section", "Digital buttons")))

    if (axisBinding) {
      if (actionType == dainput.TYPE_STEERWHEEL || dainput.is_action_stateful(cellData.ah)) {
        local texts = null
        if (isValidDevice(axisBinding.minBtn.devId))
          texts = [format_ctrl_name(axisBinding.minBtn.devId, axisBinding.minBtn.btnId, true)]
        else
          texts = []
        let cell = bindingCell(cellData.ah, cellData.column, "minBtn", texts, null, selectedAxisCell)
        rows.append(optionRow(loc($"controls/{actionName}/min", "Min Btn"), cell, toggleBg()))
      }

      local texts = null
      if (isValidDevice(axisBinding.maxBtn.devId))
        texts = [format_ctrl_name(axisBinding.maxBtn.devId, axisBinding.maxBtn.btnId, true)]
      else
        texts = []
      let cell = bindingCell(cellData.ah, cellData.column, "maxBtn", texts, null, selectedAxisCell)
      rows.append(optionRow(loc($"controls/{actionName}/max", "Max Btn"), cell, toggleBg()))

      if (dainput.is_action_stateful(cellData.ah)){
        local textsAdd = isValidDevice(axisBinding.decBtn.devId)
          ? [format_ctrl_name(axisBinding.decBtn.devId, axisBinding.decBtn.btnId, true)]
          : []
        local cellAdd = bindingCell(cellData.ah, cellData.column, "decBtn", textsAdd, null, selectedAxisCell)
        rows.append(optionRow(loc($"controls/{actionName}/dec", "DecreaseBtn"), cellAdd, toggleBg()))
        textsAdd = isValidDevice(axisBinding.incBtn.devId)
          ? [format_ctrl_name(axisBinding.incBtn.devId, axisBinding.incBtn.btnId, true)]
          : []
        cellAdd = bindingCell(cellData.ah, cellData.column, "incBtn", textsAdd, null, selectedAxisCell)
        rows.append(optionRow(loc($"controls/{actionName}/inc", "Increase Btn"), cellAdd, toggleBg()))

      }
    }
    else if (stickBinding) {
      let directions = [
        {locSuffix="X/min", axisId="axisXId", dirBtn="minXBtn", defLoc = "Min X"}
        {locSuffix="X/max", axisId="axisXId", dirBtn="maxXBtn", defLoc = "Max X"}
        {locSuffix="Y/max", axisId="axisYId", dirBtn="maxYBtn", defLoc = "Min Y"}
        {locSuffix="Y/min", axisId="axisYId", dirBtn="minYBtn", defLoc = "Max Y"}
      ]
      foreach (dir in directions) {
        let btn = stickBinding[dir.dirBtn]
        let texts = isValidDevice(btn.devId) ? [format_ctrl_name(btn.devId, btn.btnId, true)] : []

        rows.append(optionRow(loc($"controls/{actionName}{dir.locSuffix}", dir.defLoc),
          bindingCell(cellData.ah, cellData.column, dir.dirBtn, texts, null, selectedAxisCell), toggleBg()))
      }
    }
  }
  let children = [
    title

    makeVertScroll({
      flow = FLOW_VERTICAL
      key = "axis"
      size = [flex(), SIZE_TO_CONTENT]
      padding = [fsh(1), 0]
      clipChildren = true
      children = rows
    })

    buttons
  ]

  return {
    watch = actionRecording
    size = flex()
    behavior = Behaviors.Button
    stopMouse = true
    stopHotkeys = true
    skipDirPadNav = true
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    rendObj = ROBJ_WORLD_BLUR
    color = Color(190,190,190,255)

    function onClick() {
      configuredAxis(null)
    }

    children = {
      size = [sw(80), sh(80)]
      rendObj = ROBJ_WORLD_BLUR
      color = Color(120,120,120,255)
      flow = FLOW_VERTICAL
      halign = ALIGN_CENTER

      stopMouse = true
      stopHotkeys = true

      children
    }
  }
}


function actionTypeSelect(_cell_data, watched, value) {
  return watchElemState(function(sf) {
    return {
      behavior = Behaviors.Button
      function onClick() {
        watched.set(value)
      }
      watch = watched
      flow = FLOW_HORIZONTAL
      gap = hdpx(20)
      valign = ALIGN_CENTER
      children = [
        {
          halign = ALIGN_CENTER
          rendObj = ROBJ_TEXT
          text = (watched.get() == value) ? "[x]" : " o "
          color = (sf & S_HOVER) ? BtnTextHover : BtnTextNormal
        }.__update(body_txt)
        {
          rendObj = ROBJ_TEXT
          color = (sf & S_HOVER) ? BtnTextHover : BtnTextNormal
          text = loc(eventTypeLabels[value])
        }.__update(body_txt)
      ]
    }
  })
}

let selectEventTypeHdr = {
  size = [flex(), SIZE_TO_CONTENT]
  rendObj = ROBJ_TEXT
  text = loc("controls/actionEventType", "Action on")
  color = TextNormal
  halign = ALIGN_RIGHT
}.__update(body_txt)

function buttonSetupWindow() {
  let cellData = configuredButton.get()
  let binding = dainput.get_digital_action_binding(cellData.ah, cellData.column)
  let eventTypeValue = Watched(binding.eventType)
  let modifierType = Watched(binding.unordCombo)
  let needShowModType = Watched(binding.modCnt > 0)
  modifierType.subscribe(function(new_val) {
    binding.unordCombo = new_val
    haveChanges(true)
  })

  let currentBinding = {
    flow = FLOW_HORIZONTAL
    children = bindedComp(buildDigitalBindingText(binding))
  }
  eventTypeValue.subscribe(function(new_val) {
    binding.eventType = new_val
    haveChanges(true)
  })

  let actionName = dainput.get_action_name(cellData.ah)
  let title = {
    rendObj = ROBJ_TEXT
    color = TextNormal
    text = loc($"controls/{actionName}", actionName)
    margin = fsh(3)
  }.__update(h2_txt)

  let stickyToggle = Watched(binding.stickyToggle)
  stickyToggle.subscribe(function(new_val) {
    binding.stickyToggle = new_val
    haveChanges.set(true)
  })

  let buttons = {
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_HORIZONTAL
    halign = ALIGN_RIGHT
    children = [
      textButton(loc("mainmenu/btnOk", "OK"), function() {
        configuredButton.set(null)
      }, {
          hotkeys = [["^Esc"]]
      })
    ]
  }

  let isStickyToggle = {
    margin = [fsh(1), 0, 0, fsh(0.4)]
    children = checkbox({var = stickyToggle, title = loc("controls/digital/mode/toggle", "toggle")})
  }

  let eventTypesChildren = getAllowedBindingsTypes(cellData.ah)
    .map(@(eventType) actionTypeSelect(cellData, eventTypeValue, eventType))
    .append(isStickyToggle)

  let selectEventType = @() {
    flow = FLOW_VERTICAL
    size = [flex(), SIZE_TO_CONTENT]
    watch = eventTypeValue
    children = eventTypesChildren
  }

  let triggerTypeArea = {
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_HORIZONTAL
    valign = ALIGN_TOP
    gap = fsh(2)
    children = [
      selectEventTypeHdr
      selectEventType
    ]
  }

  let selectModifierType = @() {
    watch = needShowModType
    hplace = ALIGN_CENTER
    valign = ALIGN_CENTER
    children = needShowModType.get() ? [
      checkbox({var=modifierType, title = loc("controls/unordCombo", "Unordered combo")})
    ] : null
  }
  let children = [
    title
    {
      halign = ALIGN_CENTER
      flow = FLOW_VERTICAL
      children = [currentBinding, selectModifierType]
    }
    {
      halign = ALIGN_CENTER
      valign = ALIGN_CENTER
      children = triggerTypeArea
    }
    {size = flex(10) }
    buttons
  ]

  return {
    size = flex()
    behavior = Behaviors.Button
    skipDirPadNav = true
    stopMouse = true
    stopHotkeys = true
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    rendObj = ROBJ_WORLD_BLUR
    color = Color(190,190,190,255)

    function onClick() {
      configuredAxis(null)
    }

    children = {
      size = [sw(80), sh(80)]
      rendObj = ROBJ_WORLD_BLUR
      color = Color(120,120,120,255)
      flow = FLOW_VERTICAL
      halign = ALIGN_CENTER
      gap = hdpx(70)

      children = children
    }
  }
}

function controlsMenuUi() {
  let width = sw(90)
  let menu = {
    transform = {}
    hplace = ALIGN_CENTER
    vplace = ALIGN_CENTER
    size = [width, sh(85)]
    rendObj = ROBJ_WORLD_BLUR
    fillColor = Color(0,0,0,180)
    flow = FLOW_VERTICAL
    //stopHotkeys = true
    stopMouse = true

    children = [
      {
        size = flex()
        padding=[hdpx(5),hdpx(10)]
        children = bindingsPage()
      },
      mkWindowButtons(width)
    ]
  }

  let root = {
    key = "controls"
    size = [sw(100), sh(100)]
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    watch = [
      actionRecording, configuredAxis, configuredButton,
      generation, haveChanges
    ]

    children = [
      {
        size = [sw(100), sh(100)]
        stopHotkeys = true
        stopMouse = true
        rendObj = ROBJ_WORLD_BLUR
        color = Color(130,130,130)
      }
      actionRecording.value==null ? menu : null
      configuredAxis.value!=null ? axisSetupWindow : null
      configuredButton.value!=null ? buttonSetupWindow : null
      actionRecording.value!=null ? recordingWindow : null
    ]

    transform = {
      pivot = [0.5, 0.25]
    }
    animations = pageAnim
    behavior = DngBhv.ActivateActionSet
    actionSet = "StopInput"
  }

  return root
}

generation.subscribe(@(_) eventbus_send_foreign(CONTROLS_SETUP_CHANGED_EVENT_ID, null))
return {
  controlsMenuUi
  showControlsMenu
}
