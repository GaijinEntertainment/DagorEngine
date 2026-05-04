// Squirrel module under test: regexp2 (registered by bindSqrat.cpp register_reg_exp)

import "regexp2" as regexp2


// ============================================================
// Section 1: ctor + match
// ============================================================
println("--- Section 1: ctor + match ---")

let re = regexp2("\\d+")
assert(re.match("abc 123 def") == true, "match digits in mixed string")
assert(re.match("no digits here") == false, "no digits returns false")
assert(re.pattern() == "\\d+", $"pattern() returns ctor arg: {re.pattern()}")

println("Section 1 PASSED")


// ============================================================
// Section 2: fullmatch
// ============================================================
println("--- Section 2: fullmatch ---")

let reAll = regexp2("\\d+")
assert(reAll.fullmatch("12345") == true, "fullmatch all digits")
assert(reAll.fullmatch("12345abc") == false, "fullmatch fails on partial")
assert(reAll.fullmatch("") == false, "fullmatch empty string")

println("Section 2 PASSED")


// ============================================================
// Section 3: replace
// ============================================================
println("--- Section 3: replace ---")

let reSpace = regexp2("\\s+")
let collapsed = reSpace.replace("_", "hello   world\t  foo")
assert(collapsed == "hello_world_foo", $"replace collapsed: {collapsed}")

let reAB = regexp2("ab")
let r = reAB.replace("X", "ababab")
assert(r == "XXX", $"replace all ab: {r}")

println("Section 3 PASSED")


// ============================================================
// Section 4: multiExtract
// ============================================================
println("--- Section 4: multiExtract ---")

let reNum = regexp2("(\\d+)")
let nums = reNum.multiExtract("\\1", "a1 b22 c333")
assert(typeof nums == "array", "multiExtract returns array")
assert(nums.len() == 3, $"multiExtract count: {nums.len()}")
assert(nums[0] == "1" && nums[1] == "22" && nums[2] == "333", "multiExtract values")

println("Section 4 PASSED")


// ============================================================
// Section 5: incorrect input
// ============================================================
println("--- Section 5: incorrect input ---")

// PCRE construction does not throw on invalid pattern; instead the regex never matches.
// We assert that documented safe behavior (no spurious matches) holds.
let bad = regexp2("[unclosed")
assert(bad.match("anything") == false, "invalid pattern matches nothing")
assert(bad.fullmatch("anything") == false, "invalid pattern fullmatch returns false")

local threw = false
try {
  println(re.match(123))
} catch (e) {
  threw = true
}
assert(threw, "match with non-string must throw")

threw = false
try {
  re.replace(123, "abc")
} catch (e) {
  threw = true
}
assert(threw, "replace with non-string replacement must throw")

println("Section 5 PASSED")


println("ALL TESTS PASSED")
