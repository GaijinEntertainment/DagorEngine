// Helper: declares a [call_macro] `emit_or_reuse_int(name)` that:
//   1. Reads the name string literal out of arguments[0]
//   2. Calls find_variable(mod, name) to check whether the name is
//      already a global in the calling module
//   3. If absent: add_variable() a new int global with default zero-init
//   4. If present: re-use the existing global
//   5. Folds the call to ExprVar(name), so the test reads the int value
//
// This is the auto-emit-gate pattern that the dasImgui [widget] macro's
// auto-emit fix lands. Regression target: if find_variable ever loses
// visibility of just-declared module globals, or add_variable stops
// surfacing emitted globals to the typer at the same call site, the
// downstream [widget] auto-emit silently double-declares.
options gen2
options indenting = 4

module _call_macro_emit_gate_helper public

require daslib/ast public
require daslib/ast_boost

[call_macro(name = "emit_or_reuse_int")]
class private EmitOrReuseInt : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?;
                       var call : ExprCallMacro?) : ExpressionPtr {
        if (length(call.arguments) != 1 || !(call.arguments[0] is ExprConstString)) {
            macro_error(prog, call.at, "emit_or_reuse_int(\"NAME\") expects exactly one string-literal argument")
            return <- default<ExpressionPtr>
        }
        let nameLit = call.arguments[0] as ExprConstString
        let name = string(nameLit.value)

        // Gate: only emit if the global doesn't already exist.
        if (find_variable(mod, name) == null) {
            var v = new Variable(at = call.at,
                name := name,
                _type <- new TypeDecl(baseType = Type.tInt, at = call.at))
            v.flags |= VariableFlags.generated
            mod |> add_variable(v)
        }

        // Either way, hand back a reference to the (now-guaranteed-to-exist)
        // module global. The typer resolves this within the same compile
        // pass — same flow the [widget] auto-emit relies on.
        return <- new ExprVar(at = call.at, name := name)
    }
}
