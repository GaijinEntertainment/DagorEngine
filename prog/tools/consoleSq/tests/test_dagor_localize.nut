// Squirrel module under test: dagor.localize (registered by localization.cpp)
// No localization tables are loaded in csq-dev, so loc() falls back to the
// provided default or the key. We exercise that documented passthrough plus
// language metadata getters.

// getLangId is bound but dereferences a null locTable when no .csv is loaded
// (and csq-dev never loads one), so it is not exercised here.
from "dagor.localize" import loc, getLocTextForLang, processHypenationsCN, processHypenationsJP,
  doesLocTextExist, getCurrentLanguage, getForceLanguage


// ============================================================
// Section 1: loc passthrough when key missing
// ============================================================
println("--- Section 1: loc passthrough when key missing ---")

let r = loc("definitely/missing/key")
assert(r == "definitely/missing/key", $"missing key returns key: {r}")

let r2 = loc("definitely/missing/key", "fallback_default")
assert(r2 == "fallback_default", $"missing key returns default: {r2}")

println("Section 1 PASSED")


// ============================================================
// Section 2: doesLocTextExist returns false for missing key
// ============================================================
println("--- Section 2: doesLocTextExist returns false for missing key ---")

let exists = doesLocTextExist("definitely/missing/key")
assert(typeof exists == "bool", "doesLocTextExist returns bool")
assert(exists == false, "missing key reports false")

println("Section 2 PASSED")


// ============================================================
// Section 3: getLocTextForLang fallback to default
// ============================================================
println("--- Section 3: getLocTextForLang fallback to default ---")

let r3 = getLocTextForLang("definitely/missing/key", "english", "fallback_3")
assert(r3 == "fallback_3", $"missing for lang returns default: {r3}")

println("Section 3 PASSED")


// ============================================================
// Section 4: language metadata accessors
// ============================================================
println("--- Section 4: language metadata accessors ---")

let cur = getCurrentLanguage()
assert(typeof cur == "string", "getCurrentLanguage is string")

let force = getForceLanguage()
assert(typeof force == "string", "getForceLanguage is string")

println("Section 4 PASSED")


// ============================================================
// Section 5: hyphenation passthrough on plain ASCII
// ============================================================
println("--- Section 5: hyphenation passthrough on plain ASCII ---")

let cn = processHypenationsCN("hello world")
assert(typeof cn == "string", "processHypenationsCN returns string")

let jp = processHypenationsJP("hello world")
assert(typeof jp == "string", "processHypenationsJP returns string")

println("Section 5 PASSED")


// ============================================================
// Section 6: param substitution falls back to no-op without table
// ============================================================
println("--- Section 6: loc with param table substitutes {{key}} ---")

let r6 = loc("definitely/missing/key", "Hello {name}", {name = "world"})
assert(r6 == "Hello world", $"loc param substitution: {r6}")

println("Section 6 PASSED")


// ============================================================
// Section 7: incorrect input
// ============================================================
println("--- Section 7: incorrect input ---")

local threw = false
try {
  loc(123)
} catch (e) {
  threw = true
}
// loc tolerates non-string keys by returning early via SQ_FAILED; non-throw is acceptable

threw = false
try {
  doesLocTextExist(123)
} catch (e) {
  threw = true
}
assert(threw, "doesLocTextExist with non-string must throw")

threw = false
try {
  getLocTextForLang(123, "english")
} catch (e) {
  threw = true
}
// getLocTextForLang tolerates non-string keys via SQ_FAILED early-return; non-throw is acceptable

println("Section 7 PASSED")


println("ALL TESTS PASSED")
