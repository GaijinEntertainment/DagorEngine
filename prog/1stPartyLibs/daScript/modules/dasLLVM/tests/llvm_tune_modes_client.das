options gen2

require dastest/testing_boost
require llvm/daslib/llvm_tune

// Doubles as the child fixture for llvm_tune_modes.das (spawned with DAS_TUNE_MODE=test, -jit:
// main prints every grid variant's result) and as an ordinary suite test (normal mode: the
// [test] below checks the fallback contract). The variants registry exists in every mode —
// reference-row-only under normal — so both faces compile from the same source.

// companion sibling — grid modes clone it per perm too (modes_kv__k0 ... ), so the child run
// proves per-perm args reach the companion generator and the declined row falls back with it
def modes_kv() : int {
    return -1  // reference tier sentinel
}

[tune_perm(k = 0), tune_perm(k = 1), tune_perm(k = 2), tune_perm(suffix = "declined", decline = true),
 tune_companion(fn = "modes_kv", gen = "llvm_code_selftest::k_value"),
 tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k1")]
def modes_add(a, b : int) : int {
    return a + b
}

[export]
def main {
    var n = 2  // nolint:LINT003 — var defeats const-fold so the calls survive to codegen
    var vs <- modes_add_variants()
    for (v in vs) {
        print("VARIANT {v._0} -> {invoke(v._1, n, 3)}\n")
    }
    delete vs
    var kvs <- modes_kv_variants()
    for (v in kvs) {
        print("KVARIANT {v._0} -> {invoke(v._1)}\n")
    }
    delete kvs
}

[test]
def test_modes_client_fallback(t : T?) {
    var n = 2  // nolint:LINT003 — var defeats const-fold so the calls survive to codegen
    let r = modes_add(n, 3)
    t |> success(r == 6 || r == 5)  // stamped fallback k1 tier (jit) or reference tier
    let kv = modes_kv()
    t |> success(kv == 1 || kv == -1)  // companion carries the same fallback perm (k1) or reference
    if (!jit_enabled()) {
        t |> equal(r, 5)
        t |> equal(kv, -1)
    }
}
