module ast_print_flags

//! AST print-flag annotation pass.
//!
//! Visitor that sets ``ExprPrintFlags`` on AST nodes so that
//! ``ast_print`` produces correctly formatted output.

options gen2

options rtti = true
// options log_infer_passes = true
options strict_smart_pointers = false
// options log = true

require daslib/rtti
require daslib/ast
require daslib/ast_boost
require daslib/strings_boost

class SetPrinterFlags : AstVisitor {
    //! AST visitor that sets expression print formatting flags for proper output.
    def override preVisitExprBlockExpression(block1 : ExprBlock?; var expr : ExpressionPtr) {
        //! Sets argLevel print flag on block expressions.
        expr.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprNewArgument(call : ExprNew?; var expr : Expression?; last : bool) {
        //! Sets argLevel print flag on new expression arguments.
        expr.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprCallArgument(casll : ExprCall?; var expr : Expression?; last : bool) {
        //! Sets argLevel print flag on call expression arguments.
        expr.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprLooksLikeCallArgument(call : ExprLooksLikeCall?; var expr : Expression?; last : bool) {
        //! Sets argLevel print flag on looks-like-call arguments.
        expr.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprIfThenElse(var expr : ExprIfThenElse?) {
        //! Sets argLevel print flag on if-then-else condition.
        expr.cond.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprWhile(var expr : ExprWhile?) {
        //! Sets argLevel print flag on while loop condition.
        expr.cond.printFlags := ExprPrintFlags.argLevel
    }

    def override preVisitExprReturn(var expr : ExprReturn?) {
        //! Sets argLevel print flag on return subexpression.
        if (expr.subexpr != null) {
            expr.subexpr.printFlags := ExprPrintFlags.argLevel
        }
    }

    def override preVisitExprCopy(var expr : ExprCopy?) {
        //! Sets argLevel print flag on copy right-hand side when at top or arg level.
        if (expr.printFlags.topLevel || expr.printFlags.argLevel) {
            expr.right.printFlags := ExprPrintFlags.argLevel
        }
    }

    def override preVisitExprClone(var expr : ExprClone?) {
        //! Sets argLevel print flag on clone right-hand side when at top or arg level.
        if (expr.printFlags.topLevel || expr.printFlags.argLevel) {
            expr.right.printFlags := ExprPrintFlags.argLevel
        }
    }

    def override preVisitExprVar(var expr : ExprVar?) {
        //! Sets bottomLevel print flag on variable expressions.
        expr.printFlags := ExprPrintFlags.bottomLevel
    }

    def override preVisitExprTypeInfo(var expr : ExprTypeInfo?) {
        //! Sets argLevel print flag on typeinfo subexpression.
        if (expr.subexpr != null) {
            expr.subexpr.printFlags := ExprPrintFlags.argLevel
        }
    }

    def override preVisitExprArrayComprehension(var expr : ExprArrayComprehension?) {
        //! Sets argLevel print flags on array comprehension subexpressions.
        expr.subexpr.printFlags = ExprPrintFlags.argLevel;
        if (expr.exprWhere != null) {
            expr.exprWhere.printFlags := ExprPrintFlags.argLevel
        }
    }
}

[export]
def used_flags() {
    //! Entry point that instantiates the SetPrinterFlags visitor (never called at runtime).
    new SetPrinterFlags();
    panic("unused");
}