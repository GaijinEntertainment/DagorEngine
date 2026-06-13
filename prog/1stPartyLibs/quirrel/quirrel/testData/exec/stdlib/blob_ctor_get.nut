// Coverage: blob constructor sizing, element get/set and clone in sqstdblob.cpp
// (_blob_constructor, _blob__get, _blob__set, _blob__cloned, _blob__nexti)
// including the error branches the existing blob test does not reach.

let { blob } = require("iostream")

// constructor with an explicit size (zero-filled)
let b = blob(4)
assert(b.len() == 4)
for (local i = 0; i < 4; i++) assert(b[i] == 0)

// element set / get
b[0] = 0x41
b[1] = 0x42
b[3] = 0xFF
assert(b[0] == 0x41)
assert(b[1] == 0x42)
assert(b[3] == 0xFF)

// iterate via foreach (__nexti)
local sum = 0
foreach (i, byte in b) sum += byte
assert(sum == 0x41 + 0x42 + 0xFF)
println($"sum: {sum}")

// clone produces an independent copy (__cloned)
let c = clone b
c[0] = 0x99
assert(b[0] == 0x41, "original must be unaffected by clone mutation")
assert(c[0] == 0x99)
println($"clone ok: {b[0]} {c[0]}")

// zero-size constructor
let z = blob(0)
assert(z.len() == 0)

function expectErr(label, fn) {
  try { fn(); println($"FAIL: {label}") } catch (e) { println($"err: {label}") }
}

// constructor error: negative size
expectErr("negative-size", function() { blob(-1) })
// element get/set out of range
expectErr("get-oor",  function() { return b[100] })
expectErr("set-oor",  function() { b[100] = 1 })

println("blob ctor/get: OK")
