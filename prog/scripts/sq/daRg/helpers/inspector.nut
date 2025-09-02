from "string" import format
from "dagor.clipboard" import set_clipboard_text
from "%darg/ui_imports.nut" import *
import "inspectorViews.nut" as fieldsMap
import "simpleCursors.nut" as cursors

//let {locate_element_source, sh, ph} = require("daRg")
let utf8 = require_optional("utf8")

let shown          = persist("shown", @() Watched(false))
let wndHalign      = persist("wndHalign", @() Watched(ALIGN_RIGHT))
let pickerActive   = persist("pickerActive", @() Watched(false))
let highlight      = persist("highlight", @() Watched(null))
let animHighlight  = Watched(null)
let pickedList     = persist("pickedList", @() Watched([], FRP_DONT_CHECK_NESTED))
let viewIdx        = persist("viewIdx", @() Watched(0))
let showRootsInfo  = persist("showRoots", @() Watched(false))
let showChildrenInfo=persist("showChildrenInfo", @() Watched(false))

let curData        = Computed(@() pickedList.get()?[viewIdx.get()])

let fontSize = sh(1.5)
let valColor = Color(155,255,50)

function inspectorToggle() {
  shown.modify(@(v) !v)
  pickerActive.set(false)
  pickedList.set([])
  viewIdx.set(0)
  highlight.set(null)
}

function textButton(text, action, isEnabled = true) {
  let stateFlags = Watched(0)

  let override = isEnabled
    ? {
        watch = stateFlags
        onElemState = isEnabled ? @(val) stateFlags.set(val) : null
        onClick = isEnabled ? action : null
      }
    : {}

  return function() {
    let sf = stateFlags.get()
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
      padding = static [hdpx(5), hdpx(10)]
      children = {
        rendObj = ROBJ_TEXT
        text
        color = isEnabled ? 0xFFFFFFFF : 0xFFBBBBBB
      }
    }.__update(override)
  }
}

function mkDirBtn(text, dir) {
  let isVisible = Computed(@() pickedList.get().len() > 1)
  let isEnabled = Computed(@() (viewIdx.get() + dir) in pickedList.get())
  return @() {
    watch = [isVisible, isEnabled]
    children = !isVisible.get() ? null
      : textButton(text, @() isEnabled.get() ? viewIdx.set(viewIdx.get() + dir) : null, isEnabled.get())
  }
}

let gap = static { size = flex() }
let pickBtn = textButton("Pick", @() pickerActive.set(true))
let prevBtn = mkDirBtn("Prev", -1)
let nextBtn = mkDirBtn("Next", 1)
let closeBtn = textButton("Close", inspectorToggle)

let invAlign = @(align) align == ALIGN_LEFT ? ALIGN_RIGHT : ALIGN_LEFT

function panelToolbar() {
  let alignBtn = textButton(wndHalign.get() == ALIGN_RIGHT ? "<|" : "|>",
    @() wndHalign.set(invAlign(wndHalign.get())))
  return {
    watch = wndHalign
    size = FLEX_H
    flow = FLOW_HORIZONTAL
    padding = sh(1)
    gap = sh(0.5)
    children = wndHalign.get() == ALIGN_RIGHT
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
  size = FLEX_H
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

let IMAGE_KEYS = {"image":1, "fallbackImage":1}

function getPropValueTexts(desc, key, textLimit = 0) {
  let val = desc[key]
  let tp = type(val)

  local text = null
  local valCtor = fieldsMap?[key][val]

  if (val == null) {
    text = "<null>"
  } else if (tp == "array") {
    text = ", ".join(val)
  } else if (key in IMAGE_KEYS) {
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

function mkPropContent(desc, key, sf) {
  let { text, valCtor } = getPropValueTexts(desc, key, 200)
  local keyValue = $"{key.tostring()} = <color={valColor}>{text}</color>"
  if (type(valCtor) == "string")
    keyValue = $"{keyValue} {valCtor}"
  local content = {
    rendObj = ROBJ_TEXTAREA
    size = FLEX_H
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

function propPanel(desc) {
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
      size = FLEX_H
      behavior = Behaviors.Button
      onElemState = @(sf) stateFlags.set(sf)
      onClick = @() set_clipboard_text(getPropValueTexts(desc, k).text)
      children = mkPropContent(desc, k, stateFlags.get())
    }
  })
}

function elemLocationText(elem, builder, builder_func_name) {
  local text = "Source: unknown"

  let location = locate_element_source(elem)
  if (location)
    text = $"{location.stack}\n-------\n"
  return builder ? $"{text}\n(Function)\n{builder_func_name}" : $"{text}\n(Table)"
}

function updatePickedList(data) {
  pickedList.set((data ?? [])
    .map(@(d) d.__merge({
      locationText = elemLocationText(d.elem, d.builder, d.builderFuncName)
    })))
  viewIdx.set(0)
  pickerActive.set(false)
  animHighlight.set(null)
}

let prepareCallstackText = @(text) //add /t for line wraps
  text.replace("/", "/\t")

function clickableText(labelText, valueText, onClick = null, highlightBB = null) {
  let elemSF = Watched(0)
  return @() {
    watch = elemSF
    rendObj = ROBJ_TEXTAREA
    behavior = static [Behaviors.TextArea, Behaviors.Button]
    function onElemState(sf) {
      elemSF.set(sf)
      if (highlightBB)
        animHighlight.set(sf & S_HOVER ? highlightBB : null)
    }
    onDetach = @() animHighlight.set(null)
    onClick = onClick ?? @() set_clipboard_text(valueText)
    fontSize
    color = textColor(elemSF.get())
    text = $"{labelText} = <color={valColor}>{valueText}</color>"
  }
}

function rootsPanel(roots) {
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
      clickableText("---show roots---", showRootsInfo.get(), @() showRootsInfo.set(!showRootsInfo.get()))
    ].extend(showRootsInfo.get() ? rootsList : [])
  }
}

function childrenPanel(children) {
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
        showChildrenInfo.get(),
        @() showChildrenInfo.set(!showChildrenInfo.get()))
    ].extend(showChildrenInfo.get() ? childrenList : [])
  }
}

