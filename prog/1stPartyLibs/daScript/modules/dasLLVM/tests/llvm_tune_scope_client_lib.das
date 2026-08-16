options gen2

module llvm_tune_scope_client_lib public

require llvm/daslib/llvm_tune public

// Library-side fixture for llvm_tune_scope.das (no [test] on purpose — the suite just
// compiles it): declares the scope that llvm_tune_scope_client.das inherits by requiring
// this module. The fake tuner lives next to this file; the winners land in the app
// SIDECAR — <client>.tune.json beside the spawned client script (per-app IS the feature).
[tune_scope(name = "llvm_tune_scope_probe", tuner = "./llvm_tune_scope_tuner.das")]
struct private ScopeDecl {}

[tune_perm(k = 0), tune_perm(k = 1), tune_perm(k = 2),
 tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k1")]
def scope_add(a, b : int) : int {
    return a + b
}
