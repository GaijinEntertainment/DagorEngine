// hand-written (NOT generated): value-reinterpret consuming a lattice-returning extern
// DIRECTLY (no local hop) makes the interpreter read the call node through a typed eval
// slot (evalUInt/evalUInt64) — the interop dispatch must round-trip the vec4f slot for
// lattice results there. Regression for the W7 GPU-parity-gate catch: with a local hop
// or const-folded arguments the shape never executes, which is how it slipped the
// original lattice conformance suite.
options gen2
require dastest/testing_boost public

// runtime-opaque zeros: seeded from argv length so no optimizer pass can fold the
// lattice ctor calls below (a plain never-written `var` global gets const-promoted)
var runtime_fzero = 0.0
var runtime_izero = 0

[init]
def seed_runtime_zeros {
    let args <- get_command_line_arguments()
    // float mul-by-zero can't fold (NaN rules), int mul-by-zero folds — route through float
    runtime_fzero = float(length(args)) * 0.0
    runtime_izero = int(runtime_fzero)
}

def pack_bits(v : float2) : uint {
    return unsafe(reinterpret<uint>(half2(v)))
}

[test]
def test_lattice_reinterpret_composition(t : T?) {
    // every composition binds a `let` local: the typed local-set (Set_TT<uint>) is the
    // consumer that reads the extern call through evalUInt — argument-position use goes
    // through the safe vec4f path and would not regress
    t |> run("32-bit: reinterpret<uint> straight off the ctor call") @(t : T?) {
        let v = float2(1.5 + runtime_fzero, 2.0 + runtime_fzero)
        let got_half2 = unsafe(reinterpret<uint>(half2(v)))
        t |> equal(0x40003e00, got_half2)
        let got_fn = pack_bits(v)
        t |> equal(0x40003e00, got_fn)
        let got_short2 = unsafe(reinterpret<uint>(short2(int2(1000 + runtime_izero, -2000))))
        t |> equal(0xf83003e8, got_short2)
        let got_byte4 = unsafe(reinterpret<uint>(byte4(int4(1 + runtime_izero, 2, 3, -4))))
        t |> equal(0xfc030201, got_byte4)
    }
    t |> run("64-bit: reinterpret<uint64> straight off the ctor call") @(t : T?) {
        let got_half4 = unsafe(reinterpret<uint64>(half4(float4(1.5 + runtime_fzero, 0., 0., 2.0))))
        t |> equal(0x4000000000003e00ul, got_half4)
        let got_short4 = unsafe(reinterpret<uint64>(short4(int4(1000 + runtime_izero, -2000, 3000, -4000))))
        t |> equal(0xf0600bb8f83003e8ul, got_short4)
    }
    t |> run("reverse direction: lattice from reinterpreted runtime scalar") @(t : T?) {
        let bits = 0x40003e00 + uint(runtime_izero)
        let up = float2(unsafe(reinterpret<half2>(bits)))
        t |> equal(float2(1.5, 2.0), up)
    }
}
