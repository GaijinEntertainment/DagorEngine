options gen2

module llvm_tune_profiles_lib public

require daslib/tune public
require llvm/daslib/llvm_tune public

// Library-side fixture for llvm_tune_profiles.das (no [test] on purpose — the suite just
// compiles it): a scope that also ships defaults profiles. The defaults dir is created and
// filled BY THE TEST (the profile file name carries this box's tune_cpu_class(), which the
// test computes in-process). The k3 row's requires= passes on every test box (avx2 on x64,
// neon on arm) — the "unlocked seat" whose absence from a profile's features forces a race.
[tune_scope(name = "llvm_tune_profiles_probe", tuner = "./llvm_tune_profiles_tuner.das",
            defaults = "./llvm_tune_profiles_defaults")]
struct private ScopeDecl {}

[tune_perm(k = 0), tune_perm(k = 1), tune_perm(k = 2),
 tune_perm(k = 3, requires = "avx2|neon"),
 tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k1")]
def prof_add(a, b : int) : int {
    return a + b
}
