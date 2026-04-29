from "%darg/ui_imports.nut" import *
from "math" import min

let isString = @(v) type(v) == "string"

function splitTextToParts(fullText, replaceTable) {
  local parts = [fullText]
  foreach (key, comp in replaceTable) {
    let prevParts = parts
    parts = []
    foreach (text in prevParts) {
      if (!isString(text)) {
        parts.append(text)
        continue
      }
      local startIdx = 0
      local idx = text.indexof(key)
      while (idx != null) {
        if (idx > startIdx)
          parts.append(text.slice(startIdx, idx))
        if (type(comp) == "array")
          parts.extend(comp)
        else
          parts.append(comp)
        startIdx = idx + key.len()
        idx = text.indexof(key, startIdx)
      }
      if (startIdx < text.len())
        parts.append(text.slice(startIdx))
    }
  }
  return parts
}

function getSeparatorNextIdx(str, startIdx) {
  for (local i = startIdx; i < str.len(); i++) {
    let c = str[i]
    if (c == ' ' || c == '\t' || c == '\n')
      return i
  }
  return null
}

function getSeparatorPrevIdx(str, startIdx) {
  for (local i = startIdx; i >= 0; i--) {
    let c = str[i]
    if (c == ' ' || c == '\t' || c == '\n')
      return i
  }
  return null
}

function extractLastLine(text, mkTextarea) {
  let fullParH = calc_comp_size(mkTextarea(text))[1]
  local si = text.len()
  while (true) {
    si = getSeparatorPrevIdx(text, si - 1)
    if (si == null)
      break
    let par = text.slice(0, si)
    let parComp = mkTextarea(par)
    if (calc_comp_size(parComp)[1] < fullParH)
      return { t1 = par, t2 = text.slice(si + 1) }
  }
  return { t1 = "", t2 = text }
}

function extractFirstWords(text, fontParams, freeWidth) {
  local res = { t1 = "", t2 = text }
  local si = -1
  while (true) {
    si = getSeparatorNextIdx(text, si + 1)
    if (si == null)
      break
    let isLineBreak = text[si] == '\n'
    let words = text.slice(0, si)
    let wordsW = calc_str_box(words, fontParams)[0]
    if (wordsW != 0 && wordsW < freeWidth)
      res = { t1 = words, t2 = text.slice(si + (isLineBreak ? 0 : 1)) }
    else if (wordsW != 0)
      break
    if (isLineBreak)
      break
  }
  return res
}

let pushLine = @(arr, line) arr.append({
  flow = FLOW_HORIZONTAL
  children = line
})

function mkProps(textareaProps) {
  let { font = null, fontSize = null, size = null, maxWidth = 0 } = textareaProps
  let sz = size?[0] ?? size ?? 0
  return  {
    maxWidth = sz > 0 && maxWidth > 0 ? min(sz, maxWidth)
      : sz > 0 ? sz
      : maxWidth
    fontParams = { font, fontSize }
    inlineTextProps = textareaProps.__merge({ size = null, behavior = null, rendObj = ROBJ_TEXT })
  }
}

/**
 * mkTextareaBlock() func takes some text (like from localization), replaces some tokens with daRG components
 * (like clickable text links, buttons, icons, etc.), forms the rest of the text using textareaProps,
 * and returns a daRG component that looks like a solid textarea.
 * @param {string}  fullText - Input string.
 * @param {table}  textareaProps - textarea daRG component props, must contain "size" or "maxWidth" prop.
 * @param {table} replaceTable - Map of daRG components (or arrays of daRG components) by text tokens.
 * @return {table} - daRG component.
 *
 * Usage example:
 *
 * let text = "This is a long multiline text containing links like <<LINK1>> or like <<LINK2>> or like <<LINK3>>, which can be clickable buttons."
 * let textareaProps = { maxWidth = hdpx(380), rendObj = ROBJ_TEXTAREA, behavior = Behaviors.TextArea }.__update(fontSmall)
 * let mkLink = @(text) { text, rendObj = ROBJ_TEXT, color = 0xFF1697E1 }.__update(fontSmall)
 * let replaceTable = {
 *   ["<<LINK1>>"] = mkLink("Link #1"),
 *   ["<<LINK2>>"] = mkLink("Link #2"),
 *   ["<<LINK3>>"] = mkLink("Link #3"),
 * }
 * let myComponent = mkTextareaBlock(text, textareaProps, replaceTable)
 */
function mkTextareaBlock(fullText, textareaProps, replaceTable) {
  let { maxWidth, fontParams, inlineTextProps } = mkProps(textareaProps)
  if (maxWidth <= 0) {
    assert(false, "Table textareaProps must contain a valid \"size\" or \"maxWidth\" prop")
    return null
  }
  let mkTextarea = @(text) textareaProps.__merge({ text })
  let mkInlineText = @(text) inlineTextProps.__merge({ text })
  if (fullText.contains("\r"))
    fullText = fullText.replace("\r\n", "\n")
  let parts = splitTextToParts(fullText, replaceTable)
  let totalParts = parts.len()
  local res = []
  for (local i = 0; i < totalParts; i++) {
    let elem = parts[i]
    if (isString(elem))
      res.append(elem)
    else {
      local line = []
      local lineW = 0
      let prevElem = parts?[i - 1]
      if (isString(prevElem)) {
        // Trying to use the last line of previous textarea
        let { t1, t2 } = extractLastLine(prevElem, mkTextarea)
        if (t2 != "") {
          let lineStartText = t2.lstrip()
          let lineStartTextW = calc_str_box(lineStartText, fontParams)[0]
          let elemW = calc_comp_size(elem)[0]
          if (lineStartTextW + elemW <= maxWidth) {
            res.pop()
            if (t1 != "")
              res.append(t1)
            line.append(mkInlineText(lineStartText))
            lineW = lineStartTextW
          }
        }
        else if (prevElem?[prevElem.len() - 1] == '\n') {
          res.pop()
          if (t1 != "")
            res.append(t1)
        }
      }
      // Putting elems to lines, until last line is filled, and next elem is text
      for (local ii = i; ii < totalParts; ii++) {
        let curElem = parts[ii]
        let isElemText = isString(curElem)
        local isProcessed = false
        let canTryFitInline = !isElemText || !curElem.contains("\n")
        if (canTryFitInline) {
          // Try to fit it inline
          let comp = isElemText ? mkInlineText(curElem) : curElem
          let compW = calc_comp_size(comp)[0]
          let isFit = lineW + compW <= maxWidth
          let needForceFit = line.len() == 0
          if (isFit || needForceFit) {
            line.append(comp)
            lineW += compW
            i = ii
            isProcessed = true
          }
        }
        if (!isProcessed && isElemText) {
          // Try to fit at least some first words
          let { t1, t2 } = extractFirstWords(curElem, fontParams, maxWidth - lineW)
          if (t1 != "") {
            line.append(mkInlineText(t1))
            parts[ii] = t2
          }
          // And quit from line mode
          break
        }
        if (!isProcessed) {
          // Lets fit it in a new empty line
          if (line.len())
            pushLine(res, line)
          line = []
          lineW = 0
          ii--
        }
      }
      if (line.len())
        pushLine(res, line)
    }
  }
  return {
    maxWidth
    flow = FLOW_VERTICAL
    children = res.map(@(t) isString(t) ? mkTextarea(t) : t)
  }
}

return mkTextareaBlock