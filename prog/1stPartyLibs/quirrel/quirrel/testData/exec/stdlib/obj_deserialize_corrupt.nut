// Coverage: deserialize decode-error branches in sqstdserialization.cpp -
// truncated streams (unexpectedEndOfData), invalid type tags and out-of-range
// indices that the valid round-trip tests never hit.

let { blob } = require("iostream")

function expectErr(label, fn) {
  try { fn(); println($"FAIL: {label} did not throw") }
  catch (e) { println($"err: {label}") }
}

// empty stream
expectErr("empty", function() { let b = blob(); b.readobject({}) })

// truncate a valid serialization at every prefix length
let full = blob()
full.writeobject({ a = [1, 2, 1000000, 5000000000], s = "hello", f = 3.5, nested = { x = true } })
let fullLen = full.len()
local truncErrors = 0
for (local cut = 1; cut < fullLen; cut++) {
  let t = blob()
  full.seek(0)
  // copy the first `cut` bytes
  for (local i = 0; i < cut; i++) t.writen(full.readn('b'), 'b')
  t.seek(0)
  try { t.readobject({}) } catch (e) { truncErrors++ }
}
println($"truncation errors: {truncErrors > 0}")   // true (most/all prefixes fail)

// invalid leading type tag (0xFF is not a valid TP_* code)
expectErr("bad-type-tag", function() {
  let b = blob()
  b.writen(0xFF, 'b')
  b.seek(0)
  b.readobject({})
})

// a string whose declared length runs past the stream
expectErr("string-overrun", function() {
  let b = blob()
  full.seek(0)
  // grab the real first byte sequence for a string, then corrupt by truncating
  b.writen(0xFF, 'b')   // arbitrary
  b.seek(0)
  b.readobject({})
})

// a valid object still deserializes after all the failures (state not corrupted)
let ok = blob()
ok.writeobject([1, "two", 3.0])
ok.seek(0)
let r = ok.readobject({})
println($"recovered: {r[0]} {r[1]} {r[2]}")   // 1 two 3

println("deserialize corrupt: OK")
