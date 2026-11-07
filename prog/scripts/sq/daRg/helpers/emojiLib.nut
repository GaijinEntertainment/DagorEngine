from "%darg/ui_imports.nut" import *
from "math" import min, max, round
from "string" import format
import "utf8" as utf8
import "%darg/helpers/mkTextareaBlock.nut" as mkTextareaBlock

const DEF_ICON_SIZE_MUL = 0.95
const UNKNOWN_GLYPH = "\uFFFD"

local logFunc = null
local emojiSvgNames = []
local svgNameToPath = @(svgName, imgSize) $"ui/gameuiskin#{svgName}.svg:{imgSize}"
local emojiCharcodesToSvgName = @(emojiSequence) "-".join(emojiSequence.map(@(c) format("%x", c)))
local emojiSvgNameToCharcodes = @(svgName) svgName.split("-").map(@(s) s.tointeger(16))

// See https://www.unicode.org/Public/18.0.0/emoji/emoji-sequences.txt
let baseEmojiRanges = [
  [0x231A, 0x231B], [0x23E9, 0x23EC], [0x23F0, 0x23F0], [0x23F3, 0x23F3], [0x25FD, 0x25FE],
  [0x2614, 0x2615], [0x2648, 0x2653], [0x267F, 0x267F], [0x2693, 0x2693], [0x26A1, 0x26A1],
  [0x26AA, 0x26AB], [0x26BD, 0x26BE], [0x26C4, 0x26C5], [0x26CE, 0x26CE], [0x26D4, 0x26D4],
  [0x26EA, 0x26EA], [0x26F2, 0x26F3], [0x26F5, 0x26F5], [0x26FA, 0x26FA], [0x26FD, 0x26FD],
  [0x2705, 0x2705], [0x270A, 0x270B], [0x2728, 0x2728], [0x274C, 0x274C], [0x274E, 0x274E],
  [0x2753, 0x2755], [0x2757, 0x2757], [0x2795, 0x2797], [0x27B0, 0x27B0], [0x27BF, 0x27BF],
  [0x2B1B, 0x2B1C], [0x2B50, 0x2B50], [0x2B55, 0x2B55], [0x1F004, 0x1F004], [0x1F0CF, 0x1F0CF],
  [0x1F18E, 0x1F18E], [0x1F191, 0x1F19A], [0x1F201, 0x1F201], [0x1F21A, 0x1F21A], [0x1F22F, 0x1F22F],
  [0x1F232, 0x1F236], [0x1F238, 0x1F23A], [0x1F250, 0x1F251], [0x1F300, 0x1F320], [0x1F32D, 0x1F335],
  [0x1F337, 0x1F37C], [0x1F37E, 0x1F393], [0x1F3A0, 0x1F3CA], [0x1F3CF, 0x1F3D3], [0x1F3E0, 0x1F3F0],
  [0x1F3F4, 0x1F3F4], [0x1F3F8, 0x1F43E], [0x1F440, 0x1F440], [0x1F442, 0x1F4FC], [0x1F4FF, 0x1F53D],
  [0x1F54B, 0x1F54E], [0x1F550, 0x1F567], [0x1F57A, 0x1F57A], [0x1F595, 0x1F596], [0x1F5A4, 0x1F5A4],
  [0x1F5FB, 0x1F64F], [0x1F680, 0x1F6C5], [0x1F6CC, 0x1F6CC], [0x1F6D0, 0x1F6D2], [0x1F6D5, 0x1F6D9],
  [0x1F6DC, 0x1F6DF], [0x1F6EB, 0x1F6EC], [0x1F6F4, 0x1F6FC], [0x1F7E0, 0x1F7EB], [0x1F7F0, 0x1F7F0],
  [0x1F90C, 0x1F93A], [0x1F93C, 0x1F945], [0x1F947, 0x1F9FF], [0x1FA70, 0x1FA7C], [0x1FA80, 0x1FAC6],
  [0x1FAC8, 0x1FAC8], [0x1FACC, 0x1FADD], [0x1FADF, 0x1FAFA]
]

