options gen2

require dastest/testing_boost public
require daslib/tune

// The daslib/tune shells: any program spells the tune annotations and compiles on every tier -
// with dasLLVM (stamped, or unstamped under DAS_TUNE_POLICY=reference) and without it, where
// every apply is a no-op and the reference body IS the function. The gate is that the answer
// never moves between those tiers, so the reference body and the generator agree by
// construction: `llvm_code_selftest::add_plus_k` emits `a + b + k` for an (int, int) : int
// stub, and the k7 perm this grid falls back to emits exactly what the body below computes.

[tune_scope(name = "test_tune_shells")]
struct private TuneShellsScope {}

def private ref_add(a, b : int) : int {
    return a + b + 7
}

[tune_perm(k = 7), tune_perm(k = 0),
 tune(gen = "llvm_code_selftest::add_plus_k", fallback = "k7")]
def shell_add(a, b : int) : int {
    return a + b + 7
}

[tune_policy(missing = "fallback")]
def private shell_policy_probe(a : int) : int {
    return a * 2
}

[test]
def test_tune_shell_result_matches_the_reference(t : T?) {
    for (a in range(-3, 4)) {
        for (b in range(-3, 4)) {
            t |> equal(shell_add(a, b), ref_add(a, b), "shell_add({a}, {b})")
        }
    }
}

[test]
def test_tune_policy_leaves_the_body_alone(t : T?) {
    t |> equal(shell_policy_probe(21), 42)
}

// the shells reach the framework through `require [tune_framework] public`: on a build that
// carries the `llvm` witness the group has its member, and the shells see it
[test]
def test_tune_framework_group_carries_the_member(t : T?) {
    static_if (typeinfo builtin_module_exists(llvm)) {
        t |> success(typeinfo module_exists(llvm_tune), "the tune_framework group brings llvm_tune to daslib/tune on a build with the witness")
    } else {
        t |> success(!typeinfo module_exists(llvm_tune), "no witness, no framework")
    }
}
