from "types" import Null as NullType, Integer as IntegerType

function textHeight(style, Float, Integer, defButtonHeight) {
  let ovrHeight = style?.ovr.size[1]
  let txtHeight = ((ovrHeight instanceof Float || ovrHeight instanceof Integer) ? ovrHeight : defButtonHeight)
    - (style?.ovr.padding[0] ?? 0) * 2
  return txtHeight
}

function boundResult(style, Float, Integer) {
  let height = style?.height
  let result = (height instanceof Float || height instanceof Integer) ? height : 10
  return result - 2
}

function nestedChecks(style, Float, Integer, Number) {
  let height = style?.height
  return ((height instanceof Float || (height instanceof Integer || height instanceof Number)) ? (height) : 10) - 2
}

function directAccess(style, Float, Integer) {
  return ((style?.height instanceof Float || style?.height instanceof Integer) ? style?.height : 10) - 2
}

function nullChecks(style, enabled) {
  let height = style?.height
  let a = height != null ? height : 10
  let b = height == null ? 10 : height
  let c = (null != height && enabled) ? height : 10
  let d = (height == null || !enabled) ? 10 : height
  let e = height ? height : 10
  let f = !height ? 10 : height
  return a - b - c - d - e - f
}

function typeChecks(style, enabled) {
  let height = style?.height
  let a = ((typeof height == "float") || (typeof height == "integer")) ? height : 10
  let b = ((type(height) == "integer") && enabled) ? height : 10
  let c = typeof height == "null" ? 10 : height
  let d = !(typeof height == "null") ? height : 10
  return a - b - c - d
}

function mixedChecks(style, Float, enabled) {
  let height = style?.height
  let a = (height instanceof Float || height != null) ? height : 10
  let b = (height != null && enabled) ? height : 10
  let c = !(height instanceof Float) ? 10 : height
  let d = height != null ? (enabled ? height : 10) : 10
  return a - b - c - d
}

function accessChecks(style) {
  let a = style?.height != null ? style?.height : 10
  let b = style?.height == null ? 10 : style?.height
  let c = style?.height ? style?.height : 10
  let d = typeof style?.height == "null" ? 10 : style?.height
  let e = (style?.height != null || style?.height) ? style?.height : 10
  let f = style?.height == 5 ? style?.height : 10
  let g = typeof style?.height == "integer" ? style?.height : 10
  return a - b - c - d - e - f - g
}

function branchLeak(style) {
  let height = style?.height
  let result = height != null ? height : 10
  return height - result
}

function incompleteNullCheck(style, enabled) {
  let height = style?.height
  return ((height != null || enabled) ? height : 10) - 2
}

function incompleteAccessCheck(style) {
  return ((style?.height != null || style?.width != null) ? style?.height : 10) - 2
}

function reversedNullCheck(style) {
  let height = style?.height
  return (height == null ? height : 10) - 2
}

function differentValues(style, Float, Integer) {
  let height = style?.height
  let width = style?.width
  return ((height instanceof Float || width instanceof Integer) ? height : 10) - 2
}

function unrelatedCondition(style, Float, enabled) {
  let height = style?.height
  return ((height instanceof Float || enabled) ? height : 10) - 2
}

function uncheckedResult(style, Float, Integer) {
  let height = style?.height
  let width = style?.width
  return ((height instanceof Float || height instanceof Integer) ? width : 10) - 2
}

function nullableFallback(style, Float, Integer) {
  let height = style?.height
  return ((height instanceof Float || height instanceof Integer) ? height : null) - 2
}

function uncheckedElse(style, Float, Integer) {
  let height = style?.height
  return ((height instanceof Float || height instanceof Integer) ? 10 : height) - 2
}

function parenthesizedChecks(style, Float, Integer) {
  let height = style?.height
  let a = (height) instanceof Float ? (height) : 10
  let b = ((height) instanceof Float || (height) instanceof Integer) ? height : 10
  return a - b
}

function indexedChecks(x, i, Float, Integer) {
  let a = x?[i] != null ? x?[i] : 0
  let b = x?[i] == null ? 0 : x?[i]
  let c = x?[i] ? x?[i] : 0
  let d = typeof x?[i] == "integer" ? x?[i] : 0
  let e = (x?[i] instanceof Float || x?[i] instanceof Integer) ? x?[i] : 0
  let f = x?[i + 1] != null ? x?[i + 1] : 0
  let g = x?[i]?.height != null ? x?[i]?.height : 0
  let h = typeof x?[i] == "null" ? 0 : x?[i]
  return a - b - c - d - e - f - g - h
}

function uncheckedIndex(x, i, j) {
  return (x?[i] != null ? x?[j] : 0) - 2
}

function incompleteIndexCheck(x, i, enabled) {
  return ((x?[i] != null || enabled) ? x?[i] : 0) - 2
}

function nullableIndexFallback(x, i, j) {
  return (x?[i] != null ? x?[i] : x?[j]) - 2
}

function callRoot(foo) {
  return (foo()?.bar != null ? foo()?.bar : 0) - 2
}

function nullableCall(foo) {
  return (foo?() != null ? foo?() : 0) - 2
}

function callInIndex(x, index) {
  return (x?[index()] != null ? x?[index()] : 0) - 2
}

function changingIndex(x, i) {
  return (x?[i++] != null ? x?[i++] : 0) - 2
}

let NullAlias = NullType
let types = require("types")

function nullAlternative(style) {
  let height = style?.height
  return ((height instanceof NullType || height instanceof IntegerType) ? height : 10) - 2
}

function nullAlias(style) {
  let height = style?.height
  return ((height instanceof NullAlias || height instanceof IntegerType) ? height : 10) - 2
}

function nullMember(style) {
  let height = style?.height
  return ((height instanceof types.Null || height instanceof IntegerType) ? height : 10) - 2
}

function nullTypeChecks(style, x, i) {
  let height = style?.height
  let a = height instanceof NullType ? 10 : height
  let b = !(height instanceof NullAlias) ? height : 10
  let c = x?[i] instanceof NullType ? 0 : x?[i]
  return a - b - c
}

function nullIf(style) {
  let height = style?.height
  if (height instanceof NullType)
    return height - 2
  return 0
}

function nonBuiltinNull(style) {
  class Null {}
  let height = style?.height
  return (height instanceof Null ? height : 10) - 2
}

return {
  textHeight
  boundResult
  nestedChecks
  directAccess
  nullChecks
  typeChecks
  mixedChecks
  accessChecks
  branchLeak
  incompleteNullCheck
  incompleteAccessCheck
  reversedNullCheck
  differentValues
  unrelatedCondition
  uncheckedResult
  nullableFallback
  uncheckedElse
  parenthesizedChecks
  indexedChecks
  uncheckedIndex
  incompleteIndexCheck
  nullableIndexFallback
  callRoot
  nullableCall
  callInIndex
  changingIndex
  nullAlternative
  nullAlias
  nullMember
  nullTypeChecks
  nullIf
  nonBuiltinNull
}
