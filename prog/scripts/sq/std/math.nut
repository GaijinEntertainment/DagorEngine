let math = require("math")

const GOLDEN_RATIO = 1.618034

function minByAbs(a, b) { return (math.fabs(a) < math.fabs(b))? a : b }
function maxByAbs(a, b) { return (math.fabs(a) > math.fabs(b))? a : b }

//round @value to valueble @digits amount
// roundToDigits(1.23, 2) = 1.2
// roundToDigits(123, 2) = 120
function roundToDigits(value, digits) {
  if (value==0) return value
  let log = math.log10(math.fabs(value))
  let mul = math.pow(10, math.floor(log)-digits+1)
  return mul*math.floor(0.5+value.tofloat()/mul)
}

//round @value by @roundValue
//round_by_value(1.56, 0.1) = 1.6
function round_by_value(value, roundValue) {
  return math.floor(value.tofloat() / roundValue + 0.5) * roundValue
}


function number_of_set_bits(i) {
  i = i - ((i >> 1) & (0x5555555555555555));
  i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);
  return (((i + (i >> 4)) & 0xF0F0F0F0F0F0F0F) * 0x101010101010101) >> 56;
}


function is_bit_set(bitMask, bitIdx) {
  return (bitMask & 1 << bitIdx) > 0
}

function change_bit(bitMask, bitIdx, value) {
  return (bitMask & ~(1 << bitIdx)) | (value? (1 << bitIdx) : 0)
}

function change_bit_mask(bitMask, bitMaskToSet, value) {
  return (bitMask & ~bitMaskToSet) | (value? bitMaskToSet : 0)
}

/**
* Linear interpolation of f(value) where:
* f(valueMin) = resMin
* f(valueMax) = resMax
*/
function lerp(valueMin, valueMax, resMin, resMax, value) {
  if (valueMin == valueMax)
    return 0.5 * (resMin + resMax)
  return resMin + (resMax - resMin) * (value - valueMin) / (valueMax - valueMin)
}

/**
* Linear interpolation of f(value) where:
* f(valueMin) = resMin
* f(valueMax) = resMax
* but result is clamped by min/max values
*/
let lerpClamped = @(valueMin, valueMax, resMin, resMax, tvalue)
  lerp(valueMin, valueMax, resMin, resMax,
    valueMax > valueMin ? math.clamp(tvalue, valueMin, valueMax) : math.clamp(tvalue, valueMax, valueMin))


function interpolateArray(arr, value) {
  let maxIdx = arr.len() - 1
  foreach (idx, curElem in arr) {
    if (value <= curElem.x || idx == maxIdx)
      return curElem.y

    let nextElem = arr[idx + 1]
    if (value > nextElem.x)
      continue

    let valueMin = curElem.x
    let resMin = curElem.y
    let valueMax = nextElem.x
    let resMax = nextElem.y
    return lerp(valueMin, valueMax, resMin, resMax, value)
  }
  return 0
}

/*
* return columns amount for the table with <total> same size items
* with a closer table size to golden ratio
* <widthToHeight> is a item size ratio (width / height)
*/
function calc_golden_ratio_columns(total, widthToHeight = 1.0) {
  let rows = (math.sqrt(total.tofloat() / GOLDEN_RATIO * widthToHeight) + 0.5).tointeger() || 1
  return math.ceil(total.tofloat() / rows).tointeger()
}

function color2uint(r,g=0,b=0,a=255){
  if (type(r)=="table") {
    r = r?.r ?? r
    g = r?.g ?? g
    b = r?.b ?? b
    a = r?.a ?? a
  }
  return math.clamp(r+g*256+b*65536+a*16777216, 0, 4294967295)
}

let romanNumeralLookup = [
  "","I","II","III","IV","V","VI","VII","VIII","IX",
  "","X","XX","XXX","XL","L","LX","LXX","LXXX","XC",
  "","C","CC","CCC","CD","D","DC","DCC","DCCC","CM"
]
let maxRomanDigit = 3

//Function from http://blog.stevenlevithan.com/archives/javascript-roman-numeral-converter
function getRomanNumeral(num) {
  let t = type(num)
  if ((t != "integer" && t != "float") || num < 0)
    return ""

  num = num.tointeger()
  if (num >= 4000)
    return num.tostring()

  let thousands = []
  for (local n = 0; n < num / 1000; n++)
    thousands.append("M")

  let roman = []
  local i = -1
  while (num > 0 && i++ < maxRomanDigit) {
    let digit = num % 10
    num = num / 10
    roman.insert(0, romanNumeralLookup?[digit + (i * 10)])
  }
  return "".join(thousands.extend(roman).filter(@(v) v!=null))
}

function splitThousands(val, spacer = " ") {
  val = val.tostring()
  local res = val.slice(-3)
  while (val.len() > 3) {
    val = val.slice(0, -3)
    res = $"{val.slice(-3)}{spacer}{res}"
  }
  return res
}

/**
 * Calculates average value from array.
 * @param {array} list - Array of integers or floats.
 * @return {float|null} - Average value, or null for empty array.
 */
function average(list) {
  let n = list.len()
  return n == 0 ? null
    : list.reduce(@(sum, v) sum + v, 0.0) / n
}

/**
 * Calculates median value from array.
 * @param {array} sortedList - Array of integers or floats. MUST be sorted!
 * @return {float|null} - Median value, or null for empty array.
 */
function median(sortedList) {
  let n = sortedList.len()
  return n == 0 ? null
    : (n % 2 == 1) ? (sortedList[(n - 1) / 2] * 1.0)
    : (sortedList[(n / 2) - 1] + sortedList[n / 2]) / 2.0
}

//EXPORT content for require
let export = math.__merge({
  GOLDEN_RATIO
  minByAbs
  maxByAbs
  round_by_value
  number_of_set_bits
  roundToDigits
  is_bit_set
  change_bit
  change_bit_mask
  lerp
  lerpClamped
  interpolateArray
  calc_golden_ratio_columns
  color2uint
  getRomanNumeral
  splitThousands
  calcPercent = @(value) (100.0 * value + 0.5).tointeger()
  average
  median
})

return export
