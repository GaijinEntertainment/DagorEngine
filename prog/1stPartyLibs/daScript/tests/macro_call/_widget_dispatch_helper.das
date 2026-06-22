// Helper module replicating the dasImgui [widget] macro pattern in
// isolation — no imgui binding, no GLFW, just enough to exercise:
//   - function_macro injects a `widget_ident : string` param at index 1
//   - call_macro intercepts `<kind>(IDENT, args...)`, auto-emits IDENT as
//     a module global of the state struct's type, and rewrites the call
//     to `<kind>(IDENT, "IDENT", args...)`
//
// If this helper's [widget] dispatch fires correctly while dasImgui's
// equivalent doesn't, the bug is imgui-specific (require chain,
// build order, something about the C++ module). If it ALSO fails here,
// the bug is in the macro logic itself.

options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module _widget_dispatch_helper shared

require daslib/ast public
require daslib/ast_boost
require daslib/templates_boost

struct StubState {
    value : int
    pending : bool
}

def widget_prelude(widget_ident : string) {
    // No-op — real dasImgui uses PushID. For dispatch testing we just need
    // a name to call so the macro-injected `widget_prelude(widget_ident)`
    // resolves at typer time.
}

def widget_finalize(widget_ident : string; kind : string; var state : StubState) {
    state.pending = true
}

[function_macro(name = "widget")]
class WidgetFunctionMacro : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        if (empty(func.arguments)) {
            errors := "[widget] requires at least one parameter (state struct)"
            return false
        }
        let kind = string(func.name)
        let stateArg = func.arguments[0]
        let stateTypeName = describe(stateArg._type)
        // Capture the resolved structure pointer so the call_macro can build
        // a TypeDecl for auto-emitted globals from the actual struct type
        // (cross-module-safe). Type.alias with a qualified name doesn't
        // resolve at the caller's scope.
        if (stateArg._type == null || stateArg._type.structType == null) {
            errors := "[widget] first parameter must be a struct (state)"
            return false
        }
        let stateStruct = stateArg._type.structType
        func.flags |= FunctionFlags.generated
        var widgetIdentVar = new Variable(at = func.at,
            name := "widget_ident",
            _type <- new TypeDecl(baseType = Type.tString, at = func.at))
        widgetIdentVar.flags |= VariableFlags.generated
        func.arguments |> emplace(widgetIdentVar, 1)
        var fblk = new ExprBlock(at = func.body.at)
        fblk.list |> push(qmacro(widget_prelude($i("widget_ident"))))
        let oldBody = func.body as ExprBlock
        for (el in oldBody.list) {
            fblk.list |> push(clone_expression(el))
        }
        for (ef in oldBody.finalList) {
            fblk.finalList |> push(clone_expression(ef))
        }
        func.body = fblk
        var inst = new WidgetCallMacro(
            kind_name = kind,
            state_type_name = stateTypeName,
            state_struct = stateStruct,
            render_fn_name = kind)
        compiling_module() |> add_call_macro(make_call_macro(kind, inst))
        return true
    }
}

class WidgetCallMacro : AstCallMacro {
    kind_name : string
    state_type_name : string
    state_struct : Structure const?
    render_fn_name : string

    def override canVisitArgument(expr : ExprCallMacro?; argIndex : int) : bool {
        return argIndex != 0
    }

    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        if (empty(expr.arguments)) {
            macro_error(prog, expr.at, "{kind_name}(IDENT, ...): missing widget identifier")
            return <- default<ExpressionPtr>
        }
        let identExpr = expr.arguments[0]
        if (!(identExpr is ExprVar)) {
            macro_error(prog, expr.at, "{kind_name}(IDENT, ...): first argument must be an identifier")
            return <- default<ExpressionPtr>
        }
        let identName = string((identExpr as ExprVar).name)
        if (find_variable(mod, identName) == null) {
            // Build the type from the captured Structure pointer so the
            // emitted global resolves cross-module. Type.alias with a
            // qualified name like "_other_mod::Foo" doesn't resolve at the
            // caller's scope.
            var v = new Variable(at = expr.at,
                name := identName,
                _type <- new TypeDecl(baseType = Type.tStructure,
                                      structType = unsafe(reinterpret<Structure?>(state_struct)),
                                      at = expr.at))
            v.flags |= VariableFlags.generated | VariableFlags.private_variable
            mod |> add_variable(v)
        }
        var newCall = new ExprCall(at = expr.at, name := render_fn_name)
        newCall.arguments |> push(new ExprVar(at = expr.at, name := identName))
        newCall.arguments |> push(new ExprConstString(at = expr.at, value := identName))
        // Walk remaining args. Named-tuple literals `(name = val, ...)` carry
        // the call's named-arg ergonomic; daslang's square-bracket form
        // `[name = val]` builds an ExprNamedCall and bypasses call_macro
        // dispatch entirely. Destructure named tuples into positional args
        // (in source order — matches the user-facing parameter order).
        for (i in range(1, length(expr.arguments))) {
            let a = expr.arguments[i]
            if (a is ExprMakeTuple) {
                let mt = a as ExprMakeTuple
                if (length(mt.recordNames) == length(mt.values) && !empty(mt.recordNames)) {
                    for (v in mt.values) {
                        newCall.arguments |> push(clone_expression(v))
                    }
                    continue
                }
            }
            newCall.arguments |> push(clone_expression(a))
        }
        return <- newCall
    }
}
