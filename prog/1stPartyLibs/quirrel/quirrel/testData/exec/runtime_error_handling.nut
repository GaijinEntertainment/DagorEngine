// Coverage: runtime error / exception branches in sqvm.cpp
// (SQ_THROW paths, Raise_Error, type-mismatch in ARITH_OP/CMP_OP, bad index,
//  calling non-callable, PUSHTRAP/POPTRAP/THROW, error propagation).

function caught(fn) {
  try { fn(); return "ok" }
  catch (e) { return "caught" }
}

// arithmetic / type errors (use variables to defer past constant folding)
local zero = 0
local flt = 1.5
local str = "text"
local tbl = {}
println(caught(function() { return 1 / zero }))         // division by zero
println(caught(function() { return 1 % zero }))         // modulo by zero
println(caught(function() { return tbl + 1 }))          // arithmetic on table
//println(caught(function() { return str - 1 }))        // FIXME: str-1 interpreted as str+(-1)
println(caught(function() { return str - 1.5 }))        // subtraction on string
println(caught(function() { return -str }))             // unary minus on non-number
println(caught(function() { return ~flt }))             // bitwise-not on float

// comparison / index errors
println(caught(function() { local a = [1, 2]; return a[10] }))   // index out of range
println(caught(function() { local a = [1, 2]; a[10] = 0 }))      // set out of range
println(caught(function() { local n = null; return n.foo }))     // index into null
println(caught(function() { local t = {}; return t.nope }))      // missing table key

// call errors
println(caught(function() { return (5)() }))            // call a non-callable integer
println(caught(function() { local f = null; return f() }))       // call null

// explicit throw of various payloads
println(caught(function() { throw "string error" }))
println(caught(function() { throw 42 }))
println(caught(function() { throw { code = 7 } }))

// nested try/catch + rethrow (PUSHTRAP/POPTRAP nesting, THROW propagation)
function nested() {
  try {
    try { throw "inner" }
    catch (e) { throw "rethrown:" + e }
  }
  catch (e) { return e }
}
println(nested())                          // rethrown:inner

// cleanup via catch, then continue normally
local log = []
function withCleanup(shouldThrow) {
  try {
    if (shouldThrow) throw "boom"
    log.append("body-ok")
  }
  catch (e) { log.append("body-fail") }
  log.append("after")
}
withCleanup(false)
withCleanup(true)
println("|".join(log))                     // body-ok|after|body-fail|after

// assert failure is catchable
println(caught(function() { assert(false, "nope") }))   // caught
println(caught(function() { assert(1 == 1) }))          // ok
