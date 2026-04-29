// Squirrel module under test: dagor.random (registered by random.cpp)

from "dagor.random" import rnd, frnd, srnd, rnd_float, rnd_int, rnd_bound, rnd_range,
  set_rnd_seed, get_rnd_seed, gauss_rnd, uint32_hash, uint_noise1D, uint_noise2D, uint_noise3D, DAGOR_RAND_MAX


// ============================================================
// Section 1: seed determinism
// ============================================================
println("--- Section 1: seed determinism ---")

assert(typeof DAGOR_RAND_MAX == "integer", "DAGOR_RAND_MAX is integer")

set_rnd_seed(12345)
assert(get_rnd_seed() == 12345, "get_rnd_seed reflects set value")

let v1 = rnd()
let v2 = rnd()
let v3 = rnd()

set_rnd_seed(12345)
let v1b = rnd()
let v2b = rnd()
let v3b = rnd()

assert(v1 == v1b && v2 == v2b && v3 == v3b, $"reseed reproduces: {v1},{v2},{v3} vs {v1b},{v2b},{v3b}")

println("Section 1 PASSED")


// ============================================================
// Section 2: bounded values stay in range
// ============================================================
println("--- Section 2: bounded values stay in range ---")

set_rnd_seed(7777)
for (local i = 0; i < 50; i++) {
  let r = rnd_int(10, 20)
  assert(r >= 10 && r <= 20, $"rnd_int(10,20) in range: {r}")
}

for (local i = 0; i < 50; i++) {
  let f = rnd_float(0.0, 1.0)
  assert(f >= 0.0 && f <= 1.0, $"rnd_float(0,1) in range: {f}")
}

for (local i = 0; i < 50; i++) {
  let b = rnd_bound(100)
  assert(b >= 0 && b < 100, $"rnd_bound(100) in [0,100): {b}")
}

for (local i = 0; i < 50; i++) {
  let r = rnd_range(5, 15)
  assert(r >= 5 && r <= 15, $"rnd_range in [5,15]: {r}")
}

let f1 = frnd()
assert(f1 >= 0.0 && f1 <= 1.0, $"frnd in [0,1]: {f1}")

let s1 = srnd()
assert(s1 >= -1.0 && s1 <= 1.0, $"srnd in [-1,1]: {s1}")

println("Section 2 PASSED")


// ============================================================
// Section 3: gauss_rnd valid lookup tables
// ============================================================
println("--- Section 3: gauss_rnd valid lookup tables ---")

let g0 = gauss_rnd(0)
let g1 = gauss_rnd(1)
let g2 = gauss_rnd(2)
assert(typeof g0 == "float" && typeof g1 == "float" && typeof g2 == "float", "gauss_rnd returns float")

println("Section 3 PASSED")


// ============================================================
// Section 4: uint hash + noise determinism
// ============================================================
println("--- Section 4: uint hash + noise determinism ---")

let h1 = uint32_hash(42)
let h2 = uint32_hash(42)
assert(h1 == h2, $"uint32_hash deterministic: {h1} vs {h2}")
assert(uint32_hash(43) != uint32_hash(42), "uint32_hash differs for different input")

let n1 = uint_noise1D(7, 999)
let n2 = uint_noise1D(7, 999)
assert(n1 == n2, "uint_noise1D deterministic")

let n2d = uint_noise2D(1, 2, 999)
assert(n2d == uint_noise2D(1, 2, 999), "uint_noise2D deterministic")

let n3d = uint_noise3D(1, 2, 3, 999)
assert(n3d == uint_noise3D(1, 2, 3, 999), "uint_noise3D deterministic")

println("Section 4 PASSED")


// ============================================================
// Section 5: incorrect input
// ============================================================
println("--- Section 5: incorrect input ---")

local threw = false
try {
  gauss_rnd(-1)
} catch (e) {
  threw = true
}
assert(threw, "gauss_rnd(-1) must throw (range [0..2])")

threw = false
try {
  gauss_rnd(3)
} catch (e) {
  threw = true
}
assert(threw, "gauss_rnd(3) must throw (range [0..2])")

println("Section 5 PASSED")


println("ALL TESTS PASSED")
