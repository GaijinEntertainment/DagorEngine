options gen2
options no_aot

require daslib/aot_cpp
require daslib/ast
require strings
require dastest/testing_boost public

[test]
def test_template_member_emit(t : T?) {
    var cpp = ""
    using() $(var cop : CodeOfPolicies) {
        cop.aot_macros = true
        cop.aot_module = true
        cpp = aot("{get_das_root()}/tests/aot/_template_member_fixture.das", false, false, cop)
    }
    t |> success(!empty(cpp), "emitter produced output")
    t |> success(find(cpp, "DAS_COMMENT(auto)") < 0, "no unresolved auto type in output")
    t |> success(find(cpp, "DAS_COMMENT(alias)") < 0, "no unresolved alias type in output")
}
