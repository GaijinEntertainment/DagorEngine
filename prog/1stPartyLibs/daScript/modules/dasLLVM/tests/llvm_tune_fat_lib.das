options gen2

module llvm_tune_fat_lib public

require daslib/tune public
require llvm/daslib/llvm_tune public

// Library-side fixture for llvm_tune_fat.das (no [test] on purpose — the suite just compiles
// it): a scope shipping class profiles. The test writes the profiles into its working copy's
// redirected defaults dir, one per class it wants a clone for. The k3 row's requires= passes on
// every class of both ladders (avx2 on x64, neon on arm).
[tune_scope(name = "llvm_tune_fat_probe", defaults = "./llvm_tune_profiles_defaults")]
struct private ScopeDecl {}

// companion sibling — a fat build clones it per class too, and its dispatch switches with fat_add's
def fat_kv() : int {
    return -1  // reference tier sentinel
}

[tune_perm(k = 0), tune_perm(k = 1), tune_perm(k = 2),
 tune_perm(k = 3, requires = "avx2|neon"),
 tune_companion(fn = "fat_kv", gen = "llvm_code_selftest::k_value"),
 tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k1")]
def fat_add(a, b : int) : int {
    return a + b
}

// a void kernel: its dispatch arms call and return with no value, and the generator declines
// the signature, so every clone runs this reference body
[tune_perm(k = 0), tune_perm(k = 1), tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k1")]
def fat_void(var acc : int&; a : int) : void {
    acc = a + 1
}
