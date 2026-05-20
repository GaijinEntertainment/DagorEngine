// -file:duplicate-if-expression

function nestedSameCondition(a, b) {
  if (a == b) {
    if (a == b)  // WARN 1
      return 1
  }
  return 0
}

function elseSameCondition(a, b) {
  if (a == b)
    return 1
  else {
    if (a == b)  // WARN 2
      return 2
  }
  return 0
}

function elseIfSameCondition(a, b) {
  if (a == b)
    return 1
  else if (a == b)  // SKIP (this is warning "duplicate-if-expression")
    return 2
  return 0
}

function elseAndOperand(a, b, c) {
  if ((a == b) && b == c)
    return 1
  else {
    if (a == b)  // SKIP (this is warning "duplicate-if-expression")
      return 2
  }
  return 0
}

function elseAndSuperset(a, b, c) {
  if (a == b)
    return 1
  else {
    if ((a == b) && b == c)  // WARN 3
      return 2
  }
  return 0
}

function compoundRepeatedOperand(a, b, w, e) {
  if (a == b)
    return 1
  else {
    if ((w == e) && a == b)  // WARN 4
      return 2
  }
  return 0
}

function repeatedCall(fn, w, e) {
  if (fn()) {
    if ((w == e) && fn())  // WARN 5
      return 1
  }
  return 0
}

function repeatedConjunctionCalls(previewPreset, mintEditState) {
  if (previewPreset.get() && !mintEditState.get()) {
    if (previewPreset.get() && !mintEditState.get())  // WARN 6
      return 1
  }
  return 0
}

function nestedInFunction(a, b) {
  if (a == b) {
    function inner() {
      if (a == b)   // SKIP
        return 1
      return 0
    }
    return inner()
  }
  return 0
}

function nestedInWhile(a, b) {
  if (a == b) {
    while (a) {
      if (a == b)  // SKIP
        return 1
    }
  }
  return 0
}

function afterStatement(a, b) {
  if (a == b) {
    a = b
    if (a == b)  // SKIP
      return 1
  }
  return 0
}

function afterCallDeclaration(a, b, fn) {
  if (a == b) {
    local value = fn()
    if (a == b)  // SKIP (function call can modify a or b)
      return value
  }
  return 0
}

function afterSimpleDeclarations(a, b) {
  if (a == b) {
    local value = 1
    local other = value + 1
    if (a == b)  // WARN 7
      return other
  }
  return 0
}

return [
  nestedSameCondition,
  elseSameCondition,
  elseIfSameCondition,
  elseAndOperand,
  elseAndSuperset,
  compoundRepeatedOperand,
  repeatedCall,
  repeatedConjunctionCalls,
  nestedInFunction,
  nestedInWhile,
  afterStatement,
  afterCallDeclaration,
  afterSimpleDeclarations
]
