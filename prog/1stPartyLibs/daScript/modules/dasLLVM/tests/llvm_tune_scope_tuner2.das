options gen2

require llvm/daslib/llvm_tune

// Fake tuner for scope llvm_tune_scope_probe2 (no [test] on purpose): upserts ONLY its own
// key into the shared app sidecar — running after tuner 1 it must not clobber scope_add
// (upsert-preservation is the isolation contract of the one-file-per-app world).
[export]
def main {
    print("FAKE TUNER2 -> {tune_manifest_path()}\n")
    tune_manifest_set("scope_mul", "k0")
}
