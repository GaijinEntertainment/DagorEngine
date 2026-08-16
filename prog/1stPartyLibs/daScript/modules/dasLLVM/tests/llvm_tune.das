options gen2

require dastest/testing_boost
require llvm/daslib/llvm_tune

// [tune] framework (llvm/daslib/llvm_tune): normal mode stamps the fallback perm's [llvm_code]
// onto the function itself; the add_plus_k selftest generator emits a+b+k so the stamped perm
// is observable at runtime. Same tolerance contract as llvm_code.das: dastest's in-process
// multi-program execution can serve a reference-tier result with jit instrumentation live, so
// the probe asserts "stamped-perm result or reference tier, never garbage". The grid modes and
// the manifest round-trip are compile-time-env driven (DAS_TUNE_MODE / DAS_TUNE_MANIFEST) and
// are gated by the CLI probes instead — see gemm_generator_plan.md M3.

// companion sibling (the two-function stamp): stamped with tadd_tuned's winner from the same
// entry — k_value emits `return k`, so the stamped perm is observable on the sibling too.
// Declared before the tuned function (the [tune_companion] declare-before rule).
def tadd_kv() : int {
    return -1  // reference tier sentinel
}

[tune_perm(k = 0), tune_perm(k = 2), tune_perm(suffix = "declined", decline = true),
 tune_companion(fn = "tadd_kv", gen = "llvm_code_selftest::k_value"),
 tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k2")]
def tadd_tuned(a, b : int) : int {
    return a + b
}

[tune_perm(k = 1), tune(gen = "llvm_code_selftest::add_plus_k")]  // no fallback: reference body
def tadd_reference(a, b : int) : int {
    return a + b
}

// per-ISA fallback chain: ';'-list walked in order, first perm whose requires= hint passes
// on this box wins. k9 requires a feature no box has (also pins that requires= is stripped
// from the auto-derived suffix — the row still keys as "k9"); k4 has no requires and stamps.
[tune_perm(k = 9, requires = "__no_such_cpuid_feature"), tune_perm(k = 4),
 tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k9;k4")]
def tadd_chain(a, b : int) : int {
    return a + b
}

[test]
def test_llvm_tune(t : T?) {
    var n = 2  // nolint:LINT003 — var defeats const-fold so the calls survive to codegen
    let r = tadd_tuned(n, 3)
    t |> success(r == 7 || r == 5)  // stamped k2 tier (jit) or reference tier — never anything else
    let kv = tadd_kv()
    t |> success(kv == 2 || kv == -1)  // the companion carries the same winner perm (k2) or reference
    if (!jit_enabled()) {
        t |> equal(r, 5)   // interpreted lane always runs the reference body
        t |> equal(kv, -1)
    }
    t |> equal(tadd_reference(n, 3), 5)  // no manifest + no fallback = reference body everywhere
    let rc = tadd_chain(n, 3)
    // chain skipped ineligible k9 -> stamped k4 (2+3+4=9, jit tier) or reference (5) — a k9
    // stamp would yield 14, so the tolerant pair still pins the chain choice
    t |> success(rc == 9 || rc == 5)
    if (!jit_enabled()) {
        t |> equal(rc, 5)
    }
}
