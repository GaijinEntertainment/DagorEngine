options gen2

require llvm/daslib/llvm_tune

// Template fixture for llvm_tune_manifest.das (no [test] on purpose — the suite just
// compile-checks it in place, where the app sidecar is absent). The test COPIES this file
// into a temp directory and spawns it there: the per-app sidecar resolves beside the ROOT
// SCRIPT (<client>.tune.json — no declaration needed), so it lands in the temp dir.
// Spawned with "write" among the args it records k2 as the winner via tune_manifest_set
// (the harness write API); spawned again without, the fresh compile stamps man_add from
// the sidecar and RESULT reports the k2 tier.

// companion sibling: the declaration-driven re-stamp must reach it from man_add's entry
def man_kv() : int {
    return -1  // reference tier sentinel
}

[tune_perm(k = 0), tune_perm(k = 1), tune_perm(k = 2),
 tune_companion(fn = "man_kv", gen = "llvm_code_selftest::k_value"),
 tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k1")]
def man_add(a, b : int) : int {
    return a + b
}

// NO sidecar entry: the fallback stamp must stand (a sidecar written before a kernel
// family landed must not drop it to the reference tier)
[tune_perm(k = 0), tune_perm(k = 1), tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k1")]
def man_other(a, b : int) : int {
    return a + b
}

// explicit "reference" sidecar entry: the original body must run despite the fallback
[tune_perm(k = 0), tune_perm(k = 1), tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k1")]
def man_ref(a, b : int) : int {
    return a + b
}

[export]
def main {
    var n = 2  // nolint:LINT003 — var defeats const-fold so the calls survive to codegen
    let args <- get_command_line_arguments()  // locked view — do not delete
    var doWrite = false
    for (a in args) {
        doWrite ||= a == "write"
    }
    if (doWrite) {
        let ok = tune_manifest_set("man_add", "k2") && tune_manifest_set("man_ref", "reference")
        print("WRITE {ok ? "ok" : "FAILED"} -> {tune_manifest_path()}\n")
    } else {
        print("RESULT {man_add(n, 3)}\n")
        print("KV {man_kv()}\n")
        print("OTHER {man_other(n, 3)}\n")
        print("REF {man_ref(n, 3)}\n")
    }
}
