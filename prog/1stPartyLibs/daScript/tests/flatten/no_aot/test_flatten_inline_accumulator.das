options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten
require daslib/ast
require daslib/ast_boost
require daslib/rtti
require strings

//! Regression for the inlined-accumulator SSA bug (reported as the `dithering` flatten shader).
//! A `var` accumulator declared inside an INLINED helper and reassigned in a (parallel) loop is
//! uniquified to `__flat_loc_N` by inline_call. ssa_rename_pass must SSA-rename it like a user
//! accumulator — else it stays a self-referential `var __flat_loc_N` whose live `= init` a later
//! DSE strips, leaving a null-init self-ref the shader backend rejects (error[50503] undefined
//! __flat_loc_N). Plain flatten ACCEPTS the self-ref var (it is valid daslang), so a value check
//! alone can't gate this — the structural half asserts the inlined accumulator was SSA-renamed
//! (`__ssa___flat_loc_`), which is present iff ssa_rename processed it.

// `acc` lives in the inlined helper; after inlining it is a `__flat_loc_` accumulator reassigned
// across the unrolled loop. The equality-select mirrors the dithering Bayer accumulator; runtime
// `sel` keeps the selects from const-folding away.
def acc_pick(sel : float) : float {
    var acc = 0.0
    for (i, v in range(4), [0.0, 8.0, 4.0, 12.0]) {
        acc = float(i) == sel ? v : acc
    }
    return acc
}

[flatten]
def inline_acc(sel : float) : float {
    return acc_pick(sel)
}

[test]
def test_flatten_inline_accumulator(t : T?) {
    t |> run("inlined-helper accumulator: flattened twin matches original") @(t : T?) {
        // exact: the accumulator selects a constant per iteration, so no float reordering
        for (sel in [0.0, 1.0, 2.0, 3.0, 9.0]) {
            t |> equal(inline_acc_flat(sel), inline_acc(sel))
        }
    }
    t |> run("inlined accumulator is SSA-renamed (not left a self-ref __flat_loc var)") @(t : T?) {
        let root = get_das_root()
        let path = "{root}/tests/flatten/no_aot/test_flatten_inline_accumulator.das"
        var inscope access <- make_file_access("")
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.threadlock_context = true
                cop.version_2_syntax = true
                compile_file(path, access, unsafe(addr(mg)), cop) $(ok; program; iss) {
                    t |> success(ok, "compile failed: {iss}")
                    if (!ok) {
                        return
                    }
                    var body = ""
                    program_for_each_module(program) $(mod) {
                        for_each_function(mod, "") $(func) {
                            if (func.body != null && !func.flags.builtIn && func.name == "inline_acc_flat") {
                                body = "{describe(func.body)}"
                            }
                        }
                    }
                    t |> success(!empty(body), "inline_acc_flat twin must be produced")
                    t |> success(body |> find("__ssa___flat_loc") >= 0,
                        "inlined accumulator must be SSA-renamed (ssa_rename_pass): {body}")
                }
            }
        }
    }
}
