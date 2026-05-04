let { regexp } = require("string")

// Bug 1: Empty pattern should error, not crash (heap-buffer-overflow)
try {
  regexp("")
  println("BUG: empty pattern did not error")
} catch(e) {
  println($"empty pattern error: {e}")
}

// Bug 2: [A-] should error "unfinished range" (dead range-end check)
try {
  regexp("[A-]")
  println("BUG: [A-] did not error")
} catch(e) {
  println($"unfinished range error: {e}")
}

// Bug 3: Large quantifier should error, not silently truncate
try {
  regexp(@"^a{70000}$")
  println("BUG: large quantifier did not error")
} catch(e) {
  println($"quantifier error: {e}")
}

// Bug 4: Word boundary correctness at various positions
let wb = regexp(@"\bword\b")
println(wb.match("word"))
println(wb.search("hello word bye") != null)
println(wb.match("wordy"))

// Bug 5: Trailing backslash in escape should error
try {
  regexp("\\")
  println("BUG: trailing backslash did not error")
} catch(e) {
  println($"trailing backslash error: {e}")
}

// Bug 6: \m with incomplete args should error
try {
  regexp("\\m")
  println("BUG: incomplete \\m did not error")
} catch(e) {
  println($"incomplete \\m error: {e}")
}

try {
  regexp("\\m(")
  println("BUG: incomplete \\m( did not error")
} catch(e) {
  println($"incomplete \\m( error: {e}")
}

// Bug 7: Unclosed [ should error
try {
  regexp("[abc")
  println("BUG: unclosed [ did not error")
} catch(e) {
  println($"unclosed bracket error: {e}")
}

// Bug 8: Search finds matches at non-zero positions
let sr = regexp(@"ab")
let res = sr.search("xab")
println($"search begin={res.begin} end={res.end}")

// Bug 9: Character range matching correctness
let rng = regexp("[A-Z]")
println(rng.match("M"))
println(rng.match("a"))

// Bug 10: Range validation with escape sequences
try {
  let r = regexp("[A-\\n]")
  // Should not reach here - A(65) > \n(10) is an invalid range
  println("BUG: [A-\\n] should be invalid range")
} catch(e) {
  println($"[A-\\n] correctly rejected: {e}")
}

let r_az = regexp("[a-\\z]")
println($"[a-\\z] match m: {r_az.match("m")}")

// Bug 11: Pattern complexity limit
try {
  local longpat = "aaaaaaaaaa" // 10
  for (local i = 0; i < 12; i++)
    longpat += longpat // 10 * 2^12 = 40960
  longpat += longpat.slice(0, 9040) // 50000
  regexp(longpat)
  println("BUG: 50000 char pattern should error")
} catch(e) {
  println($"long pattern error: {e}")
}

// Bug 12: Nested zero-width quantifiers terminate
let zw = regexp(@"\b{0,3}word")
println($"zero-width bounded: {zw.match("word")}")

// Bug 14: {n,m} where n > m should error at compile time
try {
  regexp("a{5,2}")
  println("BUG: a{5,2} should be invalid range")
} catch(e) {
  println($"quantifier min>max error: {e}")
}

// Bug 15: Large quantifier packing correctness (UB fix for p0 >= 32768)
let lq = regexp(@"^a{40000}$")
local s40k = ""
for (local i = 0; i < 15; i++)
  s40k += "aaaaaaaaaa" // build in chunks of 10
// s40k = 150 chars, won't match {40000} but should not crash/UB
println($"large quantifier no crash: {lq.match(s40k) == false}")
