options gen2
options indenting = 4
options strict_smart_pointers = true


module macro_boost shared private

//! Macro helper utilities.
//!
//! Contains `macro_verify` (assertion that produces a macro error and returns
//! an empty expression on failure), `capture_block` (collects captured variables
//! from an expression), and other helpers used by macro authors.

require daslib/ast_boost
require daslib/templates_boost

[macro_function, tag_function(macro_verify_tag), unused_argument(expr, prog, at, message)]
def public macro_verify(expr : bool; prog : ProgramPtr; at : LineInfo; message : string) {
    //! Same as verify, only the check will produce macro error, followed by return [[ExpressionPtr]]
    pass
}

[tag_function_macro(tag="macro_verify_tag")]
class MacroVerifyMacro : AstFunctionAnnotation {
    //! Converts ``macro_verify(expr, message, prog, at)`` to equivalent inline code.
    def override transform(var call : ExprCallFunc?; var errors : das_string) : ExpressionPtr {
        // if ..
        var res = qmacro_block() {
            if (!$e(call.arguments[0])) {
                macro_error($e(call.arguments[1]), $e(call.arguments[2]), $e(call.arguments[3]))
                return null
            }
        }
        res |> force_at(call.at)
        return res
    }
}

[macro]
class CaptureBlock : AstVisitor {
    //! AST visitor that captures variables from a block scope.
    @do_not_delete scope : table<Variable?>
    @do_not_delete captured : table<Variable?; ExprVar?>
    def override preVisitExprVar(expr : ExprVar?) : void {
        if (!(expr.varFlags.local || expr.varFlags.argument || expr.varFlags._block)) return
        let vptr = expr.variable
        if (!(scope |> key_exists(vptr))) {
            unsafe {
                captured[vptr] = reinterpret<ExprVar?>(expr)
            }
        }
    }
    def override preVisitExprLetVariable(expr : ExprLet?; arg : VariablePtr; lastArg : bool) : void {
        scope |> insert(arg)
    }
    def override preVisitExprForVariable(expr : ExprFor?; svar : VariablePtr; last : bool) : void {
        scope |> insert(svar)
    }
    def override preVisitExprBlockArgument(blk : ExprBlock?; arg : VariablePtr; lastArg : bool) : void {
        scope |> insert(arg)
    }
}

struct public CapturedVariable {
    //! Stored captured variable together with the `ExprVar` which uses it
    variable : Variable?    //! captured variable
    expression : ExprVar?   //! expression which uses the variable
    eref : bool             //! this one indicates if its used by reference and does not come from argument. its only used in JIT
}

[macro_function]
def public capture_block(expr : ExpressionPtr) : array<CapturedVariable> {
    //! Collect all captured variables in the expression.
    var astVisitor = new CaptureBlock()
    make_visitor(*astVisitor) $(adapter) {
        visit(expr, adapter)
    }
    var res := [for (k, v in keys(astVisitor.captured), values(astVisitor.captured)); CapturedVariable(variable=k, expression=v) ]
    unsafe {
        delete astVisitor
    }
    return <- res
}

[macro]
class ColletFinally : AstVisitor {
    //! AST visitor that collects finally blocks.
    @do_not_delete blocks : table<ExprBlock?>
    alwaysFor : bool
    def override preVisitExprBlock(blk : ExprBlock?) : void {
        unsafe {
            if (!empty(blk.finalList)) {
                blocks |> insert(reinterpret<ExprBlock?>(blk))
            } elif (alwaysFor && blk.blockFlags.forLoop) {
                blocks |> insert(reinterpret<ExprBlock?>(blk))
            }
        }
    }
    def override canVisitMakeBlockBody(expr : ExprMakeBlock?) : bool {
        return false
    }
}

[macro_function]
def public collect_finally(expr : ExpressionPtr; alwaysFor : bool = false) {
    //! Collect all finally blocks in the expression.
    //! Returns array of ExprBlock? with all the blocks which have `finally` section
    //! Does not go into 'make_block' expression, such as `lambda`, or 'block' expressions
    var astVisitor = new ColletFinally(alwaysFor = alwaysFor)
    make_visitor(*astVisitor) $(adapter) {
        visit(expr, adapter)
    }
    var res := [for (k in keys(astVisitor.blocks)); k ]
    unsafe {
        delete astVisitor
    }
    return <- res
}

