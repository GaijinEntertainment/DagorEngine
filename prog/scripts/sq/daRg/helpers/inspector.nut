from "%darg/ui_imports.nut" import *

//let {locate_element_source, sh, ph} = require("daRg")
let {format} = require("string")
let utf8 = require_optional("utf8")
let {set_clipboard_text} = require("dagor.clipboard")
let fieldsMap = require("inspectorViews.nut")
let cursors = require("simpleCursors.nut")

let shown          = persist("shown", @() Watched(false))
let wndHalign      = persist("wndHalign", @() Watched(ALIGN_RIGHT))
let pickerActive   = persist("pickerActive", @() Watched(false))
let highlight      = persist("highlight", @() Watched(null))
let animHighlight  = Watched(null)
let pickedList     = persist("pickedList", @() Watched([], FRP_DONT_CHECK_NESTED))
let viewIdx        = persist("viewIdx", @() Watched(0))
let showRootsInfo  = persist("showRoots", @() Watched(false))
let showChildrenInfo=persist("showChildrenInfo", @() Watched(false))

let curData        = Computed(@() pickedList.value?[viewIdx.value])

let fontSize = sh(1.5)
let valColor = Color(155,255,50)

let function inspectorToggle() {
  shown(!shown.value)
  pickerActive(false)
  pickedList([])
  viewIdx(0)
  highlight(null)
}

let function textButton(text, action, isEnabled = true) {
  let stateFlags = Watched(0)

  let override = isEnabled
    ? {
        watch = stateFlags
        onElemState = isEnabled ? @(val) stateFlags.update(val) : null
        onClick = isEnabled ? action : null
      }
    : {}

  return function() {
    let sf = stateFlags.value
    let color = !isEnabled ? Color(80, 80, 80, 200)
      : sf & S_ACTIVE      ? Color(100, 120, 200, 255)
      : sf & S_HOVER       ? Color(110, 135, 220, 255)
      : sf & S_KB_FOCUS    ? Color(110, 135, 220, 255)
                           : Color(100, 120, 160, 255)
    return {
      rendObj = ROBJ_SOLID
      size = SIZE_TO_CONTENT
      behavior = Behaviors.Button
      focusOnClick = true
      color = color
      padding = [hdpx(5), hdpx(10)]
      children = {
        rendObj = ROBJ_TEXT
        text = text
        color = isEnabled ? 0xFFFFFFFF : 0xFFBBBBBB
      }
    }.__update(override)
  }
}

let function mkDirBtn(text, dir) {
  let isVisible = Computed(@() pickedList.value.len() > 1)
  let isEnabled = Computed(@() (viewIdx.value + dir) in pickedList.value)
  return @() {
    watch = [isVisible, isEnabled]
    children = !isVisible.value ? null
      : textButton(text, @() isEnabled.value ? viewIdx(viewIdx.value + dir) : null, isEnabled.value)
  }
}

let gap = { size = flex() }
let pickBtn = textButton("Pick", @() pickerActive(true))
let prevBtn = mkDirBtn("Prev", -1)
let nextBtn = mkDirBtn("Next", 1)
let closeBtn = textButton("Close", inspectorToggle)

let invAlign = @(align) align == ALIGN_LEFT ? ALIGN_RIGHT : ALIGN_LEFT

let function panelToolbar() {
  let alignBtn = textButton(wndHalign.value == ALIGN_RIGHT ? "<|" : "|>",
    @() wndHalign(invAlign(wndHalign.value)))
  return {
    watch = wndHalign
    size = [flex(), SIZE_TO_CONTENT]
    flow = FLOW_HORIZONTAL
    padding = sh(1)
    gap = sh(0.5)
    children = wndHalign.value == ALIGN_RIGHT
      ? [alignBtn, pickBtn, prevBtn, nextBtn, gap, closeBtn]
      : [closeBtn, gap, prevBtn, nextBtn, pickBtn, alignBtn]
  }
}

let cutText = utf8 ? @(text, num) utf8(text).slice(0, num)
  : @(text, num) text.slice(0, num)

let mkColorCtor = @(color) @(content) {
  flow = FLOW_HORIZONTAL
  gap = sh(0.5)
  children = [
    content.__merge({ size = SIZE_TO_CONTENT })
    { rendObj = ROBJ_SOLID, size = [ph(100), ph(100)], color }
  ]
}

let mkImageCtor = @(image) @(content) {
  size = [flex(), SIZE_TO_CONTENT]
  flow = FLOW_VERTICAL
  children = [
    content
    {
      rendObj = ROBJ_IMAGE
      maxHeight = sh(30)
      keepAspect = KEEP_ASPECT_FIT
      imageValign = ALIGN_TOP
      imageHalign = ALIGN_LEFT
      image
    }
  ]
}

