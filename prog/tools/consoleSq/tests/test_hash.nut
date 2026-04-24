// Squirrel module under test: hash (registered by hash.cpp)

from "hash" import md5, sha1, sha256, sha3_256, crc32, crc32_int, create_guid


// ============================================================
// Section 1: known-vector digests for "abc"
// ============================================================
println("--- Section 1: known-vector digests for 'abc' ---")

let md5Abc = md5("abc")
let sha1Abc = sha1("abc")
let sha256Abc = sha256("abc")
let sha3Abc = sha3_256("abc")
assert(md5Abc == "900150983cd24fb0d6963f7d28e17f72", $"md5('abc'): {md5Abc}")
assert(sha1Abc == "a9993e364706816aba3e25717850c26c9cd0d89d", $"sha1('abc'): {sha1Abc}")
assert(sha256Abc == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
  $"sha256('abc'): {sha256Abc}")
assert(sha3Abc == "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532",
  $"sha3_256('abc'): {sha3Abc}")

println("Section 1 PASSED")


// ============================================================
// Section 2: empty input round-trips
// ============================================================
println("--- Section 2: empty input ---")

assert(md5("") == "d41d8cd98f00b204e9800998ecf8427e", "md5 empty")
assert(sha1("") == "da39a3ee5e6b4b0d3255bfef95601890afd80709", "sha1 empty")
assert(sha256("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
  "sha256 empty")

println("Section 2 PASSED")


// ============================================================
// Section 3: crc32 + crc32_int
// ============================================================
println("--- Section 3: crc32 + crc32_int ---")

let crc = crc32("abc")
assert(typeof crc == "string", "crc32 returns string")
assert(crc.len() == 8, $"crc32 length 8 hex: {crc}")

let crcInt = crc32_int("abc")
assert(typeof crcInt == "integer", "crc32_int returns integer")

println("Section 3 PASSED")


// ============================================================
// Section 4: create_guid produces unique values
// ============================================================
println("--- Section 4: create_guid uniqueness ---")

let g1 = create_guid()
let g2 = create_guid()
assert(typeof g1 == "string", "create_guid returns string")
assert(g1.len() > 30, $"guid length: {g1.len()}")
assert(g1 != g2, "two guids differ")

println("Section 4 PASSED")


// ============================================================
// Section 5: incorrect input
// ============================================================
println("--- Section 5: incorrect input ---")

local threw = false
try {
  md5(123)
} catch (e) {
  threw = true
}
assert(threw, "md5 with non-string must throw")

threw = false
try {
  sha256(null)
} catch (e) {
  threw = true
}
assert(threw, "sha256 with null must throw")

println("Section 5 PASSED")


println("ALL TESTS PASSED")
