options gen2
options indenting = 4
options strict_smart_pointers = true

module only_nttp shared private

require daslib/ast
require daslib/ast_boost
require daslib/strings_boost

class OnlyNttpVisitor : AstVisitor {
    offenders : array<string>
    def override visitExprCall(var expr : ExprCall?) : ExpressionPtr {
        if (expr.func != null && expr.func.flags.builtIn && expr.func.flags.callBased && !expr.func.flags.interopFn) {
            if (!expr.func.moreFlags2.nttp) {
                offenders |> push("{expr.func.name} (line {int(expr.at.line)})")
            }
        }
        return expr
    }
}

[function_macro(name="only_nttp")]
class public OnlyNttpAnnotation : AstFunctionAnnotation {
    //! Fails the compile when the annotated function calls an extern not bound with ``addExternInline``.
    def override patch(var func : FunctionPtr; var group : ModuleGroup; var args : AnnotationArgumentList; progArgs : AnnotationArgumentList; var errors : das_string; var astChanged : bool&) : bool {
        var scan = new OnlyNttpVisitor()
        make_visitor(*scan) $(adapter) {
            visit(func, adapter)
        }
        let ok = empty(scan.offenders)
        if (!ok) {
            errors := build_string() $(writer) {
                writer |> write("only_nttp: '{func.name}' calls extern(s) not bound with addExternInline:")
                for (off in scan.offenders) {
                    writer |> write("\n    {off}")
                }
                writer |> write("\n  bind these with addExternInline, or drop [only_nttp]")
            }
        }
        unsafe {
            delete scan
        }
        return ok
    }
}
