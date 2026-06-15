// Coverage: scalar encoding/decoding branches in sqstdserialization.cpp -
// integers of every size class (1/2/4/8 byte, signed), float stored as 4-byte
// vs 8-byte, bool, null, and short vs long strings (length-prefix branches).

let { blob } = require("iostream")

function roundtrip(obj) {
  let b = blob()
  b.writeobject(obj)
  b.seek(0)
  return b.readobject({})
}

// integers spanning all size encodings, positive and negative
let ints = [
  0, 1, 127, 128, 255, 256,            // 1-byte and 2-byte boundary
  65535, 65536, 1000000,               // 2-byte / 4-byte boundary
  2147483647, 2147483648, 5000000000,  // 4-byte / 8-byte boundary
  -1, -128, -200, -70000, -5000000000  // negatives across sizes
]
foreach (i in ints)
  assert(roundtrip(i) == i, $"int {i} mismatch -> {roundtrip(i)}")
println($"ints ok: {ints.len()}")

// floats: 3.5 round-trips exactly through 4-byte; 0.1 / pi need 8-byte
foreach (f in [0.0, 3.5, -2.25, 0.1, 3.141592653589793, 1.0e30])
  assert(roundtrip(f) == f, $"float {f} mismatch")
println("floats ok")

// bool + null
assert(roundtrip(true) == true)
assert(roundtrip(false) == false)
assert(roundtrip(null) == null)
println("bool/null ok")

// strings: empty, short, and long (>255 chars exercises the long length branch)
local longStr = ""
for (local i = 0; i < 400; i++) longStr += "x"
assert(roundtrip("") == "")
assert(roundtrip("hi") == "hi")
assert(roundtrip(longStr).len() == 400)
println($"strings ok, long len {roundtrip(longStr).len()}")

// nested containers mixing every scalar type
let nested = {
  a = [0, 127, 65536, 5000000000, -7],
  f = [3.5, 0.1],
  b = [true, false, null],
  s = ["", "deep"],
  inner = { x = 256, y = [1, [2, [3]]] }
}
let rt = roundtrip(nested)
assert(rt.a[3] == 5000000000)
assert(rt.f[1] == 0.1)
assert(rt.inner.y[1][1][0] == 3)
println("nested ok")

println("serialization scalars: OK")
