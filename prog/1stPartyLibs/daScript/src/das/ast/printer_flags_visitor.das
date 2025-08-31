module printer_flags_visitor

options gen2

options rtti = true
// options log_infer_passes = true
options strict_smart_pointers = false
// options log = true

require rtti
require ast
require daslib/ast_boost
require daslib/strings_boost

class SetPrinterFlags : AstVisitor {
    def override preVisitExprBlockExpression(block1 : smart_ptr<ExprBlock>; var expr : ExpressionPtr) {
        expr.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprNewArgument(call : smart_ptr<ExprNew>; var expr : smart_ptr<Expression>; last : bool) {
        expr.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprCallArgument(casll : smart_ptr<ExprCall>; var expr : smart_ptr<Expression>; last : bool) {
        expr.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprLooksLikeCallArgument(call : smart_ptr<ExprLooksLikeCall>; var expr : smart_ptr<Expression>; last : bool) {
        expr.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprIfThenElse(var expr : smart_ptr<ExprIfThenElse>) {
        expr.cond.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprWhile(var expr : smart_ptr<ExprWhile>) {
        expr.cond.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprReturn(var expr : smart_ptr<ExprReturn>) {
        if (expr.subexpr != null) {
            expr.subexpr.printFlags := ExprPrintFlags.argLevel
        }
    }

    def override preVisitExprCopy(var expr : smart_ptr<ExprCopy>) {
        if (expr.printFlags.topLevel || expr.printFlags.argLevel) {
            expr.right.printFlags := ExprPrintFlags.argLevel
        }
    }

    def override preVisitExprClone(var expr : smart_ptr<ExprClone>) {
        if (expr.printFlags.topLevel || expr.printFlags.argLevel) {
            expr.right.printFlags := ExprPrintFlags.argLevel
        }
    }

    def override preVisitExprVar(var expr : smart_ptr<ExprVar>) {
        expr.printFlags := ExprPrintFlags.bottomLevel
    }

    def override preVisitExprTypeInfo(var expr : smart_ptr<ExprTypeInfo>) {
        if (expr.subexpr != null) {
            expr.subexpr.printFlags := ExprPrintFlags.argLevel
        }
    }

    def override preVisitExprArrayComprehension(var expr : smart_ptr<ExprArrayComprehension>) {
        expr.subexpr.printFlags = ExprPrintFlags.argLevel;
        if (expr.exprWhere != null) {
            expr.exprWhere.printFlags := ExprPrintFlags.argLevel
        }
    }
}

[export]
def used_flags() {
    new SetPrinterFlags();
    panic("unused");
}