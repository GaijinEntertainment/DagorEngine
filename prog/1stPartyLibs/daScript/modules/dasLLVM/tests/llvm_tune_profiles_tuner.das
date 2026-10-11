options gen2

require llvm/daslib/llvm_tune

// Instant fake tuner for llvm_tune_profiles.das (no [test] on purpose): prints the
// DAS_TUNE_ONLY filter the guard armed (the race-on-unlock proof) and records k2 — a winner
// distinct from both the fallback (k1) and the profiles the test writes, so RESULT
// fingerprints which path stamped.
[export]
def main {
    print("FAKE TUNER -> {tune_manifest_path()} ONLY='{tune_only_filter()}'\n")
    if (tune_family_selected("prof_add")) {
        tune_manifest_set("prof_add", "k2")
    }
}
