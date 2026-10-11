options gen2

require dastest/testing_boost
require daslib/ast
require daslib/ast_boost
require daslib/perf_lint
require strings

def fixture_path() : string {
    return "{get_das_root()}/tests/lint/_fixture_dim_expr_folding.das"
}

def compile_under_lint_policies(t : T?; blk : block<(program : smart_ptr<Program>) : void>) {
    var inscope access <- make_file_access("")
    using() $(var mg : ModuleGroup) {
        using() $(var cop : CodeOfPolicies) {
            cop.export_all = true
            cop.lint_check = true
            cop.no_optimizations = true
            cop.no_infer_time_folding = true
            compile_file(fixture_path(), access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                if (!ok) {
                    t |> failure("fixture failed to compile: {issues}")
                    return
                }
                invoke(blk, program)
            }
        }
    }
}

[test]
def test_dimension_expression_folds_without_infer_time_folding(t : T?) {
    t |> run("an array dimension folds even with folding off") @@(t : T?) {
        compile_under_lint_policies(t) $(program) {
            t |> success(program != null, "no program")
        }
    }
}

// The fold is granted per const the compiler demanded a constant of.  A const
// nobody demanded keeps its initializer whole, which is what lint reads.
[test]
def test_an_undemanded_const_keeps_its_initializer(t : T?) {
    t |> run("lint still sees the cast in an undemanded const") @@(t : T?) {
        compile_under_lint_policies(t) $(program) {
            var issues : array<string>
            perf_lint_collect(program, issues)
            var found = false
            for (issue in issues) {
                if (find(issue, "PERF020") >= 0) {
                    found = true
                }
            }
            t |> success(found, "PERF020 was folded away: {issues}")
            delete issues
        }
    }
}

// What a backend asks when it needs a value the policy left unfolded: the answer is the
// constant, and the initializer lint reads is still the arithmetic that was written.
[test]
def test_an_undemanded_const_answers_a_constant(t : T?) {
    t |> run("get_const_expr answers an undemanded const") @@(t : T?) {
        compile_under_lint_policies(t) $(program) {
            var found = false
            // the answer is a node of the asker's own, so the ask runs on its own gc root
            ast_gc_guard() {
                program_for_each_module(program) $(mod) {
                    for_each_global(mod) $(gv) {
                        if (gv.name == "SPAN") {
                            found = true
                            let ci = get_const_expr(program, gv.init) ?as ExprConstInt
                            t |> success(ci != null && ci.value == 12, "SPAN did not answer 12")
                            t |> success(gv.init is ExprOp2, "SPAN's own initializer was folded")
                        }
                    }
                }
            }
            t |> success(found, "SPAN not found")
            ast_gc_guard() {
                program_for_each_module(program) $(mod) {
                    for_each_global(mod) $(gv) {
                        if (gv.name == "DEEP_SUM") {
                            let ci = get_const_expr(program, gv.init) ?as ExprConstInt
                            t |> success(ci != null && ci.value == 40, "DEEP_SUM did not answer 40")
                        }
                        if (gv.name == "VEC_HALF") {
                            let cv = get_const_expr(program, gv.init) ?as ExprConstFloat4
                            t |> success(cv != null && cv.value == float4(0.5, 1.0, 1.5, 2.0),
                                "VEC_HALF did not answer the halved vector")
                        }
                    }
                }
            }
            // a recorded fold is what lint reads as a use: the compiler records its own,
            // and the ask records none, so the pair fails if either side moves
            program_for_each_module(program) $(mod) {
                for_each_global(mod) $(gv) {
                    if (gv.name == "ROWS") {
                        t |> success(gv.access_flags.access_fold, "the dimension fold went unrecorded")
                    }
                    if (gv.name == "STEP") {
                        t |> success(!gv.access_flags.access_fold, "the ask recorded a fold")
                    }
                }
            }
        }
    }
}