// Legacy monochrome chars, which turns into colored Emoji by adding a special char like 0xFE0F.
let emojiStartExtraRanges = [
  [0x0023, 0x0023], [0x002A, 0x002A], [0x0030, 0x0039], [0x00A9, 0x00A9], [0x00AE, 0x00AE],
  [0x203C, 0x203C], [0x2049, 0x2049], [0x2122, 0x2122], [0x2139, 0x2139], [0x2194, 0x2199],
  [0x21A9, 0x21AA], [0x2328, 0x2328], [0x23CF, 0x23CF], [0x23ED, 0x23EF], [0x23F1, 0x23F2],
  [0x23F8, 0x23FA], [0x24C2, 0x24C2], [0x25AA, 0x25AB], [0x25B6, 0x25B6], [0x25C0, 0x25C1],
  [0x25FB, 0x25FC], [0x2600, 0x2604], [0x260E, 0x260E], [0x2611, 0x2611], [0x2618, 0x2618],
  [0x261D, 0x261D], [0x2620, 0x2620], [0x2622, 0x2623], [0x2626, 0x2626], [0x262A, 0x262A],
  [0x262E, 0x262F], [0x2638, 0x263A], [0x2640, 0x2640], [0x2642, 0x2642], [0x265F, 0x2660],
  [0x2663, 0x2663], [0x2665, 0x2666], [0x2668, 0x2668], [0x267B, 0x267B], [0x267E, 0x267E],
  [0x2692, 0x2692], [0x2694, 0x2697], [0x2699, 0x2699], [0x269B, 0x269C], [0x26A0, 0x26A0],
  [0x26A7, 0x26A7], [0x26B0, 0x26B1], [0x26C8, 0x26C8], [0x26CF, 0x26CF], [0x26D1, 0x26D1],
  [0x26D3, 0x26D3], [0x26E9, 0x26E9], [0x26F0, 0x26F1], [0x26F4, 0x26F4], [0x26F7, 0x26F9],
  [0x2702, 0x2702], [0x2708, 0x2709], [0x270C, 0x270D], [0x270F, 0x270F], [0x2712, 0x2712],
  [0x2714, 0x2714], [0x2716, 0x2716], [0x271D, 0x271D], [0x2721, 0x2721], [0x2733, 0x2734],
  [0x2744, 0x2744], [0x2747, 0x2747], [0x275F, 0x275F], [0x2763, 0x2764], [0x27A1, 0x27A1],
  [0x2934, 0x2935], [0x2B05, 0x2B07], [0x2B14, 0x2B15], [0x3030, 0x3030], [0x303D, 0x303D],
  [0x3297, 0x3297], [0x3299, 0x3299], [0x1F170, 0x1F171], [0x1F17E, 0x1F17F], [0x1F202, 0x1F202],
  [0x1F237, 0x1F237], [0x1F321, 0x1F321], [0x1F324, 0x1F32C], [0x1F336, 0x1F336], [0x1F37D, 0x1F37D],
  [0x1F396, 0x1F397], [0x1F399, 0x1F39B], [0x1F39E, 0x1F39F], [0x1F3CB, 0x1F3CE], [0x1F3D4, 0x1F3DF],
  [0x1F3F3, 0x1F3F3], [0x1F3F5, 0x1F3F5], [0x1F3F7, 0x1F3F7], [0x1F43F, 0x1F43F], [0x1F441, 0x1F441],
  [0x1F4FD, 0x1F4FD], [0x1F549, 0x1F54A], [0x1F56F, 0x1F570], [0x1F573, 0x1F579], [0x1F587, 0x1F587],
  [0x1F58A, 0x1F58D], [0x1F590, 0x1F590], [0x1F5A5, 0x1F5A5], [0x1F5A8, 0x1F5A8], [0x1F5B1, 0x1F5B2],
  [0x1F5BC, 0x1F5BC], [0x1F5C2, 0x1F5C4], [0x1F5D1, 0x1F5D3], [0x1F5DC, 0x1F5DD], [0x1F5DE, 0x1F5DE],
  [0x1F5E1, 0x1F5E1], [0x1F5E3, 0x1F5E3], [0x1F5E8, 0x1F5E8], [0x1F5EF, 0x1F5EF], [0x1F5F3, 0x1F5F3],
  [0x1F5FA, 0x1F5FA], [0x1F6CB, 0x1F6CB], [0x1F6CD, 0x1F6CF], [0x1F6E0, 0x1F6E5], [0x1F6E9, 0x1F6E9],
  [0x1F6F0, 0x1F6F0], [0x1F6F3, 0x1F6F3]
]

let isInRange = @(n, rangeStart, rangeEnd) rangeStart <= n && n <= rangeEnd
let isCharcodeBaseEmoji = @(charCode) baseEmojiRanges.findindex(@(r) isInRange(charCode, r[0], r[1])) != null
let canCharcodeStartEmojiSequence = @(charCode) emojiStartExtraRanges.findindex(@(r) isInRange(charCode, r[0], r[1])) != null
let isRegionalIndicator = @(charCode) isInRange(charCode, 0x1F1E6, 0x1F1FF)
let validCharsAfterLegacyChar = [ 0xFE0F, 0x200D, 0x20E3 ]

