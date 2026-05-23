//-file:all-paths-return-value

function sameReturn(x) {
  if (x)
    return 10
  return 10
}

function differentReturn(x) {
  if (x)
    return 10
  return 11
}

function sameStringReturn(x) {
  if (x)
    return "ok"
  return "ok"
}

function sameFloatReturn(x) {
  if (x)
    return 1.25
  return 1.25
}

function sameBoolReturn(x) {
  if (x)
    return false
  return false
}

function nonLiteralReturn(x) {
  if (x)
    return x + 1
  return x + 1
}

function bareReturnsOnly(x) {
  if (x)
    return
  return
}

function bareReturnWithSameValue(x) {
  if (x)
    return
  if (x > 0)
    return 7
  return 7
}

function nestedFunctionIgnored(x) {
  function _inner() {
    return x
  }
  return x
}

function sameYield(x) {
  if (x)
    yield 3
  yield 3
}

function nullReturn(x) {
  if (x)
    return null
  return null
}

return [
  sameReturn,
  differentReturn,
  sameStringReturn,
  sameFloatReturn,
  sameBoolReturn,
  nonLiteralReturn,
  bareReturnsOnly,
  bareReturnWithSameValue,
  nestedFunctionIgnored,
  sameYield,
  nullReturn
]