let IMAGE_KEYS = ["image", "fallbackImage"]

let function getPropValueTexts(desc, key, textLimit = 0) {
  let val = desc[key]
  let tp = type(val)

  local text = null
  local valCtor = fieldsMap?[key][val]

  if (val == null) {
    text = "<null>"
  } else if (tp == "array") {
    text = ", ".join(val)
  } else if (IMAGE_KEYS.contains(key)) {
    text = val.tostring()
    valCtor = mkImageCtor(val)
  } else if (tp == "integer" && key.tolower().indexof("color") != null) {
    text = "".concat("0x", format("%16X", val).slice(8))
    valCtor = mkColorCtor(val)
  } else if (tp == "userdata" || tp == "userpointer") {
    text = "<userdata/userpointer>"
  } else {
    let s = val.tostring()
    if (textLimit <= 0)
      text = s
    else {
      text = cutText(s, textLimit)
      if (text.len() + 10 < s.len())
        valCtor = $"...({utf8?(s).charCount() ?? s.len()})"
      else
        text = s
    }
  }
  return { text, valCtor }
}

let textColor = @(sf) sf & S_ACTIVE ? 0xFFFFFF00
  : sf & S_HOVER ? 0xFF80A0FF
  : 0xFFFFFFFF

let function mkPropContent(desc, key, sf) {
  let { text, valCtor } = getPropValueTexts(desc, key, 200)
  local keyValue = $"{key.tostring()} = <color={valColor}>{text}</color>"
  if (type(valCtor) == "string")
    keyValue = $"{keyValue} {valCtor}"
  local content = {
    rendObj = ROBJ_TEXTAREA
    size = [flex(), SIZE_TO_CONTENT]
    behavior = Behaviors.TextArea
    color = textColor(sf)
    fontSize
    hangingIndent = sh(3)
    text = keyValue
  }
  if (type(valCtor) == "function")
    content = valCtor?(content)
  return content
}

let function propPanel(desc) {
  local pKeys = []
  if (type(desc) == "class")
    foreach (key, _ in desc)
      pKeys.append(key)
  else
    pKeys = desc.keys()
  pKeys.sort()

  return pKeys.map(function(k) {
    let stateFlags = Watched(0)
    return @() {
      watch = stateFlags
      size = [flex(), SIZE_TO_CONTENT]
      behavior = Behaviors.Button
      onElemState = @(sf) stateFlags(sf)
      onClick = @() set_clipboard_text(getPropValueTexts(desc, k).text)
      children = mkPropContent(desc, k, stateFlags.value)
    }
  })
}

let function elemLocationText(elem, builder, builder_func_name) {
  local text = "Source: unknown"

  let location = locate_element_source(elem)
  if (location)
    text = $"{location.stack}\n-------\n"
  return builder ? $"{text}\n(Function)\n{builder_func_name}" : $"{text}\n(Table)"
}

let function updatePickedList(data) {
  pickedList((data ?? [])
    .map(@(d) d.__merge({
      locationText = elemLocationText(d.elem, d.builder, d.builderFuncName)
    })))
  viewIdx(0)
  pickerActive(false)
  animHighlight(null)
}

let prepareCallstackText = @(text) //add /t for line wraps
  text.replace("/", "/\t")

let function clickableText(labelText, valueText, onClick = null, highlightBB = null) {
  let elemSF = Watched(0)
  return @() {
    watch = elemSF
    rendObj = ROBJ_TEXTAREA
    behavior = [Behaviors.TextArea, Behaviors.Button]
    function onElemState(sf) {
      elemSF(sf)
      if (highlightBB)
        animHighlight(sf & S_HOVER ? highlightBB : null)
    }
    onDetach = @() animHighlight(null)
    onClick = onClick ?? @() set_clipboard_text(valueText)
    fontSize
    color = textColor(elemSF.value)
    text = $"{labelText} = <color={valColor}>{valueText}</color>"
  }
}

let function rootsPanel(roots) {
  let rootsList = []
  foreach (root in roots) {
    let rootInfo = [get_element_info(root.elem)]
    rootsList.append(
      clickableText(root.name, root.builderFuncName, @() updatePickedList(rootInfo), root.bbox))
  }
  return @() {
    watch = showRootsInfo
    flow = FLOW_VERTICAL
    gap = hdpx(2) // gap here to avoid two buttons being selected at ones (they overlap otherwise) and this breaks highlighting
    children = [
      clickableText("---show roots---", showRootsInfo.value, @() showRootsInfo(!showRootsInfo.value))
    ].extend(showRootsInfo.value ? rootsList : [])
  }
}

