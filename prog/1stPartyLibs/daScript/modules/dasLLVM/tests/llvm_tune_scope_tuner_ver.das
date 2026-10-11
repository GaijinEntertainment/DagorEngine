options gen2

require llvm/daslib/llvm_tune
require llvm_tune_scope_verlib

// Instant fake tuner for the version-pinned scope (no [test] on purpose): records the winner
// AND stamps provenance.testver from the same constant the pin reads — the mint-side half of
// the contract (a tuner that skips the stamp re-tunes every start).
[export]
def main {
    print("FAKE TUNER VER -> {tune_manifest_path()}\n")
    tune_provenance_note("testver", "{TESTVER}")
    tune_manifest_set("ver_add", "k2")
}
