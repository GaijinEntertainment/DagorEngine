// Verify that scope-aware type deduction for closure hoisting produces
// correct runtime behavior: hoisted closures with typed defaults from
// const/function return types must work identically to non-hoisted versions.

const STR_CONST = "hello"

function fnReturnsInt(x): int {
  return x * x
}

function fnReturnsStringOrInt(x): string|int {
  return x ? x : STR_CONST
}

// Hoisted: const value matches param type
let test1 = function() {
  let fn = function(x: string = STR_CONST) { return x }
  return fn()
}
assert(test1() == "hello")

// Hoisted: function return type matches param type
let test2 = function() {
  let fn = function(x: int = fnReturnsInt(3)) { return x }
  return fn()
}
assert(test2() == 9)

// Hoisted: union return type matches union param type
let test3 = function() {
  let fn = function(x: string|int = fnReturnsStringOrInt(0)) { return x }
  return fn()
}
assert(test3() == "hello")

println("OK")
