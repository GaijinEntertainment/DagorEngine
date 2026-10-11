options gen2
options no_aot

require dastest/testing_boost public
require daslib/ast_boost
require daslib/rtti
require strings

def fixture_path(name : string) : string {
    return "{get_das_root()}/tests/daslib/{name}.das"
}

def compile_fixture(name : string; blk : block<(ok : bool; issues : string) : void>) {
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            compile_file(fixture_path(name), access, unsafe(addr(mg)), cop) $(ok; _program; issues) {
                invoke(blk, ok, "{issues}")
            }
        }
    }
}

[test]
def test_only_nttp_annotation(t : T?) {
    t |> run("kernel calling only inline-bound externs compiles") @(t : T?) {
        compile_fixture("only_nttp_fixture_good") $(ok, issues) {
            t |> success(ok, "expected clean compile, got: {issues}")
        }
    }
    t |> run("kernel calling a member-bound extern fails naming the call") @(t : T?) {
        compile_fixture("cant_only_nttp_fixture") $(ok, issues) {
            t |> success(!ok, "expected compile failure, fixture compiled clean")
            t |> success(issues |> find("only_nttp") >= 0, "diagnostic must name the annotation: {issues}")
            t |> success(issues |> find("get_env_variable") >= 0, "diagnostic must name the offending call: {issues}")
        }
    }
}
