options gen2

module llvm_tune_scope_client_lib2 public

require llvm/daslib/llvm_tune public

// Second library fixture for llvm_tune_scope.das (no [test] on purpose): a SEPARATE scope
// with its own tuner — the two-libraries-each-ship-tuning case. Both scopes share the ONE
// app sidecar; each tuner upserts its own keys and must preserve the other's.
[tune_scope(name = "llvm_tune_scope_probe2", tuner = "./llvm_tune_scope_tuner2.das")]
struct private ScopeDecl2 {}

[tune_perm(k = 0), tune_perm(k = 1), tune_perm(k = 2),
 tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k2")]
def scope_mul(a, b : int) : int {
    return a + b
}