function isCharcodePartOfEmojiSequence(charCode, emojiSequence) {
  if (!emojiSequence.len())
    return false
  let prevCharCode = emojiSequence.top()
  return charCode == 0x200D // Zero Width Joiner (ZWJ)
    || charCode == 0xFE0F // Selector
    || charCode == 0x20E3 // Combining Enclosing Keycap
    || isInRange(charCode, 0x1F3FB, 0x1F3FF) // Skin tone modifier
    || isInRange(charCode, 0xE0020, 0xE007F) // Subdivision flag tags
    || (emojiSequence.len() == 1 && isRegionalIndicator(prevCharCode) && isRegionalIndicator(charCode)) // Country flags
    || (prevCharCode == 0x200D
        && (isCharcodeBaseEmoji(charCode) || canCharcodeStartEmojiSequence(charCode))) // Next char in ZWJ joined sequence
}

/**
 * prepareTextForEmojiSubst() is an universal Emoji parsing func, it can be used for any purpose.
 * It itself don't use daRG, and can be called even without initEmojiLib() call.
 * When using this func result for mkTextareaWithEmoji(), you can cache this result for speed,
 * it contains only Quirrel base variable types (strings, integers).
 * @param {string}  text - Input text.
 * @return {table} - result of parsing this text for Emoji, see below.
 *
 * Usage example:
 * prepareTextForEmojiSubst("Text \U0001F44D\U0001F3FF with Emoji \U0001F600!")
 * Result:
 * {
 *   textForSubst = "Text {1f44d-1f3ff} with Emoji {1f600}!"
 *   substMap = { "1f600" = [128512], "1f44d-1f3ff" = [128077 127999] }
 * }
 */
function prepareTextForEmojiSubst(text) {
  let textU = utf8(text)
  let charsTotal = textU.charCount()
  let textArr = []
  let substMap = {}
  let emojiSequence = []
  function pushEmoji() {
    let id = emojiCharcodesToSvgName(emojiSequence)
    textArr.append("".concat("{", id, "}"))
    if (id not in substMap)
      substMap[id] <- clone emojiSequence
    emojiSequence.clear()
  }
  for (local i = 0; i < charsTotal; i++) {
    let uCharStr = textU.slice(i, i + 1)
    let charCode = utf8.charToInt(uCharStr)
    if (isCharcodePartOfEmojiSequence(charCode, emojiSequence)) {
      emojiSequence.append(charCode)
      continue
    }
    if (emojiSequence.len())
      pushEmoji()
    if (isCharcodeBaseEmoji(charCode) || isRegionalIndicator(charCode)) {
      emojiSequence.append(charCode)
      continue
    }
    if (canCharcodeStartEmojiSequence(charCode)) {
      let nextCharCode = i < (charsTotal - 1)
        ? utf8.charToInt(textU.slice(i + 1, i + 2))
        : -1
      if (validCharsAfterLegacyChar.contains(nextCharCode)) {
        emojiSequence.append(charCode)
        continue
      }
    }
    textArr.append(uCharStr)
  }
  if (emojiSequence.len())
    pushEmoji()
  return {
    textForSubst = substMap.len() ? "".join(textArr) : text
    substMap
  }
}

/**
 * initEmojiLib() func should be called once in your project, to init this library for
 * mkTextareaWithEmoji() func usage.
 * @param {table}  cfg - Config:
 *      emojiSvgNames - array of supported SVG images for Emoji. You don't need all 4000 images,
 *                      just for example 300-400 most popular. You can take SVG images from
 *                      "Twemoji", or any similar library.
 *                      Example: [ "2639", "1f600", "1f926-200d-2642-fe0f", ... ]
 *      svgNameToPath - Optional override func for converting SVG filename and size to Picture() path.
 *      emojiCharcodesToSvgName - Optional override func for converting a single Emoji charcodes
 *                      array to SVG filename without extension.
 *                      If you are using "Twemoji", then you don't need to pass this param.
 *      emojiSvgNameToCharcodes - Optional override func for converting filename to charcodes array.
 *                      If you are using "Twemoji", then you don't need to pass this param.
 *      logFunc - Optional debug logging func, usually log().
 */
function initEmojiLib(cfg) {
  emojiSvgNames = cfg.emojiSvgNames
  svgNameToPath = cfg?.svgNameToPath ?? svgNameToPath
  emojiCharcodesToSvgName = cfg?.emojiCharcodesToSvgName ?? emojiCharcodesToSvgName
  emojiSvgNameToCharcodes = cfg?.emojiSvgNameToCharcodes ?? emojiSvgNameToCharcodes
  logFunc = cfg?.logFunc
}

/**
 * mkEmojiParams() func makes emojiParams config for usage in mkTextareaWithEmoji() func calls.
 * @param {table}  fontParams - font params for daRG text elems, like { font = ..., fontSize = ... }.
 * @param {float} [iconSizeMul] - Optional scale for icons, to make some margin between Emoji
                                  images. Should be <= 1.0 (1.0 is a full line height).
 * @return {table} - config for mkTextareaWithEmoji().
 */
