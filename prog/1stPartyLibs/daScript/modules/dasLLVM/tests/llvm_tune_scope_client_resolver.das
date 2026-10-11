options gen2

require daslib/tune
require llvm_tune_scope_client_lib

// Resolver fixture for llvm_tune_scope.das (no [test] on purpose — the suite just compiles
// it): registers a scope resolver from [init], which runs before the injected restart guard.
// The guard must consult it INSTEAD of spawning the tuner; the resolver satisfies the scope
// the way an exchange client would — write the winner, print its own restart marker.
[init]
def register_resolver {
    tune_set_scope_resolver() <| @@(scopeName : string; manifestPath : string; fnames : string) : bool {
        print("RESOLVER {scopeName} -> {manifestPath} ({fnames})\n")
        set_tune_manifest_runtime_path(manifestPath)
        tune_manifest_set("scope_add", "k2")
        print("exchange: sidecar applied, restart to apply the winners\n")
        return true
    }
}

[export, tune_policy(missing = "restart")]
def main {
    var n = 2  // nolint:LINT003 — var defeats const-fold so the calls survive to codegen
    print("RESULT {scope_add(n, 3)}\n")
    var st <- tune_status()
    for (s in st) {
        print("STATUS {s.fname}|{s.scope}|{s.suffix}|{s.source}\n")
    }
    delete st
}
