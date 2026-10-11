options gen2

require dastest/testing_boost
require daslib/aarch64_neon

// aarch64_neon intrinsic emission: under -jit on an aarch64 host the module's sdot4 family, the
// tbl16 pair and smmla lower to SDOT / TBL / SMMLA (smmla only where the host has i8mm) at the
// call sites inside the jitted run_* helpers, while the ref_* copies — the same arithmetic in a
// different module, so the name-keyed lookup never matches — compile as-is; every op is exact
// integer, so hardware-vs-reference isolates the emitter. Interp, foreign-arch and generic-exe
// lanes run the identical body on both sides (trivially green) — that IS the fallback contract
// the aarch64_neon header states. The helpers take no T? (dastest keeps such functions off the
// JIT), and each [test] witnesses that its helper is jitted before trusting the comparison.

def sx8(v : int) : int => ((v & 0xff) ^ 0x80) - 0x80

def packed_byte_sx(lane : int; e : int) : int => sx8(lane >> (e * 8))

def ref_sdot4(acc : int4; w : int8 const?; x : int8 const?) : int4 {
    var r = acc
    unsafe {
        for (j in range(4)) {
            var s = 0
            for (e in range(4)) {
                s += int(w[4 * j + e]) * int(x[4 * j + e])
            }
            r[j] += s
        }
    }
    return r
}

def ref_sdot4_w(acc : int4; w : int4; x : int8 const?) : int4 {
    var r = acc
    for (j in range(4)) {
        var s = 0
        for (e in range(4)) {
            s += packed_byte_sx(w[j], e) * int(unsafe(x[4 * j + e]))
        }
        r[j] += s
    }
    return r
}

def ref_sdot4_laneq(acc : int4; w : int8 const?; x : int8 const?; lane : int) : int4 {
    var r = acc
    unsafe {
        for (j in range(4)) {
            var s = 0
            for (e in range(4)) {
                s += int(w[4 * j + e]) * int(x[4 * lane + e])
            }
            r[j] += s
        }
    }
    return r
}

def ref_sdot4_laneq_w(acc : int4; w : int4; x : int8 const?; lane : int) : int4 {
    var r = acc
    for (j in range(4)) {
        var s = 0
        for (e in range(4)) {
            s += packed_byte_sx(w[j], e) * int(unsafe(x[4 * lane + e]))
        }
        r[j] += s
    }
    return r
}

def ref_tbl16(lut : int8 const?; nib : uint8 const?; high : bool) : int4 {
    var r = int4(0)
    unsafe {
        for (j in range(4)) {
            for (e in range(4)) {
                let n = int(nib[4 * j + e])
                let idx = high ? (n >> 4) : (n & 0x0f)
                r[j] |= (int(lut[idx]) & 0xff) << (e * 8)
            }
        }
    }
    return r
}

def ref_smmla(acc : int4; a : int8 const?; b : int8 const?) : int4 {
    var r = acc
    unsafe {
        for (i in range(2)) {
            for (j in range(2)) {
                var s = 0
                for (k in range(8)) {
                    s += int(a[8 * i + k]) * int(b[8 * j + k])
                }
                r[2 * i + j] += s
            }
        }
    }
    return r
}

struct Lcg {
    state : uint
}

def next_byte(var g : Lcg) : int8 {
    g.state = g.state * 1664525u + 1013904223u
    return int8(int(g.state >> 24u) - 128)
}

def fill_bytes(var g : Lcg; var dst : array<int8>) {
    for (v in dst) {
        v = next_byte(g)
    }
}

def fill_nibbles(var g : Lcg; var dst : array<uint8>) {
    for (v in dst) {
        v = uint8(next_byte(g))
    }
}

let TRIALS = 256

//! each [test] bumps this before calling a helper: a pure call with constant arguments folds away at compile time, before the JIT ever sees its call sites
var g_salt = 0u

