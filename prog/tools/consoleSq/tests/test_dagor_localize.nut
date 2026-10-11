// Quirrel module under test: dagor.localize (registered by localization.cpp)
// No localization tables are loaded in csq-dev, so loc() falls back to the
// provided default or the key, with the params appended to the key as
// "key: name=value, ...". We exercise that documented passthrough plus
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

// no plural rule is loaded, so the last form is picked; for 5 that matches english as well
let r6plural = loc("definitely/missing/key", "{count} {count=item/items}", {count = 5})
assert(r6plural == "5 items", $"loc plural form: {r6plural}")

let r6multi = loc("definitely/missing/key", "{a}{a}{b} {a} {{a}} {ab} {c}", {a = "x", b = ""})
assert(r6multi == "xx x {0}x{1} {0}ab{1} {0}c{1}".subst("{", "}"),
  $"loc replaces every token, keeps unknown ones: {r6multi}")

let r6grow = loc("definitely/missing/key", "{a}-{a}", {a = "longer than the token"})
assert(r6grow == "longer than the token-longer than the token", $"loc value longer than the token: {r6grow}")

let r6plural2 = loc("definitely/missing/key", "{n=one} {n=a/b/c}! {n=broken", {n = 1})
assert(r6plural2 == "one c! {n=broken", $"loc plural forms, unterminated token kept: {r6plural2}")

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

threw = false
try {
  loc("definitely/missing/key", 123)
} catch (e) {
  threw = true
}
assert(threw, "loc with a non-string, non-table argument must throw")

println("Section 7 PASSED")


// ============================================================
// Section 8: missing key with params returns the key and the values
// ============================================================
println("--- Section 8: missing key with params returns the key and the values ---")

let r8 = loc("definitely/missing/key", {name = "world", count = 3})
assert(r8 == "definitely/missing/key: count=3, name=world", $"missing key keeps params, sorted by name: {r8}")

let r8lang = getLocTextForLang("definitely/missing/key", "english", {count = 3})
assert(r8lang == "definitely/missing/key: count=3", $"missing key for lang keeps params: {r8lang}")

let r8empty = loc("", {count = 3})
assert(r8empty == "", $"empty key stays empty: {r8empty}")

println("Section 8 PASSED")


println("ALL TESTS PASSED")
