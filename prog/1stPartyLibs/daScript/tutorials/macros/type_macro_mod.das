options gen2
options no_aot

// Tutorial macro module: type macro (AstTypeMacro).
//
// AstTypeMacro lets you define custom type expressions that the compiler
// resolves during type inference.  It has a single method:
//   visit(prog : ProgramPtr; mod : Module?; td : TypeDeclPtr; passT : TypeDeclPtr) : TypeDeclPtr
//
// When the compiler sees `padded(type<float>, 4)` in a type position it:
//   1. Parses it into a TypeDecl with baseType = Type.typeMacro
//   2. Stores the arguments in `td.dimExpr`:
//        td.dimExpr[0] — ExprConstString with the macro name ("padded")
//        td.dimExpr[1] — the type argument (ExprTypeDecl wrapping type<float>)
//        td.dimExpr[2] — the size argument (ExprConstInt with value 4)
//   3. Calls visit() so the macro can return the resolved type
//
// The `passT` parameter is non-null only in a generic context — it
// carries the actual argument type being matched against the parameter.
// In a concrete declaration (`var x : padded(type<float>, 4)`) passT
// is null and td.dimExpr[1]._type is already inferred.

module type_macro_mod

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost

[type_macro(name="padded")]
class PaddedTypeMacro : AstTypeMacro {
    //! Type macro that resolves `padded(type<T>, N)` to `T[N]`.
    def override visit(prog : ProgramPtr; mod : Module?; td : TypeDeclPtr; passT : TypeDeclPtr) : TypeDeclPtr {
        // --- argument validation ---
        // dimExpr must have exactly 3 entries: name + type + size.
        if (length(td.dimExpr) != 3) {
            macro_error(compiling_program(), td.at, "padded expects 2 arguments: type and size")
            return <- TypeDeclPtr()
        }
        // The size argument must be a constant integer.
        if (!(td.dimExpr[2] is ExprConstInt)) {
            macro_error(compiling_program(), td.at, "padded: second argument must be a constant integer")
            return <- TypeDeclPtr()
        }
        let count = (td.dimExpr[2] as ExprConstInt).value

        // --- generic path ---
        // When the type argument is not yet inferred (e.g. in a generic
        // parameter like `padded(type<auto(TT)>, 4)`) we return a type
        // with autoinfer so the compiler can match and deduce TT.
        if (td.dimExpr[1]._type == null) {
            var inscope auto_type : TypeDeclPtr
            if (td.dimExpr[1] is ExprTypeDecl) {
                // Clone the unresolved type expression (e.g. auto(TT)).
                auto_type |> move_new <| clone_type((td.dimExpr[1] as ExprTypeDecl).typeexpr)
            } else {
                // Fallback: pure auto-infer.
                auto_type |> move_new <| new TypeDecl(baseType = Type.autoinfer)
            }
            auto_type.dim |> push(count)
            return <- auto_type
        }

        // --- concrete path ---
        // The type argument is fully inferred.  Clone it and add the
        // dimension to produce the final array type (e.g. float[4]).
        var inscope final_type <- clone_type(td.dimExpr[1]._type)
        final_type.dim |> push(count)
        return <- final_type
    }
}
