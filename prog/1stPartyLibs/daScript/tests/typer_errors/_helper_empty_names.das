options gen2

module _helper_empty_names public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost

// Per-fixture macros for tests/typer_errors/failed_invalid_empty_name_*.das.
// Each one constructs ONE AST node carrying an empty `name` string and
// either splices it as a returned expression (call_macro) or adds it to the
// module during structure_macro patch(). The typer's `invalid_empty_name`
// (30296) check fires when it visits the node.
//
// Several fixtures also trigger a downstream typer error (e.g. ExprVar
// empty-name cascades into `30838: can't locate variable ''`); the
// corresponding `failed_*.das` test declares both codes in `expect`.
//
// Validator scope notes:
//   * ExprField + ExprSafeField — NOT covered by `invalid_empty_name`.
//     They keep their dedicated `invalid_field_syntax` (30153) check in
//     `visit(ExprField)` / `visit(ExprSafeField)`, with the parser-oriented
//     "syntax error, expecting field after '.'" message that's accurate
//     for the common `expr.` (dangling-dot) source case.
//   * ExprNamedCall + MakeFieldDecl — fixture blocked because
//     `new MakeStruct()` has no daslang binding. The validator still fires
//     on these node kinds at typer time regardless of how they were built.
//   * Enumeration — `add_enumeration` is not bound into daslang.
//   * EnumEntry — addable via `add_enumeration_entry` but requires
//                 locating an existing Enumeration in `compiling_module()`.
//   * MakeFieldDecl inside ExprMakeVariant — validator covers the path
//     (per-variant walk in `preVisit(ExprMakeVariant)`), no fixture here.

// ===== Expression-level fixtures (call_macro) =====

[call_macro(name = "make_empty_var_ref")]
class private MakeEmptyVarRef : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : ExpressionPtr {
        return new ExprVar(at = call.at, name := "")
    }
}

[call_macro(name = "make_empty_call")]
class private MakeEmptyCall : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : ExpressionPtr {
        return new ExprCall(at = call.at, name := "")
    }
}

// Splices `let _ = (var <empty> : int = 0; ...)` — a let-expression whose
// declared Variable has empty name. Exercises `preVisitLet` validator path.
// PR D3's root cause was an emission of exactly this shape via
// `Foo(field := value)` ctor silently dropping the field.
[call_macro(name = "make_empty_let_var")]
class private MakeEmptyLetVar : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : ExpressionPtr {
        var vlet = new ExprLet(at = call.at, atInit = call.at)
        var vvar <- new Variable(at = call.at, name := "",
                                 _type <- new TypeDecl(baseType = Type.tInt),
                                 init <- new ExprConstInt(at = call.at, value = 0))
        vlet.variables |> emplace_new <| vvar
        return vlet
    }
}

// ===== Declaration-level fixtures (structure_macro) =====
// Applied to a target struct; patch() adds ONE bad declaration into
// compiling_module() so the typer visits and rejects it.

[structure_macro(name = "bad_emit_function")]
class private BadEmitFunction : AstStructureAnnotation {
    def override patch(var st : StructurePtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        var fn = new Function(at = st.at, atDecl = st.at, name := "",
                              result <- new TypeDecl(baseType = Type.tVoid))
        fn.body = new ExprBlock(at = st.at)
        if (!(compiling_module() |> add_function(fn))) {
            errors := "can't add empty-named function"
            return false
        }
        astChanged = true
        return true
    }
}

[structure_macro(name = "bad_emit_structure")]
class private BadEmitStructure : AstStructureAnnotation {
    def override patch(var st : StructurePtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        var bad <- new Structure(at = st.at, name := "")
        if (!(compiling_module() |> add_structure(bad))) {
            errors := "can't add empty-named structure"
            return false
        }
        astChanged = true
        return true
    }
}

[structure_macro(name = "bad_emit_struct_field")]
class private BadEmitStructField : AstStructureAnnotation {
    def override patch(var st : StructurePtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        var bad <- new Structure(at = st.at, name := "BadFieldHost_{st.name}")
        add_structure_field(bad, "", new TypeDecl(baseType = Type.tInt), null)
        if (!(compiling_module() |> add_structure(bad))) {
            errors := "can't add structure with empty field name"
            return false
        }
        astChanged = true
        return true
    }
}
