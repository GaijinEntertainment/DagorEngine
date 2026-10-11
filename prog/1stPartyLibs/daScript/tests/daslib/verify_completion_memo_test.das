options gen2
options no_aot

require dastest/testing_boost public
require daslib/ast_boost
require daslib/rtti
require strings

//! `[verify_completion]` walks the call graph looking for recursion. The walk memoizes the
//! functions it has already cleared, so a wide fan-out graph costs one visit per function
//! instead of one per path. Without the memo the fixture below takes tens of seconds.

let private BUDGET_USEC = 5000000

def private fixture_path(name : string) : string {
    return "{get_das_root()}/tests/daslib/_verify_completion_{name}_fixture.das"
}

[test]
def test_recursion_walk_visits_each_function_once(t : T?) {
    t |> run("wide fan-out graph compiles within budget") @(t : T?) {
        var inscope access <- make_file_access("")
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.threadlock_context = true
                let started = ref_time_ticks()
                compile_file(fixture_path("fanout"), access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                    let elapsed = get_time_usec(started)
                    t |> success(ok, "fixture must compile: {issues}")
                    t |> success(elapsed < BUDGET_USEC, "compiled in {elapsed}us, budget {BUDGET_USEC}us")
                }
            }
        }
    }
}

[test]
def test_recursion_through_a_shared_callee_is_still_reported(t : T?) {
    t |> run("cycle closing through a shared function fails to compile") @(t : T?) {
        var inscope access <- make_file_access("")
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.threadlock_context = true
                compile_file(fixture_path("cycle"), access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                    t |> success(!ok, "cyclic fixture must not compile")
                    t |> success(find("{issues}", "recursive calls are not allowed") >= 0, "expected a recursion error, got: {issues}")
                }
            }
        }
    }
}