function details() {
  let res = {
    watch = curData
    size = flex()
  }
  let sel = curData.get()
  if (sel == null)
    return res

  let summarySF = Watched(0)
  let summaryText = @() {
    watch = summarySF
    size = flex()
    rendObj = ROBJ_TEXTAREA
    behavior = [Behaviors.TextArea, Behaviors.WheelScroll, Behaviors.Button]
    onElemState = @(sf) summarySF.set(sf)
    onClick = @() set_clipboard_text(sel.locationText)
    text = prepareCallstackText(sel.locationText)
    fontSize
    color = textColor(summarySF.get())
    hangingIndent = sh(3)
  }

  let bb = sel.bbox
  let bbText = $"\{ pos = [{bb.x}, {bb.y}], size = [{bb.w}, {bb.h}] \}"
  let bbox = clickableText("bbox", bbText, null, bb)


  return res.__update({
    flow = FLOW_VERTICAL
    padding = static [hdpx(5), hdpx(10)]
    children = [ bbox ]
      .extend(propPanel(sel.componentDesc))
      .append(rootsPanel(sel.roots), childrenPanel(sel.children), summaryText)
  })
}

let help = static {
  rendObj = ROBJ_TEXTAREA
  size = FLEX_H
  behavior = Behaviors.TextArea
  vplace = ALIGN_BOTTOM
  margin = [hdpx(5), hdpx(10)]
  fontSize
  text = @"L.Ctrl + L.Shift + I - switch inspector off\nL.Ctrl + L.Shift + P - switch picker on/off"
}

let hr = static {
  rendObj = ROBJ_SOLID
  color = 0x333333
  size = [flex(), hdpx(1)]
}

let inspectorPanel = @() {
  watch = wndHalign
  rendObj = ROBJ_SOLID
  color = Color(0, 0, 50, 50)
  size = static [sw(30), sh(100)]
  hplace = wndHalign.get()
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


function highlightRect() {
  let res = { watch = highlight }
  let hv = highlight.get()
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

function animHighlightRect() {
  let res = {
    watch = animHighlight
    animations = static [{
      prop = AnimProp.opacity, from = 0.5, to = 1.0, duration = 0.5, easing = CosineFull, play = true, loop = true
    }]
  }
  let ah = animHighlight.get()
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
  size = static [sw(100), sh(100)]
  behavior = Behaviors.InspectPicker
  cursor = cursors.normal
  rendObj = ROBJ_SOLID
  color = Color(20,0,0,20)
  onClick = updatePickedList
  onChange = @(hl) highlight.set(hl)
  children = highlightRect
}


function inspectorRoot() {
  let res = {
    watch = [pickerActive, shown]
    size = [sw(100), sh(100)]
    zOrder = getroottable()?.Layers.Inspector ?? 10
    skipInspection = true
  }

  if (shown.get())
    res.__update({
      cursor = cursors.normal
      children = [
        (pickerActive.get() ? elementPicker : inspectorPanel),
        animHighlightRect,
        { hotkeys = [
          ["L.Ctrl L.Shift I", @() shown.set(false)],
          ["L.Ctrl L.Shift P", @() pickerActive.set(!pickerActive.get())]
        ] }
      ]
    })

  return res
}

return {
  inspectorToggle
  inspectorRoot
}
