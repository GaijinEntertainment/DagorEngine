options gen2

require llvm/daslib/llvm_tune

// Instant fake tuner for llvm_tune_scope.das (no [test] on purpose): records k2 for
// scope_add via the upsert API — the framework points it at the app sidecar through
// DAS_TUNE_MANIFEST, and the upsert must leave every other scope's keys alone.
[export]
def main {
    print("FAKE TUNER -> {tune_manifest_path()}\n")
    tune_manifest_set("scope_add", "k2")
}
