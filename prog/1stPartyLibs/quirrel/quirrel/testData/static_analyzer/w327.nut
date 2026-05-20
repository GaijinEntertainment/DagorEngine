function bucketsDescending(x) {
  if (x > 10)
    return 1
  else if (x > 0) // OK
    return 2
  return 0
}

function unreachableGreater(x) {
  if (x > 0)
    return 1
  else if (x > 10)
    return 2
  return 0
}

function unreachableLower(x) {
  if (x < 10)
    return 1
  else if (x < 0)
    return 2
  return 0
}

function normalizedOperandOrder(x) {
  if (0 < x)
    return 1
  else if (10 < x)
    return 2
  return 0
}

const MAX_VAL = 10000
let MIN_VAL = 10
let FLOAT_VAL = 10.5

function constantBounds(x) {
  if (x > MIN_VAL)
    return 1
  else if (x > MAX_VAL)
    return 2

  if (FLOAT_VAL > x)
    return 3
  else if (0.25 > x)
    return 4

  return 0
}

function negativeGreaterBound(x) {
  if (x > -10)
    return 1
  else if (x > -5)
    return 2
  return 0
}

function negativeFloatLowerBound(x) {
  if (x < -5.5)
    return 1
  else if (x < -10.25)
    return 2
  return 0
}

function laterArmCoveredByPreviousElseIf(x) {
  if (x > 10)
    return 1
  else if (x > 0)
    return 2
  else if (x > 5)
    return 3
  return 0
}

return [
  bucketsDescending,
  unreachableGreater,
  unreachableLower,
  normalizedOperandOrder,
  constantBounds,
  negativeGreaterBound,
  negativeFloatLowerBound,
  laterArmCoveredByPreviousElseIf
]