let function childrenPanel(children) {
  let childrenList = []
  foreach (child in children) {
    let childInfo = [get_element_info(child.elem)]
    childrenList.append(
      clickableText(child.name, child.builderFuncName, @() updatePickedList(childInfo), child.bbox))
  }
  return @() {
    watch = showChildrenInfo
    flow = FLOW_VERTICAL
    gap = hdpx(2) // gap here to avoid two buttons being selected at ones (they overlap otherwise) and this breaks highlighting
    children = [
      clickableText(
        $"---show children ({childrenList.len()}) ---",
        showChildrenInfo.value,
        @() showChildrenInfo(!showChildrenInfo.value))
    ].extend(showChildrenInfo.value ? childrenList : [])
  }
}

let function details() {
  let res = {
    watch = curData
    size = flex()
  }
  let sel = curData.value
  if (sel == null)
    return res

  let summarySF = Watched(0)
  let summaryText = @() {
    watch = summarySF
    size = flex()
    rendObj = ROBJ_TEXTAREA
    behavior = [Behaviors.TextArea, Behaviors.WheelScroll, Behaviors.Button]
    onElemState = @(sf) summarySF(sf)
    onClick = @() set_clipboard_text(sel.locationText)
    text = prepareCallstackText(sel.locationText)
    fontSize
    color = textColor(summarySF.value)
    hangingIndent = sh(3)
  }

  let bb = sel.bbox
  let bbText = $"\{ pos = [{bb.x}, {bb.y}], size = [{bb.w}, {bb.h}] \}"
  let bbox = clickableText("bbox", bbText, null, bb)


  return res.__update({
    flow = FLOW_VERTICAL
    padding = [hdpx(5), hdpx(10)]
    children = [ bbox ]
      .extend(propPanel(sel.componentDesc))
      .append(rootsPanel(sel.roots), childrenPanel(sel.children), summaryText)
  })
}

let help = {
  rendObj = ROBJ_TEXTAREA
  size = [flex(), SIZE_TO_CONTENT]
  behavior = Behaviors.TextArea
  vplace = ALIGN_BOTTOM
  margin = [hdpx(5), hdpx(10)]
  fontSize
  text = @"L.Ctrl + L.Shift + I - switch inspector off\nL.Ctrl + L.Shift + P - switch picker on/off"
}

let hr = {
  rendObj = ROBJ_SOLID
  color = 0x333333
  size = [flex(), hdpx(1)]
}

let inspectorPanel = @() {
  watch = wndHalign
  rendObj = ROBJ_SOLID
  color = Color(0, 0, 50, 50)
  size = [sw(30), sh(100)]
  hplace = wndHalign.value
  behavior = Behaviors.Button
  clipChildren = true

  flow = FLOW_VERTICAL
  gap = hr
  children = [
    panelToolbar
    details
    help
  ]
}


let function highlightRect() {
  let res = { watch = highlight }
  let hv = highlight.value
  if (hv == null)
    return res
  return res.__update({
    rendObj = ROBJ_SOLID
    color = Color(50, 50, 0, 50)
    pos = [hv[0].x, hv[0].y]
    size = [hv[0].w, hv[0].h]

    children = {
      rendObj = ROBJ_FRAME
      color = Color(200, 0, 0, 180)
      size = [hv[0].w, hv[0].h]
    }
  })
}

let function animHighlightRect() {
  let res = {
    watch = animHighlight
    animations = [{
      prop = AnimProp.opacity, from = 0.5, to = 1.0, duration = 0.5, easing = CosineFull, play = true, loop = true
    }]
  }
  let ah = animHighlight.value
  if (ah == null)
    return res
  return res.__update({
    size = [ah.w, ah.h]
    pos = [ah.x, ah.y]
    rendObj = ROBJ_FRAME
    color = 0xFFFFFFFF
    fillColor = 0x40404040
  })
}


let elementPicker = @() {
  size = [sw(100), sh(100)]
  behavior = Behaviors.InspectPicker
  cursor = cursors.normal
  rendObj = ROBJ_SOLID
  color = Color(20,0,0,20)
  onClick = updatePickedList
  onChange = @(hl) highlight(hl)
  children = highlightRect
}


let function inspectorRoot() {
  let res = {
    watch = [pickerActive, shown]
    size = [sw(100), sh(100)]
    zOrder = getroottable()?.Layers.Inspector ?? 10
    skipInspection = true
  }

  if (shown.value)
    res.__update({
      cursor = cursors.normal
      children = [
        (pickerActive.value ? elementPicker : inspectorPanel),
        animHighlightRect,
        { hotkeys = [
          ["L.Ctrl L.Shift I", @() shown(false)],
          ["L.Ctrl L.Shift P", @() pickerActive(!pickerActive.value)]
        ] }
      ]
    })

  return res
}

return {
  inspectorToggle
  inspectorRoot
}