[macro]
class ColletLabels : AstVisitor {
    //! AST visitor that collects label statements.
    labels : table<int>
    def override preVisitExprLabel(expr : ExprLabel?) : void {
        labels |> insert(expr.labelName)
    }
    def override canVisitMakeBlockBody(expr : ExprMakeBlock?) : bool {
        return false
    }
}

[macro_function]
def public collect_labels(expr : ExpressionPtr) {
    //! Collect all labels in the expression. Returns array of integer with label indices
    //! Does not go into 'make_block' expression, such as `lambda`, or 'block' expressions
    var astVisitor = new ColletLabels()
    make_visitor(*astVisitor) $(adapter) {
        visit(expr, adapter)
    }
    var res := [for (k in keys(astVisitor.labels)); k ]
    unsafe {
        delete astVisitor
    }
    return <- res
}

[macro_function]
def public has_sideeffects(expr : Expression?) : bool {
    //! Conservative side-effect detection. Returns true when the expression has — or might have — side effects. Returns false ONLY when provably pure (no function calls, no heap allocation, no container mutation).
    if (expr == null || expr.flags.noSideEffects
            || expr is ExprVar
            || expr is ExprConstInt || expr is ExprConstInt8 || expr is ExprConstInt16
            || expr is ExprConstInt64 || expr is ExprConstUInt || expr is ExprConstUInt8
            || expr is ExprConstUInt16 || expr is ExprConstUInt64 || expr is ExprConstFloat
            || expr is ExprConstFloat16
            || expr is ExprConstDouble || expr is ExprConstBool || expr is ExprConstString
            || expr is ExprConstPtr || expr is ExprConstRange || expr is ExprConstURange
            || expr is ExprConstRange64 || expr is ExprConstURange64
            || expr is ExprConstEnumeration || expr is ExprConstBitfield) return false
    // Member access — recurse into operand.
    if (expr is ExprField) return has_sideeffects((expr as ExprField).value)
    if (expr is ExprSafeField) return has_sideeffects((expr as ExprSafeField).value)
    if (expr is ExprSwizzle) return has_sideeffects((expr as ExprSwizzle).value)
    // Pointer / reference artifacts.
    if (expr is ExprRef2Value) return has_sideeffects((expr as ExprRef2Value).subexpr)
    if (expr is ExprRef2Ptr) return has_sideeffects((expr as ExprRef2Ptr).subexpr)
    if (expr is ExprPtr2Ref) return has_sideeffects((expr as ExprPtr2Ref).subexpr)
    if (expr is ExprAddr) return false
    // Type / variant checks.
    if (expr is ExprIs) return has_sideeffects((expr as ExprIs).subexpr)
    if (expr is ExprIsVariant) return has_sideeffects((expr as ExprIsVariant).value)
    if (expr is ExprAsVariant) return has_sideeffects((expr as ExprAsVariant).value)
    if (expr is ExprSafeAsVariant) return has_sideeffects((expr as ExprSafeAsVariant).value)
    // Cast — recurse.
    if (expr is ExprCast) return has_sideeffects((expr as ExprCast).subexpr)
    // Compile-time meta.
    if (expr is ExprTypeInfo || expr is ExprTypeDecl || expr is ExprTag) return false
    // Subscripts.
    if (expr is ExprAt) {
        let at_e = expr as ExprAt
        // tables auto-insert on missing key — unsafe; arrays/strings safe (read-only).
        if (at_e.subexpr == null || at_e.subexpr._type == null
                || at_e.subexpr._type.isGoodTableType) return true
        return has_sideeffects(at_e.subexpr) || has_sideeffects(at_e.index)
    }
    if (expr is ExprSafeAt) {
        let sat = expr as ExprSafeAt
        return has_sideeffects(sat.subexpr) || has_sideeffects(sat.index)
    }
    // Null coalescing.
    if (expr is ExprNullCoalescing) {
        let nc = expr as ExprNullCoalescing
        return has_sideeffects(nc.subexpr) || has_sideeffects(nc.defaultValue)
    }
    // String builder — string heap allocation is no-op by compiler; recurse into operands.
    if (expr is ExprStringBuilder) {
        let sb = expr as ExprStringBuilder
        for (e in sb.elements) {
            if (has_sideeffects(e)) return true
        }
        return false
    }
    // key_exists is a pure container read.
    if (expr is ExprKeyExists) {
        let ke = expr as ExprKeyExists
        for (a in ke.arguments) {
            if (has_sideeffects(a)) return true
        }
        return false
    }
    // Function-call-shaped expressions: ExprCall (regular call) and ExprOp1/ExprOp2/ExprOp3 (operators, which also resolve to a function). Two-layer check:
    if (expr is ExprOp1) {
        let e1 = expr as ExprOp1
        // func != null → trust func flags (catches user overloads); func == null → fall
        // back to op-name allowlist (handles partial-folding artifacts). Mutation ops
        // are unconditionally unsafe (in case a C++ builtin missed the side-effect flag).
        if (is_mutation_op1(e1.op)
                || (e1.func != null && func_has_sideeffects(e1.func))
                || (e1.func == null && !is_safe_op1(e1.op))) return true
        return has_sideeffects(e1.subexpr)
    }
    if (expr is ExprOp2) {
        let e2 = expr as ExprOp2
        if (is_mutation_op2(e2.op) || e2.op == "/" || e2.op == "%"
                || (e2.func != null && func_has_sideeffects(e2.func))
                || (e2.func == null && !is_safe_op2(e2.op))) return true
        return has_sideeffects(e2.left) || has_sideeffects(e2.right)
    }
    if (expr is ExprOp3) {
        let e3 = expr as ExprOp3
        // ExprOp3 is the only ternary `?:` in daslang — pure if operands pure.
        return has_sideeffects(e3.subexpr) || has_sideeffects(e3.left) || has_sideeffects(e3.right)
    }
    if (expr is ExprCall) {
        let ec = expr as ExprCall
        if (func_has_sideeffects(ec.func)) return true
        for (a in ec.arguments) {
            if (has_sideeffects(a)) return true
        }
        return false
    }
    // Default: unknown → unsafe.
    return true
}

