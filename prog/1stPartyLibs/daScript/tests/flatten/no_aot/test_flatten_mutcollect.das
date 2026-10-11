options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten_opt_common
require daslib/ast
require daslib/ast_boost
require daslib/rtti

//! `collect_mutable` is what tells CSE and the preshader which names are value-stable, so every
//! spelling a store can take has to reach it. A store form it cannot see is a name CSE believes
//! constant: it shares a subexpression straight across the mutation and the twin computes the
//! pre-store value twice. `_mut_fixtures.das` carries one local per spelling.
//!
//! Lives in tests/flatten/no_aot/ (excluded from test_aot via tests/.das_test): it compiles the
//! fixture at runtime, which pulls macro_boost generic instantiations that don't AOT-link.

def private fixture_mutables(t : T?; fnName : string) : table<string> {
    var res : table<string>
    let path = "{get_das_root()}/tests/flatten/no_aot/_mut_fixtures.das"
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.version_2_syntax = true
            compile_file(path, access, unsafe(addr(mg)), cop) $(ok; program; iss) {
                if (!ok) {
                    t |> failure("compile failed: {path}\n{iss}")
                    return
                }
                program_for_each_module(program) $(mod) {
                    for_each_function(mod, "") $(func) {
                        if (func.body == null || func.flags.builtIn || "{func.name}" != fnName) {
                            return
                        }
                        var m <- collect_mutable(func.body as ExprBlock)
                        for (k in keys(m)) {
                            res |> insert(k)
                        }
                        delete m
                    }
                }
            }
        }
    }
    return <- res
}

def private assert_arg_guard(t : T?; fnName : string) {
    let path = "{get_das_root()}/tests/flatten/no_aot/_mut_fixtures.das"
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.version_2_syntax = true
            compile_file(path, access, unsafe(addr(mg)), cop) $(ok; program; iss) {
                if (!ok) {
                    t |> failure("compile failed: {path}\n{iss}")
                    return
                }
                var seen = false
                program_for_each_module(program) $(mod) {
                    for_each_function(mod, "") $(func) {
                        if (func.body == null || func.flags.builtIn || "{func.name}" != fnName) {
                            return
                        }
                        seen = true
                        t |> equal(length(func.arguments), 2, "mut_out takes a value and a var ref")
                        t |> success(!writes_through_argument(func, 0), "a by-value parameter is not a write target")
                        t |> success(writes_through_argument(func, 1), "a 'var T&' parameter is a write target")
                        t |> success(!writes_through_argument(func, -1), "a negative index answers false, never throws")
                        t |> success(!writes_through_argument(func, 2), "an index past the last argument answers false")
                    }
                }
                t |> success(seen, "{fnName} must be found")
            }
        }
    }
}

[test]
def test_flatten_mutcollect(t : T?) {
    t |> run("every store spelling reaches collect_mutable") @(t : T?) {
        var m <- fixture_mutables(t, "mut_forms")
        t |> success(!empty(m), "mut_forms must be found and walked")
        for (name in ["cp", "mv", "src", "cl", "inc", "acc", "lane", "sn", "cs"]) {
            t |> success(key_exists(m, name), "'{name}' is written in mut_forms but is not in the mutable set")
        }
        t |> success(!key_exists(m, "ro"), "'ro' is only read; a read must not enter the mutable set")
        delete m
    }
    t |> run("writes_through_argument answers false off both ends of the argument list") @(t : T?) {
        assert_arg_guard(t, "mut_out")
    }
}