function mkEmojiParams(fontParams, iconSizeMul = DEF_ICON_SIZE_MUL) {
  let lineH = round(calc_content_size(fontParams
    .__merge({ rendObj = ROBJ_TEXT, text = "X" }))[1]).tointeger()
  let border = round(max(1, lineH * 0.5 * (1.0 - min(1.0, iconSizeMul)))).tointeger()
  let imgSize = lineH - (2 * border)
  let imgPos = [border, border]
  return {
    lineH
    imgSize
    imgPos
  }
}

function mkEmojiIco(svgName, emojiParams) {
  let { lineH, imgSize, imgPos } = emojiParams
  return {
    size = lineH
    children = {
      size = imgSize
      pos = imgPos
      rendObj = ROBJ_IMAGE
      image = Picture(svgNameToPath(svgName, imgSize))
    }
  }
}

function mkEmojiSubstElem(emojiSequence, emojiParams) {
  local svgName = emojiCharcodesToSvgName(emojiSequence)
  local hasEmoji = emojiSvgNames.contains(svgName)
  if (!hasEmoji) {
    let baseSvgName = emojiCharcodesToSvgName([ emojiSequence[0] ])
    if (emojiSvgNames.contains(baseSvgName)) {
      svgName = baseSvgName
      hasEmoji = true
    }
  }
  if (!hasEmoji)
    logFunc?($"[EMOJI] Don't have img for {svgName}")
  return hasEmoji ? mkEmojiIco(svgName, emojiParams) : UNKNOWN_GLYPH
}

/**
 * mkTextareaWithEmoji() func takes "textForSubst" and "substMap" from prepareTextForEmojiSubst()
 * and builds daRG elem (using mkTextareaBlock func), which looks like textarea, but all supported
 * Emoji in text are replaced with SVG images, and all unsupported are replaced with UNKNOWN_GLYPH chars.
 * Returns daRG elem ready for placing in UI instead of textarea.
 * @param {string}  textForSubst - "textForSubst" from prepareTextForEmojiSubst().
 * @param {table}  substMap - "substMap" from prepareTextForEmojiSubst().
 * @param {table}  textareaParams - textarea daRG component props, should contain "size" or "maxWidth" prop.
 * @param {table} emojiParams - Params config, created by func mkEmojiParams().
 * @return {table} - daRG elem.
 *
 * Usage example:
 *
 * let text = "This is a multiline text \U0001F44D\U0001F3FF containing some Emoji \U0001F600!"
 * let textareaParams = { maxWidth = hdpx(380), rendObj = ROBJ_TEXTAREA, behavior = Behaviors.TextArea }.__update(fontSmall)
 * let emojiParams = mkEmojiParams(fontSmall)
 * let { textForSubst, substMap } = prepareTextForEmojiSubst(text)
 * let myComponent = mkTextareaWithEmoji(textForSubst, substMap, textareaParams, emojiParams)
 */
function mkTextareaWithEmoji(textForSubst, substMap, textareaParams, emojiParams) {
  let substElemsMap = substMap.map(@(v) mkEmojiSubstElem(v, emojiParams))
  return substElemsMap.len()
    ? mkTextareaBlock(textForSubst, textareaParams, substElemsMap)
    : textareaParams.__merge({ text = textForSubst })
}

/**
 * debugTestSupportedEmojiDetection() func checks how Emoji from emojiSvgNames list parses as
 * single Emoji codes in texts.
 */
function debugTestSupportedEmojiDetection() {
  local errors = 0
  foreach (svgName in emojiSvgNames) {
    let emojiSequence = emojiSvgNameToCharcodes(svgName)
    local checkSvgName = svgName
    if (emojiSequence.len() == 1 && canCharcodeStartEmojiSequence(emojiSequence[0])) {
      emojiSequence.append(0xFE0F)
      checkSvgName = emojiCharcodesToSvgName(emojiSequence)
    }
    let text = "".join(emojiSequence.map(@(c) utf8.intToChar(c)))
    let { substMap } = prepareTextForEmojiSubst(text)
    if (substMap.len() != 1 || substMap?[checkSvgName] == null) {
      console_print($"Failed to parse Emoji \"{svgName}\" as {checkSvgName}:", substMap) //-forbidden-function
      errors++
    }
  }
  console_print($"Checked {emojiSvgNames.len()}, errors: {errors}") //-forbidden-function
}

return {
  initEmojiLib
  mkEmojiParams
  prepareTextForEmojiSubst
  mkTextareaWithEmoji
  debugTestSupportedEmojiDetection
}
