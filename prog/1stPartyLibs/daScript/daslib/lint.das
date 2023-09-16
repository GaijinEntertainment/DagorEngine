options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module lint shared private

require daslib/strings_boost
require daslib/ast_boost
require daslib/strings_boost

[lint_macro]
class LintEverything : AstPassMacro
    def override apply ( prog:ProgramPtr; mod:Module? ) : bool
        paranoid(prog, true)
        return true

class LintVisitor : AstVisitor
    astVisitorAdapter : smart_ptr<ast::VisitorAdapter>
    exprForTerminator : array<uint64>
    compile_time_errors : bool
    noLint : bool = false
    def LintVisitor()
        pass
    def lint_error ( text:string; at:LineInfo ) : void
        if noLint
            return
        if compile_time_errors
            compiling_program() |> macro_error(at, text)
        else
            error("{text} at {describe(at)}\n")
    def override preVisitFunction(fun:FunctionPtr) : void
        noLint = false
        for ann in fun.annotations
            if ann.annotation.name=="no_lint"
                noLint = true
                break
    def override visitFunction(var fun:FunctionPtr) : FunctionPtr
        noLint = false
        return <- fun
    def override preVisitExprBlock(blk:smart_ptr<ExprBlock>) : void
        exprForTerminator |> push(0ul)
    def override visitExprBlock(var blk:smart_ptr<ExprBlock>) : ExpressionPtr
        exprForTerminator |> pop()
        return <- blk
    def override visitExprBlockExpression(blk:smart_ptr<ExprBlock>;var expr:ExpressionPtr) : ExpressionPtr
        let lb = exprForTerminator |> back()
        if lb != 0ul
            let eb = intptr(get_ptr(expr))
            if lb != eb
                self->lint_error("unreachable code", expr.at)
        return <- expr
    def override preVisitExprLabel(expr:smart_ptr<ExprLabel>) : void
        exprForTerminator |> pop()
        exprForTerminator |> push(0ul)
    def override preVisitExprReturn(expr:smart_ptr<ExprReturn>) : void
        exprForTerminator |> pop()
        exprForTerminator |> push(intptr(get_ptr(expr)))
    def override preVisitExprCall(expr:smart_ptr<ExprCall>): void
        if expr.name |> eq <| "panic" && expr.func._module.name |> eq <| "$"
            exprForTerminator |> pop()
            exprForTerminator |> push(intptr(get_ptr(expr)))
    def override preVisitExprForVariable(expr:smart_ptr<ExprFor>;v:VariablePtr;last:bool) : void
        validate_var(v, false)

    def validate_var(v: VariablePtr; can_make_const : bool)
        let name = string(v.name)
        if name |> starts_with("__")  // system variables
            return
        if name |> starts_with("_")
            if name |> ends_with("_") // _foo_ - ignore too
                return
            if v.access_flags.access_ref || v.access_flags.access_pass || v.access_flags.access_get || v.init != null
                self->lint_error("variable '{v.name}' is used and should be named without underscode prefix", v.at)
                return
            return
        if v.isAccessUnused
            self->lint_error("unused variable {v.name}: {describe(v._type)} (add an underscore prefix if you really need it)", v.at)
            return
        if !v.access_flags.access_ref && !v.access_flags.access_pass && v.init != null && v.init |> is_expr_const()
            self->lint_error("unused variable {v.name}: {describe(v._type)} (add an underscore prefix if you really need it)", v.at)
            return
        if !v.access_flags.access_ref && !v.access_flags.access_pass && !v.access_flags.access_get && (v.init == null || v.init.flags.noSideEffects)
            self->lint_error("unused variable {v.name}: {describe(v._type)} (add an underscore prefix if you really need it)", v.at)
            return
        if !v.access_flags.access_get && !v.access_flags.access_ref
            let sideEffects = (v.init != null && !v.init.flags.noSideEffects)
            if !sideEffects
                self->lint_error("variable {v.name}: {describe(v._type)} is never used", v.at)
            else
                self->lint_error("variable {v.name}: {describe(v._type)} is never used (be careful, initializer has side effects)", v.at)
            return
        if can_make_const && v._type.baseType != Type tPointer && !v._type.flags.constant && !v._type.flags.removeConstant && !v.access_flags.access_ref// && !v._type.flags.smartPtr
            self->lint_error("variable {v.name}: {describe(v._type)} can be made const (declare with 'let')", v.at)
            return
        if can_make_const && v._type.baseType == Type tPointer && !v._type.flags.constant && !v._type.flags.removeConstant && !(v.access_flags.access_ref || v.access_flags.access_pass)
            self->lint_error("variable {v.name}: {describe(v._type)} can be made const (declare with 'let')", v.at)
            return

    def override preVisitExprLet(expr:smart_ptr<ExprLet>) : void
        for v in expr.variables
            validate_var(v, true)

def public paranoid ( prog:ProgramPtr; compile_time_errors : bool )
    var astVisitor = new [[LintVisitor() compile_time_errors=compile_time_errors]]
    unsafe
        astVisitor.astVisitorAdapter <- make_visitor(*astVisitor)
    visit ( prog, astVisitor.astVisitorAdapter )
    astVisitor.astVisitorAdapter := null
    unsafe
        delete astVisitor
