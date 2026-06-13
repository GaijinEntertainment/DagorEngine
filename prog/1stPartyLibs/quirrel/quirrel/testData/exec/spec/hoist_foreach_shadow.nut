// Regression: a foreach loop variable is the one block-scoped binding Quirrel lets
// shadow a same-scope local of the same name. Closure hoisting (-optCH) must keep a
// closure capturing that loop var INSIDE the loop, binding to the iteration value --
// not hoist it out where the name rebinds to the shadowed outer local. The bug bound
// the captured 'e' to the outer "BASE" (silent wrong value), producing "BASE-BASE".
// See doc/closure_hoisting_block_scope_bug.md and closureHoisting.cpp enterScopeNames.

// value var shadowing a same-scope local
function valShadow() {
  let e = "BASE"
  let res = []
  foreach (e in ["a", "b"]) {
    function w() { function deep() { return e } return deep() }
    res.append(w())
  }
  return "-".join(res)
}

// index + value vars both shadowing same-scope locals
function idxValShadow() {
  let k = "BK"
  let v = "BV"
  let res = []
  foreach (k, v in ["a", "b"]) {
    function w() { function deep() { return k + ":" + v } return deep() }
    res.append(w())
  }
  return "-".join(res)
}

// foreach var shadowing a function PARAMETER (params are direct locals)
function paramShadow(e) {
  return function mid() { return function leaf() {
    let seen = []
    foreach (e in ["x", "y"]) {
      function w() { function deep() { return e } return deep() }
      seen.append(w())
    }
    return "-".join(seen) + "|" + e
  }}
}

println(valShadow())
println(idxValShadow())
println(paramShadow("PARAM")()())
