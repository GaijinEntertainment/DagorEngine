options gen2

require llvm/daslib/llvm_tune
require llvm_tune_scope_verlib

// Instant fake tuner for the version_key= scope (no [test] on purpose): stamps the pin under
// the OVERRIDDEN provenance key, from the same constant the pin reads.
[export]
def main {
    print("FAKE TUNER VER2 -> {tune_manifest_path()}\n")
    tune_provenance_note("tv2", "{TESTVER}")
    tune_manifest_set("ver2_add", "k2")
}
