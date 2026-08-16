options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten
require daslib/ast
require daslib/ast_boost
require daslib/rtti
require strings

//! Targeted test for the scalar-packing pass (`pack_map_reduce`, exposed as `flatten_pack`). It
//! compiles _pack_fixtures.das — whose `[flat_pack]` annotation flattens a twin, runs the PUBLIC
//! `flatten_pack` entry, then folds — and asserts the post-pack body structurally:
//!   • a real min reduction re-rolls into `_pk*` vector vars (the pass FIRED);
//!   • a const-threshold select and an accumulator-dependent lane do NOT (the fail-closed bailout).
//! Structural (not value) so it is tier-independent; the `_pk*` marker also regression-guards the
//! classify_link fail-closed gate — if it ever re-accepts a threshold, `_pk` reappears and this reds.
//!
//! Lives in no_aot/ (compiles daslib/flatten at runtime, pulling macro_boost instantiations that
//! don't AOT-link in the full-tree batch — same reason as test_flatten_fold.das).

// describe() of each `*_packflat` twin produced by compiling `path`, keyed by twin name.
def private pack_twins(t : T?; path : string) : table<string; string> {
    var bodies : table<string; string>
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
                        if (func.body != null && !func.flags.builtIn && func.name |> ends_with("_packflat")) {
                            bodies["{func.name}"] = "{describe(func.body)}"
                        }
                    }
                }
            }
        }
    }
    return <- bodies
}

[test]
def test_flatten_pack(t : T?) {
    let root = get_das_root()
    t |> run("min reduction packs; fail-closed shapes bail clean") @(t : T?) {
        var bodies <- pack_twins(t, "{root}/tests/flatten/no_aot/_pack_fixtures.das")
        // FIRED: the unrolled min spine re-rolls into width-4 vector vars (`_pk*`).
        let mn = bodies?["pk_min4_packflat"] ?? ""
        t |> success(!empty(mn), "pk_min4_packflat twin must be produced")
        t |> success(mn |> find("_pk") >= 0, "min reduction must re-roll into _pk* vector vars: {mn}")
        // BAILED (classification): a const-threshold select is not a min/max reduction.
        let th = bodies?["pk_thresh_packflat"] ?? ""
        t |> success(!empty(th), "pk_thresh_packflat twin must be produced")
        t |> success(th |> find("_pk") < 0, "const-threshold select must NOT pack (fail-closed): {th}")
        // BAILED (independence): a lane reading the accumulator is sequential, not data-parallel.
        let dp = bodies?["pk_dep_packflat"] ?? ""
        t |> success(!empty(dp), "pk_dep_packflat twin must be produced")
        t |> success(dp |> find("_pk") < 0, "accumulator-dependent lane must NOT pack (fail-closed): {dp}")
        delete bodies
    }
}
