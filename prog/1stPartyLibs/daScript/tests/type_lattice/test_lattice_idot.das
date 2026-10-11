// hand-written (NOT generated): the idot family — exact integer dot products and the byte
// table shuffle, the first compute ops on the 8-bit lattice. Values are hand-computed so
// every tier (interp, JIT native/generic, AOT verbatim-extern) must reproduce them bit-exact.
// Compute lives in plain helpers (T? params keep a function off the JIT — the lambdas only assert).
options gen2
require dastest/testing_boost public

def private mk_a() : byte16 => byte16(1, -2, 3, -4, 5, -6, 7, -8, 9, -10, 11, -12, 13, -14, 15, -16)
def private mk_b() : byte16 => byte16(2, 3, -4, 5, -6, 7, 8, -9, 10, 11, -12, 13, -14, 15, 16, -17)
// full-range unsigned lanes (255, 128) prove zero-extension of a
def private mk_ua() : ubyte16 => ubyte16(255u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 128u)

def private idot4_ss_pure() : int4 => idot4(mk_a(), mk_b())
def private idot4_ss_acc() : int4 => idot4(int4(100, 200, 300, 400), mk_a(), mk_b())
def private idot4_us_pure() : int4 => idot4(mk_ua(), mk_b())
def private idot4_us_acc() : int4 => idot4(int4(1, 2, 3, 4), mk_ua(), mk_b())
def private idot_ss() : int => idot(mk_a(), mk_b())
def private idot_us() : int => idot(mk_ua(), mk_b())

def private splat_b16(v : int) : byte16 => byte16(v)

def private splat_ub16(v : uint) : ubyte16 => ubyte16(v)

def private idot4_extreme_ss() : int4 => idot4(splat_b16(-128), splat_b16(127))
def private idot_extreme_ss() : int => idot(splat_b16(-128), splat_b16(127))
def private idot4_extreme_us() : int4 => idot4(splat_ub16(255u), splat_b16(-128))
def private idot_extreme_us() : int => idot(splat_ub16(255u), splat_b16(-128))

def private mk_lut() : byte16 => byte16(10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, -10, -20, -30, -40)

def private shuffle_masked() : byte16 {
    // indices 31 and 16 exercise the &15 mask (tbl-with-mask / pshufb-positive semantic)
    return shuffle(mk_lut(), ubyte16(15u, 14u, 13u, 12u, 0u, 1u, 2u, 3u, 31u, 16u, 5u, 5u, 5u, 5u, 7u, 8u))
}

def private shuffle_identity() : byte16 {
    return shuffle(mk_lut(), ubyte16(0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u))
}

[test]
def test_lattice_idot(t : T?) {
    t |> run("idot4 signed x signed") @(t : T?) {
        t |> success(idot4_ss_pure() == int4(-36, 56, -308, 120), "pure form")
        t |> success(idot4_ss_acc() == int4(64, 256, -8, 520), "accumulate form")
    }
    t |> run("idot4 unsigned x signed") @(t : T?) {
        let r = idot4_us_pure()
        t |> equal(r.x, 524)
        t |> equal(r.w, -1908)
        t |> success(idot4_us_acc() == r + int4(1, 2, 3, 4), "accumulate form")
    }
    t |> run("idot full reduce") @(t : T?) {
        t |> equal(idot_ss(), -168)
        let m = idot4_us_pure()
        t |> equal(idot_us(), m.x + m.y + m.z + m.w)
    }
    t |> run("idot extremes do not overflow lanes") @(t : T?) {
        // most negative product set: (-128 * 127) * 16 = -260096 — far inside int32, must be exact
        t |> success(idot4_extreme_ss() == int4(-65024, -65024, -65024, -65024), "sxs lane sums")
        t |> equal(idot_extreme_ss(), -260096)
        t |> success(idot4_extreme_us() == int4(-130560, -130560, -130560, -130560), "uxs lane sums")
        t |> equal(idot_extreme_us(), -522240)
    }
    t |> run("shuffle byte table") @(t : T?) {
        t |> success(shuffle_masked() == byte16(-40, -30, -20, -10, 10, 20, 30, 40, -40, 10, 60, 60, 60, 60, 80, 90), "masked select")
        t |> success(shuffle_identity() == mk_lut(), "identity round-trip")
    }
}
