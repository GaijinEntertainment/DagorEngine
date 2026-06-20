// Stage 0 coverage for the tFixedArray rework (FIXED_ARRAY_REWORK.md).
// Raw AstTypeMacro exercising the typeMacro argument payload in the dedicated
// TypeDecl.typeMacroExpr field:
//   typeMacroExpr[0] — ExprConstString with the macro name
//   typeMacroExpr[1] — type argument (ExprTypeDecl)
//   typeMacroExpr[2] — ExprConstInt (size)
//   typeMacroExpr[3] — ExprConstBool (wrap into an array or not)
//   typeMacroExpr[4] — ExprConstString (mode tag, "arr" expected when wrapping)
// NOTE: this module reads the typeMacroExpr field directly; array results are
// built with make_fixed_array_type.
options gen2
options no_aot

module _typemacro_mod shared private

require daslib/ast
require daslib/ast_boost

// resolves tm_make(type<T>, N, true, "arr") -> T[N]; tm_make(type<T>, N, false, "plain") -> T
[type_macro(name="tm_make")]
class TmMakeTypeMacro : AstTypeMacro {
    def override visit(prog : ProgramPtr; mod : Module?; td : TypeDeclPtr; passT : TypeDeclPtr) : TypeDeclPtr {
        if (length(td.typeMacroExpr) != 5) {
            macro_error(compiling_program(), td.at, "tm_make expects 4 arguments: type, size, wrap, tag")
            return <- TypeDeclPtr()
        }
        if (!(td.typeMacroExpr[2] is ExprConstInt)) {
            macro_error(compiling_program(), td.at, "tm_make: size must be a constant integer")
            return <- TypeDeclPtr()
        }
        if (!(td.typeMacroExpr[3] is ExprConstBool)) {
            macro_error(compiling_program(), td.at, "tm_make: wrap must be a constant bool")
            return <- TypeDeclPtr()
        }
        if (!(td.typeMacroExpr[4] is ExprConstString)) {
            macro_error(compiling_program(), td.at, "tm_make: tag must be a constant string")
            return <- TypeDeclPtr()
        }
        let count = (td.typeMacroExpr[2] as ExprConstInt).value
        let wrap = (td.typeMacroExpr[3] as ExprConstBool).value
        if (wrap && (td.typeMacroExpr[4] as ExprConstString).value != "arr") {
            macro_error(compiling_program(), td.at, "tm_make: wrapping requires the 'arr' tag, got '{(td.typeMacroExpr[4] as ExprConstString).value}'")
            return <- TypeDeclPtr()
        }
        // generic path: type argument not inferred yet — return auto so the compiler retries
        if (td.typeMacroExpr[1]._type == null) {
            var auto_type : TypeDeclPtr
            if (td.typeMacroExpr[1] is ExprTypeDecl) {
                auto_type = clone_type((td.typeMacroExpr[1] as ExprTypeDecl).typeexpr)
            } else {
                auto_type = new TypeDecl(baseType = Type.autoinfer)
            }
            if (wrap) {
                auto_type = make_fixed_array_type(count, auto_type)
            }
            return <- auto_type
        }
        var final_type = clone_type(td.typeMacroExpr[1]._type)
        if (wrap) {
            final_type = make_fixed_array_type(count, final_type)
        }
        return <- final_type
    }
}
