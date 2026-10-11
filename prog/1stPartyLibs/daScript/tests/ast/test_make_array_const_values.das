options gen2
options indenting = 4
options no_aot     // invokes the compiler at runtime (compile_file) — not AOT-linkable

//! ExprMakeArray.constValues: inference marks a literal whose every element is a typed
//! constant of the element type, and leaves a literal with a non-constant element unmarked.
//! The flag is what lets InferTypes::canVisitMakeArray skip re-walking the literal each pass.

require dastest/testing_boost public
require daslib/ast
require daslib/ast_boost

def fixture_path() : string {
    return "{get_das_root()}/tests/ast/_make_array_const_values_fixture.das"
}

// `array<T>(...)` globals init as to_array_move(ExprMakeArray); `fixed_array<T>(...)` as the
// ExprMakeArray itself
def global_make_array(program : ProgramPtr; name : string) : ExprMakeArray? {
    let gv = find_variable(get_this_module(program), name)
    if (gv == null || gv.init == null) {
        return null
    }
    var found = gv.init ?as ExprMakeArray
    if (found == null) {
        let call = gv.init ?as ExprCall
        if (call != null && length(call.arguments) == 1) {
            found = call.arguments[0] ?as ExprMakeArray
        }
    }
    return found
}

[test]
def test_make_array_const_values(t : T?) {
    t |> run("all-constant literals are flagged, a literal with a variable element is not") <| @(t : T?) {
        var inscope access <- make_file_access("")
        using() $(var mg : ModuleGroup) {
            using() $(var cop : CodeOfPolicies) {
                cop.export_all = true
                cop.no_optimizations = true
                cop.no_infer_time_folding = true
                compile_file(fixture_path(), access, unsafe(addr(mg)), cop) $(ok; program; issues) {
                    t |> success(ok, "the fixture did not compile: {issues}")
                    if (!ok || program == null) return
                    let all_const = global_make_array(program, "g_all_const")
                    let mixed = global_make_array(program, "g_mixed")
                    let fixed = global_make_array(program, "g_fixed")
                    t |> success(all_const != null && mixed != null && fixed != null, "the fixture globals are not array literals")
                    if (all_const == null || mixed == null || fixed == null) return
                    t |> success(all_const.constValues, "array<int>(1, 2, 3) is not flagged constValues")
                    t |> success(fixed.constValues, "fixed_array<uint>(1u, 2u, 3u) is not flagged constValues")
                    t |> success(!mixed.constValues, "array<int>(1, g_x, 3) is flagged constValues over a variable element")
                }
            }
        }
    }
}
