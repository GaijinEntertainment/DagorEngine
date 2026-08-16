options gen2

require dastest/testing_boost
require ../../modules/dasLLVM/daslib/llvm_code.das

// [llvm_code] external-codegen rail: the selftest generator (llvm/daslib/llvm_code_selftest,
// wired via llvm/daslib/llvm_user_modules) emits `a + b` for (int, int) : int stubs and
// declines any other signature. Reference bodies carry interp/AOT/declined-tier semantics.
// This test pins the correctness contract in every lane; the strict generator-FIRED gate
// (sentinel must flip) is a single-program property — dastest's in-process multi-program
// execution can serve a reference-tier result with jit instrumentation live, so here the
// probe only asserts "generated or reference, never garbage".

[llvm_code(name = "llvm_code_selftest::add_i32")]
def jadd(a, b : int) : int {
    return a + b
}

[llvm_code(name = "llvm_code_selftest::add_i32")]
def jadd_probe(a, b : int) : int {
    return a - b - 1000000  // sentinel result — visible only when the generator did NOT replace the body
}

[llvm_code(name = "llvm_code_selftest::add_i32")]
def fsub_decline(a, b : float) : float {
    return a - b  // generator declines (float): the reference body compiles as the fallback tier
}

[test]
def test_llvm_code(t : T?) {
    var n = 2  // nolint:LINT003 — var defeats const-fold so the calls survive to codegen
    t |> equal(jadd(n, 3), 5)  // bit-exact whichever tier ran
    t |> success(fsub_decline(float(n), 2f) == 0f)  // decline -> reference body everywhere
    let r = jadd_probe(n, 3)
    t |> success(r == 5 || r == -1000001)  // generated (a+b) or reference tier — never anything else
    if (!jit_enabled()) {
        t |> equal(r, -1000001)  // interpreted lane always runs the reference body
    }
}
