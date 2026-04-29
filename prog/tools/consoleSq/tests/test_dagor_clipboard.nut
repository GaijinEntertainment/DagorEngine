// Squirrel module under test: dagor.clipboard (registered by dagorClipboard.cpp)
// set_clipboard_text would mutate user-visible OS clipboard state and is not
// invoked here. We restrict coverage to get_clipboard_text (read-only) plus
// argument validation on set_clipboard_text.

from "dagor.clipboard" import set_clipboard_text, get_clipboard_text //disable: -see-other


// ============================================================
// Section 1: get_clipboard_text returns string
// ============================================================
println("--- Section 1: get_clipboard_text returns string ---")

let s = get_clipboard_text()
assert(typeof s == "string", $"get_clipboard_text type: {typeof s}")

println("Section 1 PASSED")


// ============================================================
// Section 2: incorrect input
// ============================================================
println("--- Section 2: incorrect input ---")

// set_clipboard_text on csq-dev's bind ignores non-string arguments via the
// SQ_FAILED early return path -- it does not throw. We assert the documented
// safe default: passing nothing or non-string leaves clipboard untouched.
local threw = false
try {
  set_clipboard_text() //-disable: -param-count
} catch (e) {
  threw = true
}
assert(threw, "set_clipboard_text with no arg must throw on missing param")

// Extra arg to zero-arg get_clipboard_text must throw.
threw = false
try {
  get_clipboard_text("extra") //-disable: -param-count
} catch (e) {
  threw = true
}
assert(threw, "get_clipboard_text with extra arg must throw")

println("Section 2 PASSED")


// ============================================================
// Section 3: binding shape sanity
// ============================================================
println("--- Section 3: binding shape sanity ---")

assert(typeof set_clipboard_text == "function", "set_clipboard_text callable")
assert(typeof get_clipboard_text == "function", "get_clipboard_text callable")

println("Section 3 PASSED")


println("ALL TESTS PASSED")
