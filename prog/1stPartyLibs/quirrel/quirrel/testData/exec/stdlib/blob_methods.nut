let { blob, casti2f, castf2i, swap2, swap4, swapfloat } = require("iostream")

let b = blob(16)
assert(b.len() == 16)
assert(typeof b == "blob")

b.resize(4)
assert(b.len() == 4)

b[0] = 0xAA
b[1] = 0xBB
b[2] = 0xCC
b[3] = 0xDD
assert(b[0] == 0xAA)
assert(b[1] == 0xBB)
assert(b[2] == 0xCC)
assert(b[3] == 0xDD)

let s = blob(4)
s[0] = 0x12
s[1] = 0x34
s[2] = 0x56
s[3] = 0x78
s.swap2()
assert(s[0] == 0x34 && s[1] == 0x12 && s[2] == 0x78 && s[3] == 0x56)

let q = blob(4)
q[0] = 0x11; q[1] = 0x22; q[2] = 0x33; q[3] = 0x44
q.swap4()
assert(q[0] == 0x44 && q[1] == 0x33 && q[2] == 0x22 && q[3] == 0x11)

let t = blob(5)
t[0] = 104
t[1] = 101
t[2] = 108
t[3] = 108
t[4] = 111
assert(t.as_string() == "hello")

let bi = blob(3)
bi[0] = 10; bi[1] = 20; bi[2] = 30
local sum = 0
foreach (_, v in bi) {
  sum += v
}
assert(sum == 60)

let orig = blob(3)
orig[0] = 1; orig[1] = 2; orig[2] = 3
let cp = clone orig
assert(cp.len() == 3)
assert(cp[0] == 1 && cp[1] == 2 && cp[2] == 3)
cp[0] = 99
assert(orig[0] == 1)

let i = 0x3f800000
let f = casti2f(i)
assert(typeof f == "float")
let ii = castf2i(f)
assert(ii == i)

assert(swap2(0x1234) == 0x3412)
assert(swap4(0x11223344) == 0x44332211)
let sf = swapfloat(1.0)
assert(typeof sf == "float")

function mustThrow(label, fn) {
  try { fn(); println($"FAIL: {label} accepted null") } catch (_) {}
}

let zz = blob(4)
mustThrow("blob.ctor",    @() blob(null))
mustThrow("resize",       @() zz.resize(null))
mustThrow("_set.1",       @() zz._set(null, 0))
mustThrow("_set.2",       @() zz._set(0, null))
mustThrow("_get",         @() zz._get(null))
mustThrow("_cloned",      @() zz._cloned(null))
mustThrow("casti2f",      @() casti2f(null))
mustThrow("castf2i",      @() castf2i(null))
mustThrow("g.swap2",      @() swap2(null))
mustThrow("g.swap4",      @() swap4(null))
mustThrow("g.swapfloat",  @() swapfloat(null))

println("ok")
