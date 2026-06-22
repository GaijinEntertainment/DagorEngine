options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten
require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/rtti
require strings

//! Re-entry contract for `flatten_optimize`: a backend may rewrite an already-optimized body and
//! call the pass again. The pass seeds its `_preshader_N` / `_cse_N` counters from the names
//! already present, so a second call must hoist NEW work under FRESH names instead of re-minting
//! `_preshader_0` / `_cse_0` (a duplicate-name compile error once the body re-infers). The test
//! compiles _reentry_fixture.das (its twin keeps one of each name), injects a new uniform let and
//! a new duplicated varying pair — the way a backend rewrite would — re-runs the pass, and
//! asserts every preshader/cse name in the block is still unique.

def private let_name(s : ExpressionPtr) : string {
    if (s is ExprLet && length((s as ExprLet).variables) == 1) {
        return "{(s as ExprLet).variables[0].name}"
    }
    return ""
}

// Partition the block's single-var let names into the two counter classes:
// `_preshader_cse_*`/`_cse_*` belong to the cse counter, remaining `_preshader_*` to extraction.
def private collect_class_names(blk : ExprBlock?; var pres : array<string>; var cses : array<string>) {
    for (s in blk.list) {
        let nm = let_name(s)
        if (empty(nm)) {
            continue
        }
        if (nm |> starts_with("_preshader_cse_") || nm |> starts_with("_cse_")) {
            cses |> push(nm)
        } elif (nm |> starts_with("_preshader_")) {
            pres |> push(nm)
        }
    }
}

def private has_dup(names : array<string>) : bool {
    var seen : table<string>
    for (n in names) {
        if (key_exists(seen, n)) return true
        seen |> insert(n)
    }
    return false
}

[test]
def test_flatten_reentry(t : T?) {
    let root = get_das_root()
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.threadlock_context = true
            cop.version_2_syntax = true
            compile_file("{root}/tests/flatten/no_aot/_reentry_fixture.das", access, unsafe(addr(mg)), cop) $(ok; program; iss) {
                if (!ok) {
                    t |> failure("compile failed: _reentry_fixture.das\n{iss}")
                    return
                }
                var checked = 0
                program_for_each_module(program) $(mod) {
                    for_each_function(mod, "") $(func) {
                        if ("{func.name}" != "reentry_flat" || func.body == null) {
                            return
                        }
                        checked ++
                        ast_gc_guard() {
                            var blk = func.body as ExprBlock
                            var pres1 : array<string>
                            var cses1 : array<string>
                            collect_class_names(blk, pres1, cses1)
                            // fixture sanity: the pipeline run must have left one name of each class
                            t |> success(!empty(pres1), "fixture twin must keep a _preshader_ let, got {pres1}")
                            t |> success(!empty(cses1), "fixture twin must keep a _cse_ let, got {cses1}")
                            // a backend-style rewrite: fresh uniform compute + a fresh duplicated varying pair
                            var injU = qmacro_expr(${ let _reentry_u = G_REENTRY * G_REENTRY - 3.0; })
                            var injD1 = qmacro_expr(${ let _reentry_d1 = (x - 2.0) * x; })
                            var injD2 = qmacro_expr(${ let _reentry_d2 = (x - 2.0) * x; })
                            blk.list |> emplace(injU)
                            blk.list |> emplace(injD1)
                            blk.list |> emplace(injD2)
                            let nobarriers : table<string>
                            t |> success(flatten_optimize(func, nobarriers), "re-entered optimize must hoist the injected work")
                            var pres2 : array<string>
                            var cses2 : array<string>
                            collect_class_names(blk, pres2, cses2)
                            t |> success(length(pres2) > length(pres1), "new uniform compute must hoist a fresh _preshader_ let, got {pres2}")
                            t |> success(length(cses2) > length(cses1), "new varying dup must share into a fresh _cse_ let, got {cses2}")
                            t |> success(!has_dup(pres2), "re-entry minted a colliding _preshader_ name: {pres2}")
                            t |> success(!has_dup(cses2), "re-entry minted a colliding _cse_ name: {cses2}")
                        }
                    }
                }
                t |> equal(checked, 1, "expected exactly one reentry_flat twin")
            }
        }
    }
}
