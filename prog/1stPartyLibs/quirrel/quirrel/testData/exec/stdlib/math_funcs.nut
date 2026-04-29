let m = require("math")

assert(m.sqrt(4.0) == 2.0)
assert(m.sqrt(9.0) == 3.0)

assert(m.sin(0.0) == 0.0)
assert(m.cos(0.0) == 1.0)
assert(m.tan(0.0) == 0.0)

assert(m.asin(0.0) == 0.0)
assert(m.acos(1.0) == 0.0)

assert(m.log(1.0) == 0.0)
assert(m.log10(1.0) == 0.0)
assert(m.log10(100.0) == 2.0)

assert(m.atan(0.0) == 0.0)
assert(m.atan2(0.0, 1.0) == 0.0)

assert(m.pow(2.0, 3.0) == 8.0)
assert(m.pow(10.0, 2.0) == 100.0)

assert(m.floor(1.7) == 1.0)
assert(m.ceil(1.2) == 2.0)
assert(m.round(1.5) == 2.0)
assert(m.round(1.4) == 1.0)

assert(m.exp(0.0) == 1.0)

assert(m.fabs(-3.5) == 3.5)
assert(m.fabs(3.5) == 3.5)

assert(m.abs(-7) == 7)
assert(m.abs(7) == 7)
assert(m.abs(-3.5) == 3.5)

assert(m.asin(2.0) == m.asin(1.0))
assert(m.acos(-2.0) == m.acos(-1.0))

m.srand(12345)
let r1 = m.rand()
assert(typeof r1 == "integer")
assert(r1 >= 0)
assert(r1 <= m.RAND_MAX)

m.srand(12345)
let r1again = m.rand()
assert(r1 == r1again)

assert(m.PI > 3.14 && m.PI < 3.15)
assert(m.RAND_MAX > 0)
assert(m.FLT_MAX > 0.0)
assert(m.FLT_MIN > 0.0)

function mustThrow(label, fn) {
  try { fn(); println($"FAIL: {label} accepted null") } catch (_) {}
}

mustThrow("sqrt",   @() m.sqrt(null))
mustThrow("sin",    @() m.sin(null))
mustThrow("cos",    @() m.cos(null))
mustThrow("asin",   @() m.asin(null))
mustThrow("acos",   @() m.acos(null))
mustThrow("log",    @() m.log(null))
mustThrow("log10",  @() m.log10(null))
mustThrow("tan",    @() m.tan(null))
mustThrow("atan",   @() m.atan(null))
mustThrow("atan2.1",@() m.atan2(null, 1.0))
mustThrow("atan2.2",@() m.atan2(1.0, null))
mustThrow("pow.1",  @() m.pow(null, 2.0))
mustThrow("pow.2",  @() m.pow(2.0, null))
mustThrow("floor",  @() m.floor(null))
mustThrow("ceil",   @() m.ceil(null))
mustThrow("round",  @() m.round(null))
mustThrow("exp",    @() m.exp(null))
mustThrow("srand",  @() m.srand(null))
mustThrow("fabs",   @() m.fabs(null))
mustThrow("abs",    @() m.abs(null))
mustThrow("min.1",  @() m.min(null, 1))
mustThrow("min.2",  @() m.min(1, null))
mustThrow("max.1",  @() m.max(null, 1))
mustThrow("max.2",  @() m.max(1, null))
mustThrow("clamp.1",@() m.clamp(null, 0, 1))
mustThrow("clamp.2",@() m.clamp(0, null, 1))
mustThrow("clamp.3",@() m.clamp(0, 0, null))
mustThrow("deep_hash.2", @() m.deep_hash({}, null))

println("ok")
