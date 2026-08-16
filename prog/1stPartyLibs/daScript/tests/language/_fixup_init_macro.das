options gen2
options indenting = 4

// Fixture for test_fixup_global_init.das: a function annotation whose fixup() installs a
// CALL-SHAPED global init (the array<uint> literal's to_array_move lowering — the exact shape
// dasSpirv's blob capture uses). Pins the pipeline contract that fixup-set global inits get a
// dirty re-infer (ast_parse.cpp) — without it this fails 50607 ('to_array_move' was not
// resolved during type inference).

module _fixup_init_macro shared public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/rtti

[macro_function]
def private build_uint_array_literal(words : array<uint>; at : LineInfo) : ExpressionPtr {
    var mkArr = new ExprMakeArray(at = at, makeType = new TypeDecl(at = at, baseType = Type.tUInt))
    mkArr.gen2 = true
    for (w in words) {
        mkArr.values |> emplace_new() <| new ExprConstUInt(at = at, value = w)
    }
    var toMove = new ExprCall(at = at, name := "to_array_move")
    toMove.arguments |> emplace(mkArr)
    return toMove
}

[function_macro(name="fixup_init_capture")]
class FixupInitCapture : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        func.flags.exports = true
        let argName = args |> find_arg("name") ?as tString ?? "{func.name}`captured"
        var atype = new TypeDecl(at = func.at, baseType = Type.tArray)
        atype.firstType = new TypeDecl(at = func.at, baseType = Type.tUInt)
        if (!add_global_var(compiling_module(), argName, atype, func.at, false)) {
            errors := "can't add global variable {argName}"
            return false
        }
        return true
    }
    def override fixup(var func : FunctionPtr; var group : ModuleGroup; args, progArgs : AnnotationArgumentList; var errors : das_string) : bool {
        let argName = args |> find_arg("name") ?as tString ?? "{func.name}`captured"
        var glob = find_variable(compiling_module(), argName)
        if (glob != null && glob.init == null && glob.name == argName) {
            // deterministic fixup-derived content: [7, 11, 13] plus the annotated function's arity
            let words <- [7u, 11u, 13u, uint(func.arguments |> length)]
            glob.init = build_uint_array_literal(words, func.at)
        }
        return true
    }
}
