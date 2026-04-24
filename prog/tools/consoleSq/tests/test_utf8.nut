// Squirrel module under test: utf8 (registered by bindSqrat.cpp register_utf8)

import "utf8" as utf8


// ============================================================
// Section 1: ctor + str + charCount
// ============================================================
println("--- Section 1: ctor + str + charCount ---")

let asciiStr = utf8("hello")
assert(asciiStr.str() == "hello", "ascii str round-trip")
assert(asciiStr.charCount() == 5, $"ascii charCount: {asciiStr.charCount()}")
assert(asciiStr.tostring() == "hello", "tostring delegates to str")

// Two-byte UTF-8 chars
let twoByte = utf8("\xc3\xa9\xc3\xa8")
assert(twoByte.charCount() == 2, $"two two-byte chars: {twoByte.charCount()}")

println("Section 1 PASSED")


// ============================================================
// Section 2: toUpper / toLower
// ============================================================
println("--- Section 2: toUpper / toLower ---")

let mixed = utf8("Hello World")
assert(mixed.toUpper() == "HELLO WORLD", $"toUpper ascii: {mixed.toUpper()}")
let mixed2 = utf8("Hello World")
assert(mixed2.toLower() == "hello world", $"toLower ascii: {mixed2.toLower()}")

println("Section 2 PASSED")


// ============================================================
// Section 3: strtr
// ============================================================
println("--- Section 3: strtr ---")

let s = utf8("abc")
let translated = s.strtr("abc", "xyz")
assert(translated == "xyz", $"strtr ascii: {translated}")

println("Section 3 PASSED")


// ============================================================
// Section 4: slice + indexof
// ============================================================
println("--- Section 4: slice + indexof ---")

let abc = utf8("abcdef")
let sliced = abc.slice(1, 4)
assert(sliced == "bcd", $"slice 1..4: {sliced}")

let abc2 = utf8("abcdef")
let sliceTail = abc2.slice(2)
assert(sliceTail == "cdef", $"slice 2..end: {sliceTail}")

let needle = utf8("abcdef")
let pos = needle.indexof("cd")
assert(pos == 2, $"indexof 'cd' in 'abcdef': {pos}")

let miss = utf8("abcdef")
let posMiss = miss.indexof("xy")
assert(posMiss == null, $"indexof miss returns null: {posMiss}")

println("Section 4 PASSED")


// ============================================================
// Section 5: incorrect input
// ============================================================
println("--- Section 5: incorrect input ---")

local threw = false
try {
  utf8(123)
} catch (e) {
  threw = true
}
assert(threw, "ctor with non-string must throw")

threw = false
try {
  let u = utf8("abc")
  u.strtr(123, "xyz")
} catch (e) {
  threw = true
}
assert(threw, "strtr with non-string from must throw")

println("Section 5 PASSED")


println("ALL TESTS PASSED")
