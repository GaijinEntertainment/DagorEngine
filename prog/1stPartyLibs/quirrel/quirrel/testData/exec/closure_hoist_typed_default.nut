// Closure hoisting must not move closures with non-literal typed default
// parameters out of try-catch blocks when the default value type is unknown
// at compile time. The _OP_CLOSURE instruction checks default value types
// at runtime and can throw.

// Non-literal default with type mismatch inside try-catch in a nested function.
// The closure must not be hoisted because getExprLiteralTypeMask returns ~0u.
let wrongVal = vargv.len() < 10000 ? "wrong" : vargv[0] // must have unknown type during compilation
let test1 = function() {
  local result = 0
  try {
    let fn = function(x: int = wrongVal) { return x }
  } catch(e) {
    result = 1
  }
  return result
}
assert(test1() == 1)

// Valid typed literal defaults should still allow hoisting and work normally
let test2 = function() {
  let fn = function(x: int = 42) { return x }
  return fn()
}
assert(test2() == 42)

println("OK")