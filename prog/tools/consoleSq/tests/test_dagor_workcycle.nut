// Squirrel module under test: dagor.workcycle (registered by dagorWorkCycle.cpp)
// Tests defer/timer registration and clearance API. Note: in csq-dev the
// idle work cycle is not pumped, so we exercise registration/clearance/has APIs
// rather than waiting for callbacks to fire.

from "dagor.workcycle" import defer, deferOnce, setTimeout, resetTimeout, setInterval, clearTimer, hasTimer


// ============================================================
// Section 1: setTimeout returns id and hasTimer reports it
// ============================================================
println("--- Section 1: setTimeout + hasTimer + clearTimer ---")

let id1 = setTimeout(60.0, function() {})
assert(id1 != null, "setTimeout returns id")
assert(hasTimer(id1) == true, "hasTimer true after setTimeout")
clearTimer(id1)
assert(hasTimer(id1) == false, "hasTimer false after clearTimer")

println("Section 1 PASSED")


// ============================================================
// Section 2: setInterval registers and is clearable
// ============================================================
println("--- Section 2: setInterval ---")

let id2 = setInterval(60.0, function() {})
assert(hasTimer(id2) == true, "hasTimer true after setInterval")
clearTimer(id2)
assert(hasTimer(id2) == false, "hasTimer false after clearTimer interval")

println("Section 2 PASSED")


// ============================================================
// Section 3: explicit id parameter
// ============================================================
println("--- Section 3: explicit id parameter ---")

let myId = "my_explicit_id"
setTimeout(60.0, function() {}, myId)
assert(hasTimer(myId) == true, "hasTimer with explicit id")
clearTimer(myId)
assert(hasTimer(myId) == false, "cleared explicit id")

println("Section 3 PASSED")


// ============================================================
// Section 4: resetTimeout reuses an existing id
// ============================================================
println("--- Section 4: resetTimeout reuses existing id ---")

let rid = "reset_id"
resetTimeout(60.0, function() {}, rid)
assert(hasTimer(rid) == true, "first resetTimeout registers")
resetTimeout(60.0, function() {}, rid)
assert(hasTimer(rid) == true, "second resetTimeout still registered (replaced)")
clearTimer(rid)

println("Section 4 PASSED")


// ============================================================
// Section 5: defer + deferOnce accept zero-arg closures
// ============================================================
println("--- Section 5: defer + deferOnce ---")

defer(function() {})
deferOnce(function() {})

println("Section 5 PASSED")


// ============================================================
// Section 6: incorrect input
// ============================================================
println("--- Section 6: incorrect input ---")

local threw = false
try {
  setTimeout(60.0, function() {}, "dup_id")
  setTimeout(60.0, function() {}, "dup_id")
} catch (e) {
  threw = true
}
assert(threw, "duplicate setTimeout id must throw")
clearTimer("dup_id")

threw = false
try {
  defer(function(_arg1, _arg2) {})
} catch (e) {
  threw = true
}
assert(threw, "defer with required args must throw")

threw = false
try {
  setTimeout(60.0, function(_) {})
} catch (e) {
  threw = true
}
// setTimeout's closure may take args; throwing is not guaranteed. Don't assert.

threw = false
try {
  setTimeout("not number", function() {})
} catch (e) {
  threw = true
}
assert(threw, "setTimeout with non-number first arg must throw")

println("Section 6 PASSED")


println("ALL TESTS PASSED")
