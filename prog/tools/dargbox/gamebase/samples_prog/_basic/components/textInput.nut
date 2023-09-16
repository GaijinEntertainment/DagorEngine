from "%darg/ui_imports.nut" import *
from "string" import regexp, split_by_chars

/*
  todo:
    - somehow provide result of validation - maybe more complex type of inputState, like Watched({text=text isValid=true}))
    - important to know about language and CapsLock. The easiest way - show last symbol in password for 0.25 seconds before hide it with *

    - replace editor in enlisted with this component (it should be already suitable)
*/
let rexInt = regexp(@"[\+\-]?[0-9]+")
let function isStringInt(str){
  return rexInt.match(str) //better use one from string.nut
}

let rexFloat = regexp(@"(\+|-)?([0-9]+\.?[0-9]*|\.[0-9]+)([eE](\+|-)?[0-9]+)?")
let function isStringFloat(str){
  return rexFloat.match(str) //better use one from string.nut
}

let rexEng = regexp(@"[a-z,A-Z]*")
let function isStringEng(str){
  return rexEng.match(str)
}
let function isStringLikelyEmail(str, _verbose=true) {
// this check is not rfc fully compatible. We check that @ exist and correctly used, and that let and domain parts exist and they are correct length.
// Domain part also have at least one period and main domain at least 2 symbols
// also come correct emails on google are against RFC, for example a.a.a@gmail.com.

  if (type(str)!="string")
    return false
  let splitted = split_by_chars(str,"@")
  if (splitted.len()<2)
    return false
  local locpart = splitted[0]
  if (splitted.len()>2)
    locpart = "@".join(splitted.slice(0,-1))
  if (locpart.len()>64)
    return false
  let dompart = splitted[splitted.len()-1]
  if (dompart.len()>253 || dompart.len()<4) //RFC + domain should be at least x.xx
    return false
  let quotes = locpart.indexof("\"")
  if (quotes && quotes!=0)
    return false //quotes only at the begining
  if (quotes==null && locpart.indexof("@")!=null)
    return false //no @ without quotes
  if (dompart.indexof(".")==null || dompart.indexof(".")>dompart.len()-3) // warning disable: -func-can-return-null
    return false  //too short first level domain or no periods
  return true
}

let function defaultFrame(inputObj, group, sf) {
  return {
    rendObj = ROBJ_FRAME
    borderWidth = [hdpx(1), hdpx(1), 0, hdpx(1)]
    size = [flex(), SIZE_TO_CONTENT]
    color = (sf & S_KB_FOCUS) ? Color(180, 180, 180) : Color(120, 120, 120)
    group = group

    children = {
      rendObj = ROBJ_FRAME
      borderWidth = [0, 0, hdpx(1), 0]
      size = [flex(), SIZE_TO_CONTENT]
      color = (sf & S_KB_FOCUS) ? Color(250, 250, 250) : Color(180, 180, 180)
      group = group

      children = inputObj
    }
  }
}

let function isValidStrByType(str, inputType) {
  if (str=="")
    return true
  if (inputType=="mail")
     return isStringLikelyEmail(str)
  if (inputType=="num")
     return isStringInt(str) || isStringFloat(str)
  if (inputType=="integer")
     return isStringInt(str)
  if (inputType=="float")
     return isStringFloat(str)
  if (inputType=="lat")
     return isStringEng(str)
  return true
}

let defaultColors = {
  placeHolderColor = Color(80, 80, 80, 80)
  textColor = Color(255,255,255)
  backGroundColor = Color(28, 28, 28, 150)
  highlightFailure = Color(255,60,70)
}


let failAnim = @(trigger) {
  prop = AnimProp.color
  from = defaultColors.highlightFailure
  easing = OutCubic
  duration = 1.0
  trigger = trigger
}

let interactiveValidTypes = ["num","lat","integer","float"]

let function textInput(text_state, options={}, frameCtor=defaultFrame) {
  let group = ElemGroup()
  let {
    setValue = @(v) text_state(v), inputType = null,
    placeholder = null, showPlaceHolderOnFocus = false, password = null, maxChars = null,
    title = null, font = null, fontSize = null, hotkeys = null,
    size = [flex(), fontH(100)], textmargin = [sh(1), sh(0.5)], valignText = ALIGN_BOTTOM,
    margin = [sh(1), 0], padding = 0, borderRadius = hdpx(3), valign = ALIGN_CENTER,
    xmbNode = null, imeOpenJoyBtn = null, charMask = null,

    //handlers
    onBlur = null, onReturn = null,
    onEscape = @() set_kb_focus(null), onChange = null, onFocus = null, onAttach = null,
    onHover = null, onImeFinish = null
  } = options

  local {
    isValidResult = null, isValidChange = null
  } = options

  let colors = defaultColors.__merge(options?.colors ?? {})

  isValidResult = isValidResult ?? @(new_value) isValidStrByType(new_value, inputType)
  isValidChange = isValidChange
    ?? @(new_value) interactiveValidTypes.indexof(inputType) == null
      || isValidStrByType(new_value, inputType)

  let stateFlags = Watched(0)

  let function onBlurExt() {
    if (!isValidResult(text_state.value))
      anim_start(text_state)
    onBlur?()
  }

  let function onReturnExt(){
    if (!isValidResult(text_state.value))
      anim_start(text_state)
    onReturn?()
  }

  let function onEscapeExt(){
    if (!isValidResult(text_state.value))
      anim_start(text_state)
    onEscape()
  }

  let function onChangeExt(new_val) {
    onChange?(new_val)
    if (!isValidChange(new_val))
      anim_start(text_state)
    else
      setValue(new_val)
  }

  local placeholderObj = null
  if (placeholder != null) {
    let phBase = {
      text = placeholder
      rendObj = ROBJ_TEXT
      font
      fontSize
      color = colors.placeHolderColor
      animations = [failAnim(text_state)]
      margin = [0, sh(0.5)]
    }
    placeholderObj = placeholder instanceof Watched
      ? @() phBase.__update({ watch = placeholder, text = placeholder.value })
      : phBase
  }

  let inputObj = @() {
    watch = [text_state, stateFlags]
    rendObj = ROBJ_TEXT
    behavior = Behaviors.TextInput

    size
    font
    fontSize
    color = colors.textColor
    group
    margin = textmargin
    valign = valignText

    animations = [failAnim(text_state)]

    text = text_state.value
    title
    inputType = inputType
    password = password
    key = text_state

    maxChars
    hotkeys
    charMask

    onChange = onChangeExt

    onFocus
    onBlur   = onBlurExt
    onAttach
    onReturn = onReturnExt
    onEscape = onEscapeExt
    onHover
    onImeFinish
    xmbNode
    imeOpenJoyBtn

    children = (text_state.value?.len() ?? 0)== 0
        && (showPlaceHolderOnFocus || !(stateFlags.value & S_KB_FOCUS))
      ? placeholderObj
      : null
  }

  return @() {
    watch = [stateFlags]
    onElemState = @(sf) stateFlags(sf)
    margin
    padding

    rendObj = ROBJ_BOX
    fillColor = colors.backGroundColor
    borderWidth = 0
    borderRadius
    clipChildren = true
    size = [flex(), SIZE_TO_CONTENT]
    group
    animations = [failAnim(text_state)]
    valign

    children = frameCtor(inputObj, group, stateFlags.value)
  }
}


let export = class{
  defaultColors = defaultColors
  _call = @(_self, text_state, options = {}, frameCtor = defaultFrame) textInput(text_state, options, frameCtor)
}()

return export
