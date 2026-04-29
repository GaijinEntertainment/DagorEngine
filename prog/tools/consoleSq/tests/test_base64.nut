// Squirrel module under test: base64 (registered by base64.cpp)

from "base64" import encodeString, decodeString, encodeUrlString, decodeUrlString, encodeJson, encodeBlob, decodeBlob
from "iostream" import blob


// ============================================================
// Section 1: encodeString / decodeString round-trip
// ============================================================
println("--- Section 1: encodeString / decodeString round-trip ---")

let plain = "Hello, World!"
let encoded = encodeString(plain)
assert(encoded == "SGVsbG8sIFdvcmxkIQ==", $"encodeString mismatch: {encoded}")
let decoded = decodeString(encoded)
assert(decoded == plain, $"decodeString round-trip mismatch: {decoded}")

let plain2 = "abc"
let enc2 = encodeString(plain2)
assert(enc2 == "YWJj", $"encodeString abc: {enc2}")
assert(decodeString(enc2) == plain2, "decodeString abc round-trip")

let empty = ""
let encEmpty = encodeString(empty)
assert(decodeString(encEmpty) == empty, "empty round-trip")

println("Section 1 PASSED")


// ============================================================
// Section 2: encodeUrlString / decodeUrlString round-trip
// ============================================================
println("--- Section 2: encodeUrlString / decodeUrlString round-trip ---")

let urlPlain = "subjects?abc/def+xyz"
let urlEnc = encodeUrlString(urlPlain)
assert(urlEnc.indexof("+") == null, $"url-safe must not contain '+': {urlEnc}")
assert(urlEnc.indexof("/") == null, $"url-safe must not contain '/': {urlEnc}")
let urlDec = decodeUrlString(urlEnc)
assert(urlDec == urlPlain, $"url round-trip mismatch: {urlDec}")

println("Section 2 PASSED")


// ============================================================
// Section 3: encodeJson with various Squirrel objects
// ============================================================
println("--- Section 3: encodeJson with table and array ---")

let jsonEnc = encodeJson({a = 1, b = "x"})
let jsonDec = decodeString(jsonEnc)
assert(jsonDec.indexof("\"a\"") != null, $"encodeJson table missing key 'a': {jsonDec}")
assert(jsonDec.indexof("\"b\"") != null, $"encodeJson table missing key 'b': {jsonDec}")

let arrEnc = encodeJson([1, 2, 3])
let arrDec = decodeString(arrEnc)
assert(arrDec == "[1,2,3]", $"encodeJson array mismatch: {arrDec}")

println("Section 3 PASSED")


// ============================================================
// Section 4: encodeBlob / decodeBlob round-trip
// ============================================================
println("--- Section 4: encodeBlob / decodeBlob round-trip ---")

let b = blob(4)
b[0] = 0x41
b[1] = 0x42
b[2] = 0x43
b[3] = 0x44
let blobEnc = encodeBlob(b)
assert(blobEnc == "QUJDRA==", $"encodeBlob mismatch: {blobEnc}")
let blobDec = decodeBlob(blobEnc)
assert(blobDec.len() == 4, $"decodeBlob length: {blobDec.len()}")
assert(blobDec[0] == 0x41 && blobDec[1] == 0x42 && blobDec[2] == 0x43 && blobDec[3] == 0x44, "decodeBlob bytes")

println("Section 4 PASSED")


// ============================================================
// Section 5: incorrect input
// ============================================================
println("--- Section 5: incorrect input ---")

local threw = false
try {
  encodeBlob("not a blob")
} catch (e) {
  threw = true
}
assert(threw, "encodeBlob with non-blob argument must throw")

threw = false
try {
  decodeBlob(123)
} catch (e) {
  threw = true
}
assert(threw, "decodeBlob with non-string argument must throw")

threw = false
try {
  encodeString(null)
} catch (e) {
  threw = true
}
assert(threw, "encodeString with null must throw")

println("Section 5 PASSED")


println("ALL TESTS PASSED")
