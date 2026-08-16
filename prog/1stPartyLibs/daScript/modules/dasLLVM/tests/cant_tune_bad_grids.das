// [tune] / [tune_perm] / [tune_companion] validation — covers every grid-shape error branch
// (llvm_tune.das). Each bad function fails one annotation apply with error 20800 (macro
// failed to apply).
options gen2
expect 20800:12

require llvm/daslib/llvm_tune

// 1. missing gen=
[tune_perm(k = 1), tune(fallback = "k1")]
def bad_no_gen(a, b : int) : int {
    return a + b
}

// 2. no [tune_perm] rows at all
[tune(gen = "llvm_code_selftest::add_plus_k")]
def bad_no_rows(a, b : int) : int {
    return a + b
}

// 3. rows listed AFTER tune(...) — applies run in declaration order, so tune sees none
[tune(gen = "llvm_code_selftest::add_plus_k"), tune_perm(k = 1)]
def bad_rows_after(a, b : int) : int {
    return a + b
}

// 4. duplicate perm suffix
[tune_perm(k = 1), tune_perm(k = 1), tune(gen = "llvm_code_selftest::add_plus_k")]
def bad_dup_suffix(a, b : int) : int {
    return a + b
}

// 5. 'reference' is reserved for the unstamped body
[tune_perm(suffix = "reference", k = 1), tune(gen = "llvm_code_selftest::add_plus_k")]
def bad_reserved_suffix(a, b : int) : int {
    return a + b
}

// 6. no explicit return type (function_to_type runs pre-infer; the registry needs it)
[tune_perm(k = 1), tune(gen = "llvm_code_selftest::add_plus_k")]
def bad_no_return_type(a, b : int) {
    return a + b
}

// well-formed companion fixtures for cases 7-12
def good_kv() : int {
    return -1
}

def kv_no_return_type(a : int) {
    return a
}

// 7. companion function not found in the module
[tune_perm(k = 1), tune_companion(fn = "no_such_fn", gen = "llvm_code_selftest::k_value"), tune(gen = "llvm_code_selftest::add_plus_k")]
def bad_comp_missing(a, b : int) : int {
    return a + b
}

// 8. companion row without gen=
[tune_perm(k = 1), tune_companion(fn = "good_kv"), tune(gen = "llvm_code_selftest::add_plus_k")]
def bad_comp_no_gen(a, b : int) : int {
    return a + b
}

// 9. companion listed AFTER tune(...) — the perm bank is already consumed (order guard)
[tune_perm(k = 1), tune(gen = "llvm_code_selftest::add_plus_k"), tune_companion(fn = "good_kv", gen = "llvm_code_selftest::k_value")]
def bad_comp_after(a, b : int) : int {
    return a + b
}

// 10. duplicate companion
[tune_perm(k = 1), tune_companion(fn = "good_kv", gen = "llvm_code_selftest::k_value"), tune_companion(fn = "good_kv", gen = "llvm_code_selftest::k_value"), tune(gen = "llvm_code_selftest::add_plus_k")]
def bad_comp_dup(a, b : int) : int {
    return a + b
}

// 11. companion without an explicit return type
[tune_perm(k = 1), tune_companion(fn = "kv_no_return_type", gen = "llvm_code_selftest::k_value"), tune(gen = "llvm_code_selftest::add_plus_k")]
def bad_comp_no_result(a, b : int) : int {
    return a + b
}

// 12. companion can't be the tuned function itself
[tune_perm(k = 1), tune_companion(fn = "bad_comp_self", gen = "llvm_code_selftest::k_value"), tune(gen = "llvm_code_selftest::add_plus_k")]
def bad_comp_self(a, b : int) : int {
    return a + b
}

[export]
def main {
}