// mismatch count over TRIALS random int8 vectors; the lane is a compile-time constant of the
// intrinsic, so the four lane forms are spelled out
def run_sdot4_family(salt : uint) : int {
    var g = Lcg(state = 0x9e3779b9u ^ salt)
    var w : array<int8>
    var x : array<int8>
    w |> resize(16)
    x |> resize(16)
    var bad = 0
    for (trial in range(TRIALS)) {
        fill_bytes(g, w)
        fill_bytes(g, x)
        let acc = int4(trial, -trial, trial * 7, 3 - trial)
        unsafe {
            let wp = addr(w[0])
            let xp = addr(x[0])
            if (sdot4(acc, wp, xp) != ref_sdot4(acc, wp, xp)) {
                bad++
            }
            let packed = ref_tbl16(wp, reinterpret<uint8 const?>(xp), false)
            if (sdot4_w(acc, packed, xp) != ref_sdot4_w(acc, packed, xp)) {
                bad++
            }
            if (sdot4_laneq(acc, wp, xp, 0) != ref_sdot4_laneq(acc, wp, xp, 0)) {
                bad++
            }
            if (sdot4_laneq(acc, wp, xp, 1) != ref_sdot4_laneq(acc, wp, xp, 1)) {
                bad++
            }
            if (sdot4_laneq(acc, wp, xp, 2) != ref_sdot4_laneq(acc, wp, xp, 2)) {
                bad++
            }
            if (sdot4_laneq(acc, wp, xp, 3) != ref_sdot4_laneq(acc, wp, xp, 3)) {
                bad++
            }
            if (sdot4_laneq_w(acc, packed, xp, 0) != ref_sdot4_laneq_w(acc, packed, xp, 0)) {
                bad++
            }
            if (sdot4_laneq_w(acc, packed, xp, 1) != ref_sdot4_laneq_w(acc, packed, xp, 1)) {
                bad++
            }
            if (sdot4_laneq_w(acc, packed, xp, 2) != ref_sdot4_laneq_w(acc, packed, xp, 2)) {
                bad++
            }
            if (sdot4_laneq_w(acc, packed, xp, 3) != ref_sdot4_laneq_w(acc, packed, xp, 3)) {
                bad++
            }
        }
    }
    return bad
}

def run_tbl16(salt : uint) : int {
    var g = Lcg(state = 0x2545f491u ^ salt)
    var lut : array<int8>
    var nib : array<uint8>
    lut |> resize(16)
    nib |> resize(16)
    var bad = 0
    for (_trial in range(TRIALS)) {
        fill_bytes(g, lut)
        fill_nibbles(g, nib)
        unsafe {
            let lp = addr(lut[0])
            let np = addr(nib[0])
            if (tbl16_lo(lp, np) != ref_tbl16(lp, np, false)) {
                bad++
            }
            if (tbl16_hi(lp, np) != ref_tbl16(lp, np, true)) {
                bad++
            }
        }
    }
    return bad
}

def run_smmla(salt : uint) : int {
    var g = Lcg(state = 0x7f4a7c15u ^ salt)
    var a : array<int8>
    var b : array<int8>
    a |> resize(16)
    b |> resize(16)
    var bad = 0
    for (trial in range(TRIALS)) {
        fill_bytes(g, a)
        fill_bytes(g, b)
        let acc = int4(-trial, trial, 11 - trial, trial * 5)
        unsafe {
            let ap = addr(a[0])
            let bp = addr(b[0])
            if (smmla(acc, ap, bp) != ref_smmla(acc, ap, bp)) {
                bad++
            }
        }
    }
    return bad
}

[test]
def test_sdot4_family(t : T?) {
    g_salt++
    t |> run("sdot4 / sdot4_w / sdot4_laneq / sdot4_laneq_w: hardware == reference") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@run_sdot4_family), "the helper is jitted, so its call sites reach the emitter")
        t |> equal(run_sdot4_family(g_salt), 0, "every sdot4-family lane matches the reference over {TRIALS} random int8 vectors")
    }
}

[test]
def test_tbl16(t : T?) {
    g_salt++
    t |> run("tbl16_lo / tbl16_hi: hardware == reference") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@run_tbl16), "the helper is jitted, so its call sites reach the emitter")
        t |> equal(run_tbl16(g_salt), 0, "every tbl16 lookup matches the reference over {TRIALS} random tables")
    }
}

[test]
def test_smmla(t : T?) {
    g_salt++
    t |> run("smmla: hardware == reference") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@run_smmla), "the helper is jitted, so its call sites reach the emitter")
        t |> equal(run_smmla(g_salt), 0, "every smmla 2x2 product matches the reference over {TRIALS} random int8 matrices")
    }
}