[macro_function]
def private func_has_sideeffects(f : Function?) : bool {
    //! True when calling `f` may have side effects. Allowlists builtins (`flags.builtIn`) without `knownSideEffects` or `unsafeOperation`.
    return (f == null || !f.flags.builtIn
        || f.flags.knownSideEffects || f.flags.unsafeOperation)
}

[macro_function]
def private is_safe_op1(op : das_string) : bool {
    //! Unary operators that are pure on workhorse types — no overflow trap, no mutation.
    return op == "-" || op == "!" || op == "~" || op == "+"
}

[macro_function]
def private is_safe_op2(op : das_string) : bool {
    //! Binary operators that are pure on workhorse types. Excludes `/`, `%` (div-by-zero panic — design decision) and all compound-assignment ops (handled by is_mutation_op2).
    return (op == "+" || op == "-" || op == "*"
        || op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">="
        || op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>"
        || op == "&&" || op == "||")
}

[macro_function]
def private is_mutation_op1(op : das_string) : bool {
    //! Unary operators that mutate their operand. Unconditionally unsafe — bypasses any flag check on the resolved builtin (in case the C++ side forgot to mark it).
    return op == "++" || op == "--" || op == "+++" || op == "---"
}

[macro_function]
def private is_mutation_op2(op : das_string) : bool {
    //! Compound-assignment operators (mutate the left operand). Same unconditional-unsafe treatment as is_mutation_op1.
    return (op == "+=" || op == "-=" || op == "*=" || op == "/=" || op == "%="
        || op == "&=" || op == "|=" || op == "^="
        || op == "<<=" || op == ">>="
        || op == "&&=" || op == "||=" || op == "^^=")
}

