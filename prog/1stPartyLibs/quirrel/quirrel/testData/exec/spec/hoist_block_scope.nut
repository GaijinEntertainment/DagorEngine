// Regression: closure hoisting (-optCH, always on in tests) must respect BLOCK scope.
// A name declared in a nested block / loop-init / loop-var is block-scoped; it must
// not leak so that a later reference to the same name (which binds to an outer
// captured local) is mistaken for a function-local. Leaking it made capture analysis
// hoist the leaf closure past the scope owning the outer 'cap', producing "Unknown
// variable". See doc/closure_hoisting_block_scope_bug.md.
// Each leaf is nested 2 deep (so it is a hoisting candidate) and returns the OUTER
// 'cap' after a block-scoped 'cap' has gone out of scope.

function blockCase() {
  let cap = "CAPTURED"
  return function m() { return function leaf() {
    { let cap = "x" }
    return cap
  }}
}
function ifCase() {
  let cap = "CAPTURED"
  return function m() { return function leaf() {
    if (true) { let cap = "x" }
    return cap
  }}
}
function whileCase() {
  let cap = "CAPTURED"
  return function m() { return function leaf() {
    local n = 0
    while (n < 1) { let cap = "x"; n += 1 }
    return cap
  }}
}
function foreachCase() {
  let cap = "CAPTURED"
  return function m() { return function leaf() {
    foreach (cap in [1, 2]) { }
    return cap
  }}
}
function forInitCase() {
  let cap = "CAPTURED"
  return function m() { return function leaf() {
    for (local cap = 0; cap < 2; cap += 1) { }
    return cap
  }}
}

println(blockCase()()())
println(ifCase()()())
println(whileCase()()())
println(foreachCase()()())
println(forInitCase()()())
