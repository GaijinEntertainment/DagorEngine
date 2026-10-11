options gen2

module llvm_tune_scope_early public

require daslib/tune public
require llvm/daslib/llvm_tune public

// Helper for cant_tune_scope_after_covered.das: a kernel module with no scope of its own, so the
// scope that covers it can only arrive after it compiled. No [test] on purpose - the suite just
// compiles it (fallback stamps, no scope declared).
[tune_perm(k = 0), tune_perm(k = 1),
 tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k1")]
def early_add(a, b : int) : int {
    return a + b
}
