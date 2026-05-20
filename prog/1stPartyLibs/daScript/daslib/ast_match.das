options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true
options stack = 1_048_576

module ast_match shared public

//! AST pattern matching — reverse reification.
//!
//! ``qmatch`` matches an expression against a structural pattern and extracts
//! sub-expressions, constant values, and identifier names using the same tag
//! system as ``qmacro`` but in reverse: ``$e`` clones a matched expression,
//! ``$v`` extracts a constant value, ``$i`` extracts an identifier name.
//!
//! ``ExprRef2Value`` transparency. Post-inference, a read of a ``var`` local
//! can appear wrapped in ``ExprRef2Value(inner)`` — the ref-to-value adapter
//! the typer inserts when a reference is dereferenced for its value. The
//! wrapper is invisible (no surface syntax, no user-visible semantics), so
//! ``generate_match`` peels it on both sides:
//!   - **Pattern side** (compile time): ``ExprRef2Value`` in the pattern is
//!     stripped at the top of ``generate_match`` before dispatch.
//!   - **Source side** (runtime): every dispatch emits a
//!     ``qm_peel_ref2value(actual_var)`` call that strips the wrapper from
//!     the runtime expression before the RTTI guard fires.
//! ``ExprPtr2Ref`` (the ``*foo`` operator) is intentionally NOT peeled —
//! it is user-visible, semantically meaningful, and ``*foo`` should NOT
//! match ``foo``.

require daslib/ast public
require daslib/rtti
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost
require daslib/strings

// ── Result types ────────────────────────────────────────────────────────

//! Error codes for pattern matching failures.
enum QMatchError : int {
    ok                  //! Match succeeded.
    rtti_mismatch       //! Expression RTTI type does not match the pattern.
    field_mismatch      //! A named field (name, op, etc.) differs between pattern and expression.
    const_type_mismatch //! $v extraction failed — expression is not the expected constant type.
    type_mismatch       //! TypeDecl comparison failed — types do not match.
    list_length         //! Block statement count mismatch (after wildcard resolution).
    wildcard_not_found  //! Wildcard could not find a matching statement.
    null_expression     //! A null expression was passed where a non-null was expected.
}

//! Result of a pattern match attempt.
struct QMatchResult {
    matched : bool              //! True if the pattern matched the expression.
    error   : QMatchError       //! Error code describing why the match failed (ok when matched).
    expr    : Expression const? //! Pointer to the expression where matching failed (null on success).
}

// ── Runtime helpers ─────────────────────────────────────────────────────

def qm_run(var expr : Expression?; blk : block<(var arg : Expression?) : QMatchResult>) : QMatchResult {
    //! Invoke a matching block on the expression.
    //! Used internally by the ``qmatch`` macro.
    return invoke(blk, expr)
}

def qm_peel_ref2value(var e : Expression?&) : void {
    //! Strip ``ExprRef2Value`` wrappers from ``e`` in place. The typer inserts
    //! these post-inference around any read of a reference; they are invisible
    //! to user-written code and the matcher peels them so a clean pattern
    //! (``_.X``) matches a wrapped source (``ExprRef2Value(_.X)``) and vice
    //! versa. Emitted at the top of every ``generate_match`` dispatch.
    while (e != null && e is ExprRef2Value) {
        e = (e as ExprRef2Value).subexpr
    }
}

def qm_run_function(var func : FunctionPtr&; blk : block<(var body : Expression?; var func_ptr : Function?) : QMatchResult>) : QMatchResult {
    //! Run matching block on a function, passing both body and Function pointer.
    if (func == null) {
        return qm_fail_null()
    }
    return invoke(blk, func.body, func)
}

def qm_ok() : QMatchResult {
    //! Return a successful match result.
    return QMatchResult(matched = true)
}

def qm_fail_rtti(var expr : Expression?) : QMatchResult {
    //! Return a failure — expression RTTI type does not match the pattern.
    return QMatchResult(error = QMatchError.rtti_mismatch, expr = expr)
}

def qm_fail_field(var expr : Expression?) : QMatchResult {
    //! Return a failure — a named field differs between pattern and expression.
    return QMatchResult(error = QMatchError.field_mismatch, expr = expr)
}

def qm_fail_null() : QMatchResult {
    //! Return a failure — null expression encountered.
    return QMatchResult(error = QMatchError.null_expression)
}

def qm_fail_list(var expr : Expression?) : QMatchResult {
    //! Return a failure — block statement count mismatch.
    return QMatchResult(error = QMatchError.list_length, expr = expr)
}

def qm_fail_const(var expr : Expression?) : QMatchResult {
    //! Return a failure — constant type does not match $v extraction target.
    return QMatchResult(error = QMatchError.const_type_mismatch, expr = expr)
}

def qm_fail_type(var expr : Expression?) : QMatchResult {
    //! Return a failure — TypeDecl comparison failed.
    return QMatchResult(error = QMatchError.type_mismatch, expr = expr)
}

def qm_fail_wildcard(var expr : Expression?) : QMatchResult {
    //! Return a failure — wildcard could not find a matching statement.
    return QMatchResult(error = QMatchError.wildcard_not_found, expr = expr)
}

def qm_scan(var blk_expr : Expression?&; var pos : int&; len : int; matcher : block<(var elem : Expression?) : QMatchResult>) : QMatchResult {
    //! Scan block statements from pos to len, trying matcher at each position.
    //! On first success, update pos to the position after the match and return ok.
    var blk = blk_expr as ExprBlock
    for (scan_pos in range(pos, len)) {
        let r = invoke(matcher, blk.list[scan_pos])
        if (r.matched) {
            pos = scan_pos + 1
            return qm_ok()
        }
    }
    return qm_fail_wildcard(blk_expr)
}

def qm_extract(var expr : Expression?; var val : int&) : QMatchResult {
    //! Extract int from ExprConstInt. Fails if wrong type.
    if (expr is ExprConstInt) {
        val = (expr as ExprConstInt).value
        return qm_ok()
    }
    return qm_fail_const(expr)
}

def qm_extract(var expr : Expression?; var val : float&) : QMatchResult {
    //! Extract float from ExprConstFloat. Fails if wrong type.
    if (expr is ExprConstFloat) {
        val = (expr as ExprConstFloat).value
        return qm_ok()
    }
    return qm_fail_const(expr)
}

def qm_extract(var expr : Expression?; var val : bool&) : QMatchResult {
    //! Extract bool from ExprConstBool. Fails if wrong type.
    if (expr is ExprConstBool) {
        val = (expr as ExprConstBool).value
        return qm_ok()
    }
    return qm_fail_const(expr)
}

def qm_extract(var expr : Expression?; var val : string&) : QMatchResult {
    //! Extract string from ExprConstString. Fails if wrong type.
    if (expr is ExprConstString) {
        val = string((expr as ExprConstString).value)
        return qm_ok()
    }
    return qm_fail_const(expr)
}

def qm_extract(var expr : Expression?; var val : int64&) : QMatchResult {
    //! Extract int64 from ExprConstInt64. Fails if wrong type.
    if (expr is ExprConstInt64) {
        val = (expr as ExprConstInt64).value
        return qm_ok()
    }
    return qm_fail_const(expr)
}

def qm_extract(var expr : Expression?; var val : uint&) : QMatchResult {
    //! Extract uint from ExprConstUInt. Fails if wrong type.
    if (expr is ExprConstUInt) {
        val = (expr as ExprConstUInt).value
        return qm_ok()
    }
    return qm_fail_const(expr)
}

def qm_extract(var expr : Expression?; var val : double&) : QMatchResult {
    //! Extract double from ExprConstDouble. Fails if wrong type.
    if (expr is ExprConstDouble) {
        val = (expr as ExprConstDouble).value
        return qm_ok()
    }
    return qm_fail_const(expr)
}

def qm_extract_name(var expr : Expression?) : string {
    //! Extract the "name" field from common expression types.
    //! Works for ExprVar, ExprCall, ExprField, ExprSafeField, ExprLet variables, etc.
    if (expr is ExprVar) {
        return string((expr as ExprVar).name)
    } elif (expr is ExprField || expr is ExprSafeField) {
        return string((expr as ExprField).name)
    } elif (expr is ExprCall) {
        return string((expr as ExprCall).name)
    } elif (expr is ExprIsVariant || expr is ExprAsVariant || expr is ExprSafeAsVariant) {
        return string((expr as ExprIsVariant).name)
    }
    return ""
}

def qm_count_real_args(args : dasvector`ptr`Variable) : int {
    //! Count arguments excluding fakeContext and fakeLineInfo.
    var count = 0
    for (arg in args) {
        if (arg._type != null) {
            let bt = arg._type.baseType
            if (bt != Type.fakeContext && bt != Type.fakeLineInfo) {
                count ++
            }
        } else {
            count ++
        }
    }
    return count
}

def qm_real_arg_index(args : dasvector`ptr`Variable; logical_idx : int) : int {
    //! Map logical argument index (skipping fakeContext/fakeLineInfo) to physical index.
    var logical = 0
    for (arg, phys in args, range(length(args))) {
        if (arg._type != null) {
            let bt = arg._type.baseType
            if (bt == Type.fakeContext || bt == Type.fakeLineInfo) {
                continue
            }
        }
        if (logical == logical_idx) {
            return phys
        }
        logical ++
    }
    return -1
}

def qm_extract_remaining_args(args : dasvector`ptr`Variable; from_logical : int; var result : array<VariablePtr>&) : void {
    //! Collect remaining real arguments (skipping fakeContext/fakeLineInfo) starting from logical index.
    var logical = 0
    for (arg in args) {
        if (arg._type != null) {
            let bt = arg._type.baseType
            if (bt == Type.fakeContext || bt == Type.fakeLineInfo) {
                continue
            }
        }
        if (logical >= from_logical) {
            {
                var cloned <- clone_variable(arg)
                result |> emplace(cloned)
            }
        }
        logical ++
    }
}

def qm_is_fake_arg(arg : Expression?) : bool {
    //! Returns true if the expression is a compiler-injected fakeContext or fakeLineInfo.
    if (arg._type != null) {
        let bt = arg._type.baseType
        return bt == Type.fakeContext || bt == Type.fakeLineInfo
    }
    return false
}

def qm_count_real_call_args(args : dasvector`ptr`Expression) : int {
    //! Count call arguments excluding fakeContext and fakeLineInfo expressions.
    var count = 0
    for (arg in args) {
        if (!qm_is_fake_arg(arg)) {
            count ++
        }
    }
    return count
}

def qm_real_call_arg_index(args : dasvector`ptr`Expression; logical_idx : int) : int {
    //! Map logical call argument index (skipping fake args) to physical index.
    var logical = 0
    for (arg, phys in args, range(length(args))) {
        if (qm_is_fake_arg(arg)) {
            continue
        }
        if (logical == logical_idx) {
            return phys
        }
        logical ++
    }
    return -1
}

def qm_extract_stmts(var blk_expr : Expression?; from_pos : int; to_pos : int; var result : array<Expression?>&) : void {
    //! Extract statements [from_pos, to_pos) from a block into an array (cloned).
    var blk = blk_expr as ExprBlock
    result |> reserve(to_pos - from_pos)
    for (i in range(from_pos, to_pos)) {
        {
            var cloned = clone_expression(blk.list[i])
            result |> emplace(cloned)
        }
    }
}

def qm_call_name_matches(call : ExprCall?; expected_name : string) : bool {
    //! Check if a call's name matches, accounting for generic instantiation.
    //! When a generic function is instantiated, the call name is mangled
    //! (e.g. ``generic_add`123456``). This checks the mangled name first,
    //! then falls back to the ``fromGeneric`` original name.
    if (call.name == expected_name) {
        return true
    }
    if (call.func != null && call.func.fromGeneric != null) {
        return call.func.fromGeneric.name == expected_name
    }
    return false
}

def qm_extract_call_name(var expr : Expression?) : string {
    //! Extract call name from ExprCall. If the call targets a generic instance,
    //! returns the original generic name (before mangling).
    if (expr is ExprCall) {
        let call = expr as ExprCall
        if (call.func != null && call.func.fromGeneric != null) {
            return string(call.func.fromGeneric.name)
        }
        return string(call.name)
    }
    return ""
}

def qm_extract_field_name(var expr : Expression?) : string {
    //! Extract field name from ExprField, ExprSafeField, or variant expressions.
    if (expr is ExprField || expr is ExprSafeField) {
        return string((expr as ExprField).name)
    } elif (expr is ExprIsVariant || expr is ExprAsVariant || expr is ExprSafeAsVariant) {
        return string((expr as ExprIsVariant).name)
    }
    return ""
}

def qm_extract_type(var expr : Expression?; var out_type : TypeDeclPtr&) : QMatchResult {
    //! Extract TypeDeclPtr from ExprTypeDecl.
    if (!(expr is ExprTypeDecl)) {
        return qm_fail_type(expr)
    }
    out_type  = clone_type((expr as ExprTypeDecl).typeexpr)
    return qm_ok()
}

def qm_rtti(expr : Expression const?) : string {
    //! Return the RTTI type name of an expression.
    return string(expr.__rtti)
}


def qm_make_zero_const(call_name : string) : Expression? {
    //! Create a zero-value ExprConst for the range first component.
    //! range → 0, urange → 0u, range64 → 0l, urange64 → 0ul.
    if (call_name == "range") {
        return new ExprConstInt(value = 0)
    } elif (call_name == "urange") {
        return new ExprConstUInt(value = 0u)
    } elif (call_name == "range64") {
        return new ExprConstInt64(value = 0l)
    } elif (call_name == "urange64") {
        return new ExprConstUInt64(value = 0ul)
    }
    return null
}

def qm_convert_local_function(var expr : Expression?) : Expression? {
    //! Reverse generateLocalFunction: ExprAddr → ExprMakeBlock(isLocalFunction=true).
    //! Returns null if the expression is not a generated local function.
    if (!(expr is ExprAddr)) {
        return null
    }
    let ea = expr as ExprAddr
    if (ea.func == null || !ea.func.flags.generated) {
        return null
    }
    var found_function = false
    peek(ea.func.name) $(fn) {
        found_function = find(fn, "`function") >= 0
    }
    if (!found_function) {
        return null
    }
    let fn = ea.func
    // Reconstruct ExprBlock with args + return type + body
    var blk = new ExprBlock(at = fn.at)
    blk.returnType  = clone_type(fn.result)
    for (arg in fn.arguments) {
        blk.arguments |> emplace_new <| clone_variable(arg)
    }
    // Clone body statements
    var fn_body = clone_expression(fn.body)
    let fn_blk = fn_body as ExprBlock
    if (fn_blk != null) {
        for (stmt in fn_blk.list) {
            blk.list |> emplace_new <| clone_expression(stmt)
        }
    }
    // Wrap in ExprMakeBlock with isLocalFunction flag
    return new ExprMakeBlock(at = fn.at, _block = blk, mmFlags = ExprMakeBlockFlags.isLocalFunction)
}

def qm_resolve_local_function(var expr : Expression?) : Expression? {
    //! If expr is ExprMakeBlock, clone it. If ExprAddr (post-inference local function),
    //! convert back to ExprMakeBlock. Returns null if neither works.
    if (expr is ExprMakeBlock) {
        return clone_to_move(expr)
    }
    return qm_convert_local_function(expr)
}

def qm_convert_comprehension(var expr : Expression?) : Expression? {
    //! Reverse generateComprehension: ExprInvoke(invoke, ExprMakeBlock(closure)) → ExprArrayComprehension.
    //! Detects the pattern by checking ExprReturn.fromComprehension flag.
    //! Returns null if the expression is not a generated comprehension.
    if (!(expr is ExprInvoke)) {
        return null
    }
    let inv = expr as ExprInvoke
    if (length(inv.arguments) != 1) {
        return null
    }
    // The argument should be ExprMakeBlock wrapping the closure
    if (!(inv.arguments[0] is ExprMakeBlock)) {
        return null
    }
    let mkblk = inv.arguments[0] as ExprMakeBlock
    let blk = mkblk._block as ExprBlock
    if (blk == null || length(blk.list) != 3) {
        return null
    }
    // stmt[0] = ExprLet with __acomp_ variable
    // stmt[1] = ExprFor
    // stmt[2] = ExprReturn with fromComprehension=true
    if (!(blk.list[0] is ExprLet) || !(blk.list[1] is ExprFor) || !(blk.list[2] is ExprReturn)) {
        return null
    }
    let ret = blk.list[2] as ExprReturn
    if (!ret.returnFlags.fromComprehension) {
        return null
    }
    // Detect tableSyntax: return subexpr is to_table_move(...)
    var is_table = false
    if (ret.subexpr is ExprCall) {
        let ret_call = ret.subexpr as ExprCall
        if (ret_call.func != null && ret_call.func.fromGeneric != null && ret_call.func.fromGeneric.name == "to_table_move") {
            is_table = true
        }
    }
    // Extract exprFor — clone and clear body (comprehension exprFor has null body)
    var comp_for = clone_expression(blk.list[1])
    (comp_for as ExprFor).body := null
    // Extract subexpr and exprWhere from the for body
    let src_for = blk.list[1] as ExprFor
    let for_body = src_for.body as ExprBlock
    if (for_body == null || length(for_body.list) == 0) {
        return null
    }
    var comp_subexpr : Expression?
    var comp_where : Expression?
    if (for_body.list[0] is ExprIfThenElse) {
        // Has where clause: if (where) { push(acomp, subexpr) }
        let ite = for_body.list[0] as ExprIfThenElse
        comp_where = clone_expression(ite.cond)
        let then_blk = ite.if_true as ExprBlock
        if (then_blk != null && length(then_blk.list) > 0 && then_blk.list[0] is ExprCall) {
            let push_call = then_blk.list[0] as ExprCall
            if (length(push_call.arguments) >= 2) {
                comp_subexpr = clone_expression(push_call.arguments[1])
            }
        }
    } elif (for_body.list[0] is ExprCall) {
        // No where clause: push(acomp, subexpr)
        let push_call = for_body.list[0] as ExprCall
        if (length(push_call.arguments) >= 2) {
            comp_subexpr = clone_expression(push_call.arguments[1])
        }
    }
    if (comp_subexpr == null) {
        return null
    }
    // Reconstruct ExprArrayComprehension
    var result = new ExprArrayComprehension(at = expr.at)
    result.exprFor = clone_expression(comp_for)
    result.subexpr = clone_expression(comp_subexpr)
    if (comp_where != null) {
        result.exprWhere = clone_expression(comp_where)
    }
    result.tableSyntax = is_table
    result.generatorSyntax = false
    return result
}

def qm_resolve_comprehension(var expr : Expression?) : Expression? {
    //! If expr is ExprArrayComprehension, shallow-clone it. If ExprInvoke (post-inference
    //! comprehension), reconstruct ExprArrayComprehension. Returns null if neither works.
    if (expr is ExprArrayComprehension) {
        return clone_to_move(expr)
    }
    return qm_convert_comprehension(expr)
}

// ── Compile-time AST builders ───────────────────────────────────────────

[macro_function]
def private make_is_check(at : LineInfo; var expr : Expression?&; rtti_name : string) : Expression? {
    return new ExprIsVariant(at = at, value = expr, name := rtti_name)
}

[macro_function]
def private make_not(at : LineInfo; var expr : Expression?&) : Expression? {
    return new ExprOp1(at = at, op := "!", subexpr = expr)
}

[macro_function]
def private make_var_ref(at : LineInfo; name : string) : Expression? {
    return new ExprVar(at = at, name := name)
}

[macro_function]
def private qm_guard(at : LineInfo; var cond : Expression?&; var fail_call : Expression?&) : Expression? {
    //! Build: if (cond) { return fail_call }
    var fail_ret = new ExprReturn(at = at, subexpr = fail_call)
    var fail_blk = new ExprBlock(at = at)
    fail_blk.list |> emplace(fail_ret)
    return new ExprIfThenElse(at = at, cond = cond, if_true = fail_blk)
}

[macro_function]
def private qm_rtti_guard(at : LineInfo; actual_var : string; rtti_name : string) : Expression? {
    var cond = qmacro(qm_rtti($i(actual_var)) != $v(rtti_name))
    var fail = qmacro(qm_fail_rtti($i(actual_var)))
    return qm_guard(at, cond, fail)
}

[macro_function]
def private qm_null_guard(at : LineInfo; actual_var : string) : Expression? {
    var cond = qmacro($i(actual_var) == null)
    var fail = qmacro(qm_fail_null())
    return qm_guard(at, cond, fail)
}

[macro_function]
def private next_var(var index : int&) : string {
    let name = "_qm_{index}"
    index++
    return name
}

[macro_function]
def private qm_skip_field(rtti_name : string; field_name : string) : bool {
    //! Returns true if the field should be skipped during pattern matching.
    //! Compiler-internal fields are populated during compilation but null/zero
    //! in raw patterns from qmacro/quote.

    // Fields on all expressions (inherited from Expression base)
    if (field_name == "_type" || field_name == "flags" || field_name == "genFlags"
        || field_name == "printFlags" || field_name == "at") {
        return true
    }
    // Stack/annotation internals
    if (field_name == "stackTop" || field_name == "stackVarBottom" || field_name == "stackVarTop"
        || field_name == "annotationDataSid" || field_name == "annotationData"
        || field_name == "maxLabelIndex" || field_name == "refStackTop"
        || field_name == "initStackSize" || field_name == "argStackTop") {
        return true
    }
    // ExprVar — resolved variable reference
    if (rtti_name == "ExprVar") {
        if (field_name == "variable" || field_name == "argumentIndex" || field_name == "pBlock"
            || field_name == "varFlags") {
            return true
        }
    }
    // ExprField / ExprSafeField / ExprIsVariant / ExprAsVariant / ExprSafeAsVariant —
    // typer-resolved field metadata. The .name string carries the user-visible field
    // identifier and is sufficient for matching; fieldIndex / fieldRef / annotation /
    // derefFlags / fieldFlags are typer caches whose presence diverges between a
    // qmacro pattern (defaults) and a typed source (resolved indices/refs), so they
    // would force every qmatch against typed source to fail.
    if (rtti_name == "ExprField" || rtti_name == "ExprSafeField"
            || rtti_name == "ExprIsVariant" || rtti_name == "ExprAsVariant"
            || rtti_name == "ExprSafeAsVariant") {
        if (field_name == "fieldIndex" || field_name == "fieldRef"
                || field_name == "annotation" || field_name == "atField"
                || field_name == "derefFlags" || field_name == "fieldFlags") {
            return true
        }
    }
    // ExprCall/ExprOp*/ExprLooksLikeCall — resolved function reference
    if (field_name == "func" || field_name == "atEnclosure" || field_name == "stackTop"
        || field_name == "doesNotNeedSp" || field_name == "cmresAlias"
        || field_name == "argumentsFailedToInfer") {
        return true
    }
    // ExprReturn
    if (field_name == "returnInBlock") {
        return true
    }
    return false
}

// ── TypeDecl matching code generation ────────────────────────────────────
// Hardcoded because TypeDecl is not an Expression — no get_expression_annotation.
// Handles $t capture, dimExpr (contains Expressions), and recursive child types.

[macro_function]
def private qm_type_guard(at : LineInfo; actual_var : string; var cond : Expression?&) : Expression? {
    //! Build: if (cond) { return qm_fail_type(expr_var) }
    //! actual_var is used for error context (the original ExpressionPtr)
    var fail = qmacro(qm_fail_type($i(actual_var)))
    return qm_guard(at, cond, fail)
}

[macro_function]
def private generate_type_match(pat_type : TypeDecl?; td_var : string; expr_var : string; var body : array<Expression?>&; var index : int&; at : LineInfo) {
    //! Generate code to match a TypeDecl at runtime.
    //! td_var — name of a TypeDeclPtr variable holding the actual type
    //! expr_var — name of the ExpressionPtr variable for error reporting
    //! pat_type — compile-time pattern TypeDecl

    if (pat_type == null) {
        return
    }

    // ── $t tag — capture TypeDeclPtr ────────────────────────
    if (pat_type.baseType == Type.alias && pat_type.flags.isTag) {
        if (pat_type.firstType != null && length(pat_type.firstType.dimExpr) == 1) {
            let tag_expr = pat_type.firstType.dimExpr[0]
            if (tag_expr != null && tag_expr is ExprVar) {
                let tmp = next_var(index)
                body |> push <| qmacro_expr(${ var $i(tmp) = clone_type($i(td_var)); }) // nolint:LINT004
                body |> push <| qmacro_expr(${ $i((tag_expr as ExprVar).name) = $i(tmp); })
            }
        }
        return
    }

    // ── auto — match any type (wildcard) ─────────────────────
    if (pat_type.baseType == Type.autoinfer) {
        return
    }

    // ── baseType ────────────────────────────────────────────
    let bt_int = int(pat_type.baseType)
    var bt_cond = qmacro(int($i(td_var).baseType) != $v(bt_int))
    body |> push <| qm_type_guard(at, expr_var, bt_cond)

    // ── flags — match structurally relevant flags ────────────
    if (pat_type.flags.constant) {
        var c = qmacro(!$i(td_var).flags.constant)
        body |> push <| qm_type_guard(at, expr_var, c)
    }
    if (pat_type.flags.ref) {
        var c = qmacro(!$i(td_var).flags.ref)
        body |> push <| qm_type_guard(at, expr_var, c)
    }
    if (pat_type.flags.temporary) {
        var c = qmacro(!$i(td_var).flags.temporary)
        body |> push <| qm_type_guard(at, expr_var, c)
    }
    if (pat_type.flags.smartPtr) {
        var c = qmacro(!$i(td_var).flags.smartPtr)
        body |> push <| qm_type_guard(at, expr_var, c)
    }
    if (pat_type.flags.isExplicit) {
        var c = qmacro(!$i(td_var).flags.isExplicit)
        body |> push <| qm_type_guard(at, expr_var, c)
    }

    // ── structType — compare by name ────────────────────────
    if (pat_type.structType != null) {
        var c = qmacro($i(td_var).structType == null || $i(td_var).structType.name != $v(string(pat_type.structType.name)))
        body |> push <| qm_type_guard(at, expr_var, c)
    }

    // ── enumType — compare by name ──────────────────────────
    if (pat_type.enumType != null) {
        var c = qmacro($i(td_var).enumType == null || $i(td_var).enumType.name != $v(string(pat_type.enumType.name)))
        body |> push <| qm_type_guard(at, expr_var, c)
    }

    // ── annotation — compare by name (handled types) ────────
    if (pat_type.annotation != null) {
        var c = qmacro($i(td_var).annotation == null || $i(td_var).annotation.name != $v(string(pat_type.annotation.name)))
        body |> push <| qm_type_guard(at, expr_var, c)
    }

    // ── alias — compare string ──────────────────────────────
    if (!empty(pat_type.alias)) {
        var c = qmacro($i(td_var).alias != $v(string(pat_type.alias)))
        body |> push <| qm_type_guard(at, expr_var, c)
    }

    // ── firstType — recursive ───────────────────────────────
    if (pat_type.firstType != null) {
        let child_var = next_var(index)
        var nc = qmacro($i(td_var).firstType == null)
        body |> push <| qm_type_guard(at, expr_var, nc)
        body |> push <| qmacro_expr(${ var $i(child_var) = clone_type($i(td_var).firstType); })
        generate_type_match(pat_type.firstType, child_var, expr_var, body, index, at)
    }

    // ── secondType — recursive ──────────────────────────────
    if (pat_type.secondType != null) {
        let child_var = next_var(index)
        var nc = qmacro($i(td_var).secondType == null)
        body |> push <| qm_type_guard(at, expr_var, nc)
        body |> push <| qmacro_expr(${ var $i(child_var) = clone_type($i(td_var).secondType); })
        generate_type_match(pat_type.secondType, child_var, expr_var, body, index, at)
    }

    // ── argTypes — element-wise recursive ────────────────────
    let n_args = length(pat_type.argTypes)
    if (n_args > 0) {
        var lc = qmacro(length($i(td_var).argTypes) != $v(n_args))
        body |> push <| qm_type_guard(at, expr_var, lc)
        for (arg, i in pat_type.argTypes, range(n_args)) {
            let pat_arg = arg
            if (pat_arg != null) {
                let child_var = next_var(index)
                body |> push <| qmacro_expr(${ var $i(child_var) = clone_type($i(td_var).argTypes[$v(i)]); })
                generate_type_match(pat_arg, child_var, expr_var, body, index, at)
            }
        }
    }

    // ── dim — compare fixed dimensions ──────────────────────
    let n_dim = length(pat_type.dim)
    if (n_dim > 0) {
        var dc = qmacro(length($i(td_var).dim) != $v(n_dim))
        body |> push <| qm_type_guard(at, expr_var, dc)
        for (d, i in pat_type.dim, range(n_dim)) {
            {
                var c = qmacro($i(td_var).dim[$v(i)] != $v(d))
                body |> push <| qm_type_guard(at, expr_var, c)
            }
        }
    }

    // ── dimExpr — these are Expressions, use generate_match ──
    let n_dimexpr = length(pat_type.dimExpr)
    if (n_dimexpr > 0) {
        var dc = qmacro(length($i(td_var).dimExpr) != $v(n_dimexpr))
        body |> push <| qm_type_guard(at, expr_var, dc)
        for (dexpr, i in pat_type.dimExpr, range(n_dimexpr)) {
            let pat_expr = dexpr
            if (pat_expr != null) {
                let child_var = next_var(index)
                body |> push <| qmacro_expr(${ var $i(child_var) = clone_expression($i(td_var).dimExpr[$v(i)]); })
                generate_match(pat_expr, child_var, body, index, at)
            }
        }
    }
}

// ── Argument and return type matching ────────────────────────────────────

[macro_function]
def private generate_pattern_args_match(var pat_block : ExprBlock?; target_var : string; ret_field : string; expr_var : string; var body : array<Expression?>&; var index : int&; at : LineInfo) {
    //! Generate code to match argument names, types, and return type.
    //! target_var — runtime variable with .arguments and return type field
    //! ret_field — "returnType" for ExprBlock, "result" for Function
    //! Arguments always match strictly — 0 pattern args means 0 actual args.
    //! Use $a(var) to capture remaining arguments beyond the fixed ones.
    //! If pattern has no return type (null), skip return type check.

    let n_args = length(pat_block.arguments)
    if (n_args > 0) {
        // Detect $a(var) tag on the last argument — captures remaining args
        var has_a_tag = false
        var a_tag_vname = ""
        {
            let last_arg = pat_block.arguments[n_args - 1]
            if (last_arg._type != null && last_arg._type.flags.isTag && last_arg._type.baseType == Type.none) {
                has_a_tag = true
                if (last_arg.init != null && last_arg.init is ExprVar) {
                    a_tag_vname = string((last_arg.init as ExprVar).name)
                }
            }
        }
        let n_fixed = has_a_tag ? n_args - 1 : n_args

        // Check argument count (excluding fakeContext/fakeLineInfo)
        // With $a: at least n_fixed args. Without: exactly n_args.
        if (has_a_tag) {
            var cond = qmacro(qm_count_real_args($i(target_var).arguments) < $v(n_fixed))
            var fail = qmacro(qm_fail_list($i(expr_var)))
            body |> push <| qm_guard(at, cond, fail)
        } else {
            var cond = qmacro(qm_count_real_args($i(target_var).arguments) != $v(n_args))
            var fail = qmacro(qm_fail_list($i(expr_var)))
            body |> push <| qm_guard(at, cond, fail)
        }

        // Match each fixed argument's name and type (using real arg index mapping)
        for (arg, i in pat_block.arguments, range(n_fixed)) {
            {
                var pat_arg = arg
                let arg_name = string(pat_arg.name)

                // Get physical index at runtime (skipping fake args)
                let phys_var = next_var(index)
                body |> push <| qmacro_expr(${ var $i(phys_var) = qm_real_arg_index($i(target_var).arguments, $v(i)); })

                // Name matching
                if (!empty(arg_name)) {
                    var nc = qmacro($i(target_var).arguments[$i(phys_var)].name != $v(arg_name))
                    var nf = qmacro(qm_fail_field($i(expr_var)))
                    body |> push <| qm_guard(at, nc, nf)
                }

                // Type matching — if argument has a type, match it
                if (pat_arg._type != null) {
                    var pat_td = pat_arg._type
                    // Skip if type is autoinfer (unspecified)
                    if (pat_td.baseType != Type.autoinfer) {
                        let td_var = next_var(index)
                        body |> push <| qmacro_expr(${ var $i(td_var) = clone_type($i(target_var).arguments[$i(phys_var)]._type); })
                        generate_type_match(pat_td, td_var, expr_var, body, index, at)
                    }
                }
            }
        }

        // $a(var) — capture remaining arguments
        if (has_a_tag && !empty(a_tag_vname)) {
            body |> push <| qmacro_expr(${ qm_extract_remaining_args($i(target_var).arguments, $v(n_fixed), $i(a_tag_vname)); })
        }
    } else {
        // No pattern arguments — require target has 0 real args
        var cond = qmacro(qm_count_real_args($i(target_var).arguments) != 0)
        var fail = qmacro(qm_fail_list($i(expr_var)))
        body |> push <| qm_guard(at, cond, fail)
    }

    // Return type matching
    if (pat_block.returnType != null) {
        var pat_ret = pat_block.returnType
        // Skip if return type is autoinfer or void (unspecified)
        if (pat_ret.baseType != Type.autoinfer && pat_ret.baseType != Type.tVoid) {
            let ret_var = next_var(index)
            body |> push <| qmacro_expr(${ var $i(ret_var) = clone_type($i(target_var).$f(ret_field)); })
            generate_type_match(pat_ret, ret_var, expr_var, body, index, at)
        }
    }
}

[macro_function]
def private is_wildcard_call(pattern : Expression?) : bool {
    if (!(pattern is ExprCall)) {
        return false
    }
    let call = pattern as ExprCall
    return call.name == "_wildcard" || call.name == "_wildcard1" || call.name == "_optional" || call.name == "_any"
}

[macro_function]
def private get_wildcard_b_tag(pattern : Expression?) : string {
    //! If wildcard has a $b(var) argument, return the variable name. Otherwise "".
    if (!(pattern is ExprCall)) {
        return ""
    }
    let call = pattern as ExprCall
    if (length(call.arguments) == 0) {
        return ""
    }
    let arg = call.arguments[0]
    if (arg is ExprTag) {
        let tag = arg as ExprTag
        if (tag.name == "b") {
            let var_expr = tag.subexpr as ExprVar
            if (var_expr != null) {
                return string(var_expr.name)
            }
        }
    }
    return ""
}

// ── Core code generator ─────────────────────────────────────────────────
// Generated code uses var (not let) for all variables so as-cast returns
// non-const and field access gives non-const Expression?.

// Map from constructor call name to the ExprConst* RTTI name that constant folding produces.
// Returns "" if the call is not a known constant constructor.
[macro_function]
def private const_constructor_rtti(call_name : string) : string {
    if (call_name == "range") { return "ExprConstRange"; }
    if (call_name == "urange") { return "ExprConstURange"; }
    if (call_name == "range64") { return "ExprConstRange64"; }
    if (call_name == "urange64") { return "ExprConstURange64"; }
    if (call_name == "int2") { return "ExprConstInt2"; }
    if (call_name == "int3") { return "ExprConstInt3"; }
    if (call_name == "int4") { return "ExprConstInt4"; }
    if (call_name == "uint2") { return "ExprConstUInt2"; }
    if (call_name == "uint3") { return "ExprConstUInt3"; }
    if (call_name == "uint4") { return "ExprConstUInt4"; }
    if (call_name == "float2") { return "ExprConstFloat2"; }
    if (call_name == "float3") { return "ExprConstFloat3"; }
    if (call_name == "float4") { return "ExprConstFloat4"; }
    return ""
}

// Map folded ExprConst RTTI to the scalar ExprConst RTTI for component extraction.
[macro_function]
def private const_component_expr_rtti(folded_rtti : string) : string {
    if (folded_rtti == "ExprConstRange" || folded_rtti == "ExprConstInt2"
        || folded_rtti == "ExprConstInt3" || folded_rtti == "ExprConstInt4") {
        return "ExprConstInt"
    }
    if (folded_rtti == "ExprConstURange" || folded_rtti == "ExprConstUInt2"
        || folded_rtti == "ExprConstUInt3" || folded_rtti == "ExprConstUInt4") {
        return "ExprConstUInt"
    }
    if (folded_rtti == "ExprConstRange64") { return "ExprConstInt64"; }
    if (folded_rtti == "ExprConstURange64") { return "ExprConstUInt64"; }
    if (folded_rtti == "ExprConstFloat2" || folded_rtti == "ExprConstFloat3"
        || folded_rtti == "ExprConstFloat4") {
        return "ExprConstFloat"
    }
    return ""
}

// Returns true if the constant constructor is a range type (range, urange, range64, urange64).
// Range one-value form: range(y) = range(0, y). Vector one-value form: float2(v) = float2(v, v).
[macro_function]
def private is_range_constructor(call_name : string) : bool {
    return call_name == "range" || call_name == "urange" || call_name == "range64" || call_name == "urange64"
}

// Component field names for each vector/range ExprConst type.
[macro_function]
def private const_component_fields(folded_rtti : string) : array<string> {
    if (folded_rtti == "ExprConstRange" || folded_rtti == "ExprConstURange"
        || folded_rtti == "ExprConstRange64" || folded_rtti == "ExprConstURange64") {
        return <- array<string>("x", "y")
    }
    if (folded_rtti |> ends_with("2")) { return <- array<string>("x", "y"); }
    if (folded_rtti |> ends_with("3")) { return <- array<string>("x", "y", "z"); }
    if (folded_rtti |> ends_with("4")) { return <- array<string>("x", "y", "z", "w"); }
    return <- array<string>()
}

// Check if any argument in an ExprCall contains a tag expression.
[macro_function]
def private has_tags_in_args(call : ExprCall?) : bool {
    // Top-level check only — tags nested inside expressions (e.g. 1.0 + $v(x))
    // would prevent constant folding, so the call stays as ExprCall and hits the
    // normal matching path instead of reaching here.
    for (arg in call.arguments) {
        if (arg != null && arg is ExprTag) {
            return true
        }
    }
    return false
}

[macro_function]
def private qm_const_value_guard(at : LineInfo; cast_var : string; actual_var : string; var cond : Expression?&; var body : array<Expression?>&) {
    var fail = qmacro(qm_fail_field($i(actual_var)))
    body |> push <| qm_guard(at, cond, fail)
}

[macro_function]
def private generate_synth_component(comp_rtti : string; synth_var : string; cast_var : string; field_name : string; var body : array<Expression?>&; at : LineInfo) {
    //! Synthesize a ``var synth = new ExprConstXxx(value = cast_var.value.field)``
    //! so that the component can be fed through generate_match for tag handling.
    if (comp_rtti == "ExprConstInt") {
        body |> push <| qmacro_expr(${ var $i(synth_var) = new ExprConstInt(value = $i(cast_var).value.$f(field_name)); })
    } elif (comp_rtti == "ExprConstUInt") {
        body |> push <| qmacro_expr(${ var $i(synth_var) = new ExprConstUInt(value = $i(cast_var).value.$f(field_name)); })
    } elif (comp_rtti == "ExprConstFloat") {
        body |> push <| qmacro_expr(${ var $i(synth_var) = new ExprConstFloat(value = $i(cast_var).value.$f(field_name)); })
    } elif (comp_rtti == "ExprConstInt64") {
        body |> push <| qmacro_expr(${ var $i(synth_var) = new ExprConstInt64(value = $i(cast_var).value.$f(field_name)); })
    } elif (comp_rtti == "ExprConstUInt64") {
        body |> push <| qmacro_expr(${ var $i(synth_var) = new ExprConstUInt64(value = $i(cast_var).value.$f(field_name)); })
    }
}

[macro_function]
def private generate_const_constructor_match(call : ExprCall?; folded_rtti : string; actual_var : string; var body : array<Expression?>&; var index : int&; at : LineInfo) {
    //! Handle constant constructor calls (range, int2, float3, etc.) that may be constant-folded.
    //! The pattern is an ExprCall like ``range(1, 10)`` but the compiled AST may have
    //! ``ExprConstRange`` instead. Clone the pattern call expression and compare the folded
    //! constant's value against it directly — the compiler will fold the clone too.
    body |> push <| qm_rtti_guard(at, actual_var, folded_rtti)
    let cast_var = next_var(index)
    var as_val = make_var_ref(at, actual_var)
    var as_cast = new ExprAsVariant(at = at, value = as_val, name := folded_rtti)
    body |> push <| qmacro_expr(${ var $i(cast_var) = $e(as_cast); })
    if (!has_tags_in_args(call)) {
        // Fast path: all args are constants — clone the call, compiler folds it, compare with !=
        var pat_clone = clone_expression(unsafe(reinterpret<Expression?>(call)))
        let pat_var = next_var(index)
        body |> push <| qmacro_expr(${ var $i(pat_var) = $e(pat_clone); })
        var cond = qmacro($i(cast_var).value != $i(pat_var))
        qm_const_value_guard(at, cast_var, actual_var, cond, body)
    } else {
        // Slow path: some args have tags ($v, $e, etc.) — synthesize per-component ExprConst
        // expressions from the folded value and match each arg individually
        let comp_rtti = const_component_expr_rtti(folded_rtti)
        var inscope fields <- const_component_fields(folded_rtti)
        for (arg, field_name in call.arguments, fields) {
            let child = arg
            if (child == null) {
                continue
            }
            let synth_var = next_var(index)
            generate_synth_component(comp_rtti, synth_var, cast_var, field_name, body, at)
            generate_match(child, synth_var, body, index, at)
        }
    }
}

[macro_function]
def private cm_helper(val : auto; cast_var, actual_var : string; var body : array<Expression?>&; at : LineInfo) {
    //! Generate: if (cast_var.value != val) { return fail }
    var cond = qmacro($i(cast_var).value != $v(val))
    qm_const_value_guard(at, cast_var, actual_var, cond, body)
}

[macro_function]
def private generate_const_match(pattern : Expression?; rtti : string; actual_var : string; var body : array<Expression?>&; var index : int&; at : LineInfo) {
    //! Generate matching code for ExprConst* nodes — RTTI guard + value comparison.
    body |> push <| qm_rtti_guard(at, actual_var, rtti)
    let cast_var = next_var(index)
    var as_val = make_var_ref(at, actual_var)
    var as_cast = new ExprAsVariant(at = at, value = as_val, name := rtti)
    body |> push <| qmacro_expr(${ var $i(cast_var) = $e(as_cast); })
    if (rtti == "ExprConstInt") { cm_helper((pattern as ExprConstInt).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstFloat") { cm_helper((pattern as ExprConstFloat).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstBool") { cm_helper((pattern as ExprConstBool).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstString") { cm_helper(string((pattern as ExprConstString).value), cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstInt64") { cm_helper((pattern as ExprConstInt64).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstUInt") { cm_helper((pattern as ExprConstUInt).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstDouble") { cm_helper((pattern as ExprConstDouble).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstUInt64") { cm_helper((pattern as ExprConstUInt64).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstInt8") { cm_helper((pattern as ExprConstInt8).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstUInt8") { cm_helper((pattern as ExprConstUInt8).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstInt16") { cm_helper((pattern as ExprConstInt16).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstUInt16") { cm_helper((pattern as ExprConstUInt16).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstEnumeration") { cm_helper(string((pattern as ExprConstEnumeration).value), cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstBitfield") { cm_helper((pattern as ExprConstBitfield).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstPtr") { pass; } // null — RTTI match is sufficient
    elif (rtti == "ExprConstRange") { cm_helper((pattern as ExprConstRange).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstURange") { cm_helper((pattern as ExprConstURange).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstRange64") { cm_helper((pattern as ExprConstRange64).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstURange64") { cm_helper((pattern as ExprConstURange64).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstInt2") { cm_helper((pattern as ExprConstInt2).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstInt3") { cm_helper((pattern as ExprConstInt3).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstInt4") { cm_helper((pattern as ExprConstInt4).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstUInt2") { cm_helper((pattern as ExprConstUInt2).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstUInt3") { cm_helper((pattern as ExprConstUInt3).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstUInt4") { cm_helper((pattern as ExprConstUInt4).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstFloat2") { cm_helper((pattern as ExprConstFloat2).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstFloat3") { cm_helper((pattern as ExprConstFloat3).value, cast_var, actual_var, body, at); }
    elif (rtti == "ExprConstFloat4") { cm_helper((pattern as ExprConstFloat4).value, cast_var, actual_var, body, at); }
}

[macro_function]
def private generate_match(pattern : Expression?; actual_var : string; var body : array<Expression?>&; var index : int&; at : LineInfo) {
    if (pattern == null) {
        return
    }

    // Pattern-side peel: ExprRef2Value is a typer artifact, not user-visible.
    // Descend into the wrapped expression and discard the wrapper. Recursive
    // call handles nested wrappers naturally.
    if (pattern is ExprRef2Value) {
        generate_match((pattern as ExprRef2Value).subexpr, actual_var, body, index, at)
        return
    }

    // Source-side peel: emit a runtime call that strips ExprRef2Value from
    // the actual expression before any RTTI/field guards fire.
    body |> push <| qmacro_expr(${ qm_peel_ref2value($i(actual_var)); })

    let rtti = string(pattern.__rtti)

    // ── ExprTag — captures ($e, $v, $i, $t, $c, $f) ────────
    if (pattern is ExprTag) {
        let tag = pattern as ExprTag
        if (tag.name == "e" || tag.name == "") {
            let var_expr = tag.subexpr as ExprVar
            if (var_expr != null) {
                let tmp = next_var(index)
                body |> push <| qmacro_expr(${ var $i(tmp) = clone_expression($i(actual_var)); }) // nolint:LINT004
                body |> push <| qmacro_expr(${ $i(var_expr.name) <- $i(tmp); })
            }
            return
        } elif (tag.name == "v") {
            // $v(var) — extract constant value. Overload resolved by variable type.
            // Generates: var _qm_r = qm_extract(actual, vname); if (!_qm_r.matched) return _qm_r
            let var_expr = tag.subexpr as ExprVar
            if (var_expr != null) {
                let r_var = next_var(index)
                body |> push <| qmacro_expr(${ var $i(r_var) = qm_extract($i(actual_var), $i(var_expr.name)); })
                var cond = qmacro($i(r_var).matched == false)
                var fail = make_var_ref(at, r_var)
                body |> push <| qm_guard(at, cond, fail)
            }
            return
        } elif (tag.name == "i") {
            let var_expr = tag.subexpr as ExprVar
            if (var_expr != null) {
                // Check if $i wraps a declaration (ExprLet, ExprFor, ExprAddr)
                if (tag.value != null) {
                    let wrapped = tag.value
                    if (wrapped is ExprLet) {
                        // var $i(name) = expr — match ExprLet, capture variable name
                        // Clone the wrapped expression so we don't modify the pattern AST
                        var wrapped_clone = clone_expression(tag.value)
                        var wrapped_clone_let = wrapped_clone as ExprLet
                        if (length(wrapped_clone_let.variables) > 0) {
                            wrapped_clone_let.variables[0].name := ""
                        }
                        // Generate ExprLet matching for the cloned expression
                        generate_match(wrapped_clone, actual_var, body, index, at)
                        // After matching, extract variable name
                        let cast_let = next_var(index)
                        var asv = make_var_ref(at, actual_var)
                        var asc = new ExprAsVariant(at = at, value = asv, name := "ExprLet")
                        body |> push <| qmacro_expr(${ var $i(cast_let) = $e(asc); })
                        body |> push <| qmacro_expr(${ $i(var_expr.name) = string($i(cast_let).variables[0].name); })
                    } else {
                        // Other wrapped types — just match and extract name
                        generate_match(wrapped, actual_var, body, index, at)
                        body |> push <| qmacro_expr(${ $i(var_expr.name) = qm_extract_name($i(actual_var)); })
                    }
                } else {
                    // Standalone $i(name) — extract name from any expression
                    body |> push <| qmacro_expr(${ $i(var_expr.name) = qm_extract_name($i(actual_var)); })
                }
            }
            return
        } elif (tag.name == "t") {
            // $t(var) — extract TypeDeclPtr from ExprTypeDecl
            let var_expr = tag.subexpr as ExprVar
            if (var_expr != null) {
                let r_var = next_var(index)
                body |> push <| qmacro_expr(${ var $i(r_var) = qm_extract_type($i(actual_var), $i(var_expr.name)); })
                var cond = qmacro($i(r_var).matched == false)
                var fail = make_var_ref(at, r_var)
                body |> push <| qm_guard(at, cond, fail)
            }
            return
        } elif (tag.name == "c") {
            // $c(var)(args) — match call arguments, capture call name
            let var_expr = tag.subexpr as ExprVar
            if (var_expr != null) {
                if (tag.value != null) {
                    // Clone wrapped ExprCall, clear placeholder name so field match skips it
                    var wrapped_clone = clone_expression(tag.value)
                    var wrapped_call = wrapped_clone as ExprCall
                    wrapped_call.name := ""
                    generate_match(wrapped_clone, actual_var, body, index, at)
                }
                // Extract call name
                body |> push <| qmacro_expr(${ $i(var_expr.name) = qm_extract_call_name($i(actual_var)); })
            }
            return
        } elif (tag.name == "f") {
            // a.$f(var) — match field value, capture field name
            let var_expr = tag.subexpr as ExprVar
            if (var_expr != null) {
                if (tag.value != null) {
                    // Clone wrapped ExprField, clear placeholder name so field match skips it
                    var wrapped_clone = clone_expression(tag.value)
                    if (wrapped_clone is ExprField) {
                        var wf = wrapped_clone as ExprField
                        wf.name := ""
                    } elif (wrapped_clone is ExprSafeField) {
                        var wf = wrapped_clone as ExprSafeField
                        wf.name := ""
                    }
                    generate_match(wrapped_clone, actual_var, body, index, at)
                }
                // Extract field name
                body |> push <| qmacro_expr(${ $i(var_expr.name) = qm_extract_field_name($i(actual_var)); })
            }
            return
        }
        return
    }

    // ── _any() wildcard ────────────────────────────────────────
    if (is_wildcard_call(pattern)) {
        return
    }

    // ── ExprTypeDecl — type matching with generate_type_match ──
    if (pattern is ExprTypeDecl) {
        body |> push <| qm_rtti_guard(at, actual_var, "ExprTypeDecl")
        let cast_var = next_var(index)
        var as_val = make_var_ref(at, actual_var)
        var as_cast = new ExprAsVariant(at = at, value = as_val, name := "ExprTypeDecl")
        body |> push <| qmacro_expr(${ var $i(cast_var) = $e(as_cast); }) // nolint:LINT004
        // Clone the typeexpr for matching
        let td_var = next_var(index)
        body |> push <| qmacro_expr(${ var $i(td_var) = clone_type($i(cast_var).typeexpr); }) // nolint:LINT004
        generate_type_match((pattern as ExprTypeDecl).typeexpr, td_var, actual_var, body, index, at)
        return
    }

    // ── ExprMakeBlock — lightweight inline block matching ───────
    if (pattern is ExprMakeBlock) {
        let pat_mkblk = pattern as ExprMakeBlock
        // For local function patterns, also accept ExprAddr (post-inference form)
        var eff_actual = actual_var
        if (pat_mkblk.mmFlags.isLocalFunction) {
            let eff_var = next_var(index)
            body |> push <| qmacro_expr(${ var $i(eff_var) = qm_resolve_local_function($i(actual_var)); })
            var nc = qmacro($i(eff_var) == null)
            var nf = qmacro(qm_fail_rtti($i(actual_var)))
            body |> push <| qm_guard(at, nc, nf)
            eff_actual = eff_var
        } else {
            body |> push <| qm_rtti_guard(at, actual_var, "ExprMakeBlock")
        }
        // Match mmFlags (isLambda, isLocalFunction) to distinguish block/lambda/local-function
        let mkblk_cast = next_var(index)
        var as_val_mkb = make_var_ref(at, eff_actual)
        var as_cast_mkb = new ExprAsVariant(at = at, value = as_val_mkb, name := "ExprMakeBlock")
        body |> push <| qmacro_expr(${ var $i(mkblk_cast) = $e(as_cast_mkb); })
        if (pat_mkblk.mmFlags.isLambda) {
            var cl = qmacro(!$i(mkblk_cast).mmFlags.isLambda)
            var fl = qmacro(qm_fail_field($i(actual_var)))
            body |> push <| qm_guard(at, cl, fl)
        } else {
            var cl = qmacro($i(mkblk_cast).mmFlags.isLambda)
            var fl = qmacro(qm_fail_field($i(actual_var)))
            body |> push <| qm_guard(at, cl, fl)
        }
        if (pat_mkblk.mmFlags.isLocalFunction) {
            var clf = qmacro(!$i(mkblk_cast).mmFlags.isLocalFunction)
            var flf = qmacro(qm_fail_field($i(actual_var)))
            body |> push <| qm_guard(at, clf, flf)
        } else {
            var clf = qmacro($i(mkblk_cast).mmFlags.isLocalFunction)
            var flf = qmacro(qm_fail_field($i(actual_var)))
            body |> push <| qm_guard(at, clf, flf)
        }
        var pat_block = pat_mkblk._block as ExprBlock
        // Unwrap ExprMakeBlock → ExprBlock at runtime
        let blk_var = next_var(index)
        body |> push <| qmacro_expr(${ var $i(blk_var) = clone_expression(($i(eff_actual) as ExprMakeBlock)._block); })
        body |> push <| qm_rtti_guard(at, blk_var, "ExprBlock")
        let cast_var = next_var(index)
        var as_val_b = make_var_ref(at, blk_var)
        var as_cast_b = new ExprAsVariant(at = at, value = as_val_b, name := "ExprBlock")
        body |> push <| qmacro_expr(${ var $i(cast_var) = $e(as_cast_b); })
        // Match arguments and return type
        generate_pattern_args_match(pat_block, cast_var, "returnType", actual_var, body, index, at)
        // Match statements
        let n_stmts = length(pat_block.list)
        if (n_stmts > 0) {
            var sc = qmacro(length($i(cast_var).list) != $v(n_stmts))
            var sf = qmacro(qm_fail_list($i(actual_var)))
            body |> push <| qm_guard(at, sc, sf)
            for (stmt, i in pat_block.list, range(n_stmts)) {
                let child_pat = stmt
                if (child_pat != null) {
                    let child_var = next_var(index)
                    body |> push <| qmacro_expr(${ var $i(child_var) = clone_expression($i(cast_var).list[$v(i)]); })
                    generate_match(child_pat, child_var, body, index, at)
                }
            }
        }
        return
    }

    // ── ExprArrayComprehension — array/table/generator comprehension ─
    if (pattern is ExprArrayComprehension) {
        let pat_ac = pattern as ExprArrayComprehension
        let pat_gen = pat_ac.generatorSyntax
        let pat_tbl = pat_ac.tableSyntax
        // Resolve: accept both ExprArrayComprehension and post-inference invoke pattern
        let eff_var = next_var(index)
        body |> push <| qmacro_expr(${ var $i(eff_var) = qm_resolve_comprehension($i(actual_var)); })
        var nc_res = qmacro($i(eff_var) == null)
        var nf_res = qmacro(qm_fail_rtti($i(actual_var)))
        body |> push <| qm_guard(at, nc_res, nf_res)
        // Cast resolved expression to ExprArrayComprehension
        let cast_var = next_var(index)
        var as_val_ac = make_var_ref(at, eff_var)
        var as_cast_ac = new ExprAsVariant(at = at, value = as_val_ac, name := "ExprArrayComprehension")
        body |> push <| qmacro_expr(${ var $i(cast_var) = $e(as_cast_ac); })
        // Match generatorSyntax (both true and false enforced)
        if (pat_gen) {
            var cg = qmacro($i(cast_var).generatorSyntax == false)
            var fg = qmacro(qm_fail_field($i(actual_var)))
            body |> push <| qm_guard(at, cg, fg)
        } else {
            var cg = qmacro($i(cast_var).generatorSyntax)
            var fg = qmacro(qm_fail_field($i(actual_var)))
            body |> push <| qm_guard(at, cg, fg)
        }
        // Match tableSyntax (both true and false enforced)
        if (pat_tbl) {
            var ct = qmacro($i(cast_var).tableSyntax == false)
            var ft = qmacro(qm_fail_field($i(actual_var)))
            body |> push <| qm_guard(at, ct, ft)
        } else {
            var ct = qmacro($i(cast_var).tableSyntax)
            var ft = qmacro(qm_fail_field($i(actual_var)))
            body |> push <| qm_guard(at, ct, ft)
        }
        // Recurse into exprFor
        let pat_for = pat_ac.exprFor
        if (pat_for != null) {
            let for_var = next_var(index)
            body |> push <| qmacro_expr(${ var $i(for_var) = clone_expression($i(cast_var).exprFor); })
            if (!is_wildcard_call(pat_for) && !(pat_for is ExprTag)) {
                var nc = qmacro($i(for_var) == null)
                var nf = qmacro(qm_fail_null())
                body |> push <| qm_guard(at, nc, nf)
            }
            generate_match(pat_for, for_var, body, index, at)
        }
        // Recurse into subexpr
        let pat_sub = pat_ac.subexpr
        if (pat_sub != null) {
            let sub_var = next_var(index)
            body |> push <| qmacro_expr(${ var $i(sub_var) = clone_expression($i(cast_var).subexpr); })
            if (!is_wildcard_call(pat_sub) && !(pat_sub is ExprTag)) {
                var nc = qmacro($i(sub_var) == null)
                var nf = qmacro(qm_fail_null())
                body |> push <| qm_guard(at, nc, nf)
            }
            generate_match(pat_sub, sub_var, body, index, at)
        }
        // Handle exprWhere
        if (pat_ac.exprWhere != null) {
            let pat_where = pat_ac.exprWhere
            let where_var = next_var(index)
            body |> push <| qmacro_expr(${ var $i(where_var) = clone_expression($i(cast_var).exprWhere); })
            if (!is_wildcard_call(pat_where) && !(pat_where is ExprTag)) {
                var nc = qmacro($i(where_var) == null)
                var nf = qmacro(qm_fail_null())
                body |> push <| qm_guard(at, nc, nf)
            }
            generate_match(pat_where, where_var, body, index, at)
        } else {
            // Pattern has no where clause — actual shouldn't either
            var nc = qmacro($i(cast_var).exprWhere != null)
            var nf = qmacro(qm_fail_field($i(actual_var)))
            body |> push <| qm_guard(at, nc, nf)
        }
        return
    }

    // ── ExprFor — handle iteratorsTags for $i capture ─────────
    if (pattern is ExprFor) {
        var pat_for = pattern as ExprFor
        // Process iteratorsTags: clear tagged iterator names (so generic matching skips them)
        // and record which indices need $i capture
        var has_tags = false
        for (tag in pat_for.iteratorsTags) {
            if (tag != null) {
                has_tags = true
                break
            }
        }
        body |> push <| qm_rtti_guard(at, actual_var, rtti)
        let cast_var = next_var(index)
        var as_val_f = make_var_ref(at, actual_var)
        var as_cast_f = new ExprAsVariant(at = at, value = as_val_f, name := rtti)
        body |> push <| qmacro_expr(${ var $i(cast_var) = $e(as_cast_f); })
        // Run annotation reflection for fields
        var ann = unsafe(reinterpret<BasicStructureAnnotation?> get_expression_annotation(pattern))
        if (ann != null) {
            for_each_field(*ann) $(name, cppName, xtype, offset) {
                return if (offset == 0xFFFFFFFFu)
                return if (qm_skip_field(rtti, name))
                return if (name == "iteratorsTags")
                return if (name == "iteratorsAt" || name == "iteratorVariables" || name == "iteratorsTupleExpansion" || name == "iteratorsAka")
                return if (name == "visibility" || name == "allowIteratorOptimization" || name == "canShadow")
                return if (length(xtype.dim) != 0)
                var p8 : int8?
                unsafe {
                    p8 = (reinterpret<int8?> pattern) + int(offset)
                }
                let bt = xtype.baseType
                // Reuse the same field matching logic from the generic handler
                if (bt == Type.tHandle && xtype.annotation.name == "dasvector`das_string") {
                    let pv = unsafe(reinterpret<dasvector`das_string?> p8)
                    let pat_len = length(*pv)
                    if (pat_len > 0) {
                        var cond_s = qmacro(length($i(cast_var).$f(name)) != $v(pat_len))
                        var fail_s = qmacro(qm_fail_list($i(actual_var)))
                        body |> push <| qm_guard(at, cond_s, fail_s)
                        for (selem, si in *pv, range(pat_len)) {
                            {
                                // Skip tagged iterators ($i capture) — they'll be handled later
                                let is_tagged = has_tags && si < length(pat_for.iteratorsTags) && pat_for.iteratorsTags[si] != null
                                if (!is_tagged) {
                                    if (!empty(selem)) {
                                        var sc = qmacro($i(cast_var).$f(name)[$v(si)] != $v(string(selem)))
                                        var sf = qmacro(qm_fail_field($i(actual_var)))
                                        body |> push <| qm_guard(at, sc, sf)
                                    }
                                }
                            }
                        }
                    }
                } elif (bt == Type.tPointer && xtype.firstType != null && xtype.firstType.annotation != null && string(xtype.firstType.annotation.name) == "Expression") {
                    let inner_ann_name = xtype.firstType != null && xtype.firstType.annotation != null ? string(xtype.firstType.annotation.name) : ""
                    if (inner_ann_name == "TypeDecl") {
                        // skip TypeDecl fields
                        pass
                    } else {
                        let pv = unsafe(reinterpret<Expression ??> p8)
                        if (*pv != null) {
                            let child_pattern = *pv
                            let child_var = next_var(index)
                            body |> push <| qmacro_expr(${ var $i(child_var) = clone_expression($i(cast_var).$f(name)); }) // nolint:LINT004
                            if (!is_wildcard_call(child_pattern) && !(child_pattern is ExprTag)) {
                                var ncond = qmacro($i(child_var) == null)
                                var nfail = qmacro(qm_fail_null())
                                body |> push <| qm_guard(at, ncond, nfail)
                            }
                            generate_match(child_pattern, child_var, body, index, at)
                        }
                    }
                } elif (bt == Type.tHandle && xtype.annotation.name == "dasvector`ptr`Expression") {
                    let pv = unsafe(reinterpret<dasvector`ptr`Expression?> p8)
                    let pat_len = length(*pv)
                    if (pat_len > 0) {
                        var cond_e = qmacro(length($i(cast_var).$f(name)) != $v(pat_len))
                        var fail_e = qmacro(qm_fail_list($i(actual_var)))
                        body |> push <| qm_guard(at, cond_e, fail_e)
                        for (elem, i in *pv, range(pat_len)) {
                            let child_pattern = elem
                            if (child_pattern != null) {
                                let child_var = next_var(index)
                                body |> push <| qmacro_expr(${ var $i(child_var) = clone_expression($i(cast_var).$f(name)[$v(i)]); })
                                generate_match(child_pattern, child_var, body, index, at)
                            }
                        }
                    }
                }
            }
        }
        // Generate $i capture code for tagged iterators
        if (has_tags) {
            for (tag, ti in pat_for.iteratorsTags, range(length(pat_for.iteratorsTags))) {
                if (tag != null) {
                    let tag_expr = tag
                    if (tag_expr is ExprVar) {
                        let cap_name = string((tag_expr as ExprVar).name)
                        body |> push <| qmacro_expr(${ $i(cap_name) = string($i(cast_var).iterators[$v(ti)]); })
                    }
                }
            }
        }
        return
    }

    // ── ExprConst* — RTTI check + value comparison ────────────
    if (rtti |> starts_with("ExprConst")) {
        generate_const_match(pattern, rtti, actual_var, body, index, at)
        return
    }

    // ── ExprCall that might be a constant constructor (range, int2, float3, etc.) ────
    // The compiler folds these into ExprConst* but raw qmacro output keeps them as ExprCall.
    // Generate a runtime check: if the actual is the folded form, match as constant;
    // otherwise match as ExprCall with one-value expansion support.
    if (rtti == "ExprCall") {
        let call = pattern as ExprCall
        let call_name = string(call.name)
        let folded_rtti = const_constructor_rtti(call_name)
        if (!empty(folded_rtti)) {
            let is_folded_var = next_var(index)
            body |> push <| qmacro_expr(${ var $i(is_folded_var) = qm_rtti($i(actual_var)) == $v(folded_rtti); })
            // If folded: match as constant constructor and continue
            var folded_body : array<Expression?>
            generate_const_constructor_match(call, folded_rtti, actual_var, folded_body, index, at)
            var folded_blk = new ExprBlock(at = at)
            for (stmt in folded_body) {
                folded_blk.list |> emplace(stmt)
            }
            var folded_cond = make_var_ref(at, is_folded_var)
            var folded_if = new ExprIfThenElse(at = at, cond = folded_cond, if_true = folded_blk)
            body |> emplace(folded_if)
            // Unfolded: match as ExprCall with one-value expansion (skip if folded matched)
            var unfolded_body : array<Expression?>
            unfolded_body |> push <| qm_rtti_guard(at, actual_var, "ExprCall")
            let cast_var = next_var(index)
            var as_val_c = make_var_ref(at, actual_var)
            var as_cast_c = new ExprAsVariant(at = at, value = as_val_c, name := "ExprCall")
            unfolded_body |> push <| qmacro_expr(${ var $i(cast_var) = $e(as_cast_c); })
            if (!empty(call_name)) {
                var nc = qmacro(!qm_call_name_matches($i(cast_var), $v(call_name)))
                var nf = qmacro(qm_fail_field($i(actual_var)))
                unfolded_body |> push <| qm_guard(at, nc, nf)
            }
            let n_pat_args = length(call.arguments)
            if (n_pat_args > 0) {
                // One-value expansion: range(y) = range(0, y), float2(v) = float2(v, v)
                // Generate: if actual has 1 arg and pattern has N, apply expansion;
                // if actual has N args, match directly; otherwise fail.
                let n_actual_var = next_var(index)
                unfolded_body |> push <| qmacro_expr(${ var $i(n_actual_var) = qm_count_real_call_args($i(cast_var).arguments); })
                let is_one_val = next_var(index)
                unfolded_body |> push <| qmacro_expr(${ var $i(is_one_val) = $i(n_actual_var) == 1 && $v(n_pat_args) > 1; })
                // Guard: actual args must be 1 (one-value) or N (full form)
                var ac = qmacro(!$i(is_one_val) && $i(n_actual_var) != $v(n_pat_args))
                var af = qmacro(qm_fail_list($i(actual_var)))
                unfolded_body |> push <| qm_guard(at, ac, af)
                // Get the single actual arg for one-value expansion
                let single_arg_var = next_var(index)
                unfolded_body |> push <| qmacro_expr(${ var $i(single_arg_var) = clone_expression($i(cast_var).arguments[qm_real_call_arg_index($i(cast_var).arguments, 0)]); })
                let is_range = is_range_constructor(call_name)
                for (arg, i in call.arguments, range(n_pat_args)) {
                    let child_pattern = arg
                    if (child_pattern == null) {
                        continue
                    }
                    let child_var = next_var(index)
                    if (is_range && i == 0) {
                        // Range first component: full form uses actual arg[0], one-value form uses 0
                        var full_body : array<Expression?>
                        let phys_full = next_var(index)
                        full_body |> push <| qmacro_expr(${ var $i(phys_full) = qm_real_call_arg_index($i(cast_var).arguments, 0); })
                        full_body |> push <| qmacro_expr(${ $i(child_var) = clone_expression($i(cast_var).arguments[$i(phys_full)]); })
                        var one_body : array<Expression?>
                        one_body |> push <| qmacro_expr(${ $i(child_var) = qm_make_zero_const($v(call_name)); })
                        // if (is_one_val) { child = zero } else { child = actual[0] }
                        var one_blk = new ExprBlock(at = at)
                        for (s in one_body) { one_blk.list |> emplace(s); }
                        var full_blk = new ExprBlock(at = at)
                        for (s in full_body) { full_blk.list |> emplace(s); }
                        var branch_cond = make_var_ref(at, is_one_val)
                        var branch_if = new ExprIfThenElse(at = at, cond = branch_cond, if_true = one_blk, if_false = full_blk)
                        unfolded_body |> push <| qmacro_expr(${ var $i(child_var) : Expression ?; })
                        unfolded_body |> emplace(branch_if)
                    } elif (is_range && i == 1) {
                        // Range second component: full form uses actual arg[1], one-value form uses actual arg[0]
                        var full_body : array<Expression?>
                        let phys_full = next_var(index)
                        full_body |> push <| qmacro_expr(${ var $i(phys_full) = qm_real_call_arg_index($i(cast_var).arguments, 1); })
                        full_body |> push <| qmacro_expr(${ $i(child_var) = clone_expression($i(cast_var).arguments[$i(phys_full)]); })
                        var one_body : array<Expression?>
                        one_body |> push <| qmacro_expr(${ $i(child_var) = clone_expression($i(single_arg_var)); })
                        var one_blk = new ExprBlock(at = at)
                        for (s in one_body) { one_blk.list |> emplace(s); }
                        var full_blk = new ExprBlock(at = at)
                        for (s in full_body) { full_blk.list |> emplace(s); }
                        var branch_cond = make_var_ref(at, is_one_val)
                        var branch_if = new ExprIfThenElse(at = at, cond = branch_cond, if_true = one_blk, if_false = full_blk)
                        unfolded_body |> push <| qmacro_expr(${ var $i(child_var) : Expression ?; })
                        unfolded_body |> emplace(branch_if)
                    } else {
                        // Vector broadcast: full form uses actual arg[i], one-value form uses actual arg[0]
                        var full_body : array<Expression?>
                        let phys_full = next_var(index)
                        full_body |> push <| qmacro_expr(${ var $i(phys_full) = qm_real_call_arg_index($i(cast_var).arguments, $v(i)); })
                        full_body |> push <| qmacro_expr(${ $i(child_var) = clone_expression($i(cast_var).arguments[$i(phys_full)]); })
                        var one_body : array<Expression?>
                        one_body |> push <| qmacro_expr(${ $i(child_var) = clone_expression($i(single_arg_var)); })
                        var one_blk = new ExprBlock(at = at)
                        for (s in one_body) { one_blk.list |> emplace(s); }
                        var full_blk = new ExprBlock(at = at)
                        for (s in full_body) { full_blk.list |> emplace(s); }
                        var branch_cond = make_var_ref(at, is_one_val)
                        var branch_if = new ExprIfThenElse(at = at, cond = branch_cond, if_true = one_blk, if_false = full_blk)
                        unfolded_body |> push <| qmacro_expr(${ var $i(child_var) : Expression ?; })
                        unfolded_body |> emplace(branch_if)
                    }
                    generate_match(child_pattern, child_var, unfolded_body, index, at)
                }
            }
            // Wrap unfolded path in if (!is_folded)
            var unfolded_blk = new ExprBlock(at = at)
            for (stmt in unfolded_body) {
                unfolded_blk.list |> emplace(stmt)
            }
            var not_folded_cond = make_var_ref(at, is_folded_var)
            var not_folded = new ExprOp1(at = at, op := "!", subexpr = not_folded_cond)
            var unfolded_if = new ExprIfThenElse(at = at, cond = not_folded, if_true = unfolded_blk)
            body |> emplace(unfolded_if)
            return
        }
    }

    // ── ExprCall / ExprInvoke — match name + arguments only ─────
    if (rtti == "ExprCall" || rtti == "ExprInvoke") {
        body |> push <| qm_rtti_guard(at, actual_var, rtti)
        let cast_var = next_var(index)
        var as_val_c = make_var_ref(at, actual_var)
        var as_cast_c = new ExprAsVariant(at = at, value = as_val_c, name := rtti)
        body |> push <| qmacro_expr(${ var $i(cast_var) = $e(as_cast_c); }) // nolint:LINT004
        // Match name (ExprCall only) then match arguments (skipping fakeContext/fakeLineInfo)
        if (rtti == "ExprCall") {
            let call = pattern as ExprCall
            if (!empty(call.name)) {
                var nc = qmacro(!qm_call_name_matches($i(cast_var), $v(string(call.name))))
                var nf = qmacro(qm_fail_field($i(actual_var)))
                body |> push <| qm_guard(at, nc, nf)
            }
            let n_args = length(call.arguments)
            if (n_args > 0) {
                var ac = qmacro(qm_count_real_call_args($i(cast_var).arguments) != $v(n_args))
                var af = qmacro(qm_fail_list($i(actual_var)))
                body |> push <| qm_guard(at, ac, af)
                for (arg, i in call.arguments, range(n_args)) {
                    let child_pattern = arg
                    if (child_pattern != null) {
                        let phys_var = next_var(index)
                        body |> push <| qmacro_expr(${ var $i(phys_var) = qm_real_call_arg_index($i(cast_var).arguments, $v(i)); }) // nolint:LINT004
                        let child_var = next_var(index)
                        body |> push <| qmacro_expr(${ var $i(child_var) = clone_expression($i(cast_var).arguments[$i(phys_var)]); }) // nolint:LINT004
                        generate_match(child_pattern, child_var, body, index, at)
                    }
                }
            }
        } elif (rtti == "ExprInvoke") {
            let inv = pattern as ExprInvoke
            let n_args = length(inv.arguments)
            if (n_args > 0) {
                var ac = qmacro(qm_count_real_call_args($i(cast_var).arguments) != $v(n_args))
                var af = qmacro(qm_fail_list($i(actual_var)))
                body |> push <| qm_guard(at, ac, af)
                for (arg, i in inv.arguments, range(n_args)) {
                    let child_pattern = arg
                    if (child_pattern != null) {
                        let phys_var = next_var(index)
                        body |> push <| qmacro_expr(${ var $i(phys_var) = qm_real_call_arg_index($i(cast_var).arguments, $v(i)); }) // nolint:LINT004
                        let child_var = next_var(index)
                        body |> push <| qmacro_expr(${ var $i(child_var) = clone_expression($i(cast_var).arguments[$i(phys_var)]); }) // nolint:LINT004
                        generate_match(child_pattern, child_var, body, index, at)
                    }
                }
            }
        }
        return
    }

    // ── Generic expression — RTTI check + field-by-field ──────
    body |> push <| qm_rtti_guard(at, actual_var, rtti)

    // Generate: var cast = actual as ExprXxx
    let cast_var = next_var(index)
    var as_val2 = make_var_ref(at, actual_var)
    var as_cast2 = new ExprAsVariant(at = at, value = as_val2, name := rtti)
    body |> push <| qmacro_expr(${ var $i(cast_var) = $e(as_cast2); }) // nolint:LINT004

    // Iterate fields via annotation reflection
    var ann = unsafe(reinterpret<BasicStructureAnnotation?> get_expression_annotation(pattern))
    if (ann == null) {
        return
    }

    // Field-by-field matching via annotation reflection
    for_each_field(*ann) $(name, cppName, xtype, offset) {
        return if (offset == 0xFFFFFFFFu)
        return if (qm_skip_field(rtti, name))
        if (length(xtype.dim) != 0) {
            panic("qmatch: fixed-size array field '{name}' not supported")
        }
        var p8 : int8?
        unsafe {
            p8 = (reinterpret<int8?> pattern) + int(offset)
        }
        let bt = xtype.baseType

        let inner_ann_name = bt == Type.tPointer && xtype.firstType != null && xtype.firstType.annotation != null ? string(xtype.firstType.annotation.name) : ""
        if (inner_ann_name == "TypeDecl" || inner_ann_name == "Expression") {
            if (inner_ann_name == "TypeDecl") {
                // TypeDecl? field — use generate_type_match
                let pv = unsafe(reinterpret<TypeDeclPtr?> p8)
                if (*pv != null) {
                    let child_td = *pv
                    let child_var = next_var(index)
                        // Check not null
                    var ncond = qmacro($i(cast_var).$f(name) == null)
                    var nfail = qmacro(qm_fail_null())
                    body |> push <| qm_guard(at, ncond, nfail)
                    body |> push <| qmacro_expr(${ var $i(child_var) = clone_type($i(cast_var).$f(name)); })
                    generate_type_match(child_td, child_var, actual_var, body, index, at)
                }
                // If pattern's TypeDecl field is null, skip (don't require actual's to be null)
            } else {
                // Expression? child — recurse or check null
                let pv = unsafe(reinterpret<Expression ??> p8)
                if (*pv != null) {
                    let child_pattern = *pv
                    let child_var = next_var(index)
                    body |> push <| qmacro_expr(${ var $i(child_var) = clone_expression($i(cast_var).$f(name)); }) // nolint:LINT004
                    if (!is_wildcard_call(child_pattern) && !(child_pattern is ExprTag)) {
                        var ncond = qmacro($i(child_var) == null)
                        var nfail = qmacro(qm_fail_null())
                        body |> push <| qm_guard(at, ncond, nfail)
                    }
                    generate_match(child_pattern, child_var, body, index, at)
                } else {
                    // Pattern's child is null — require actual's child is also null
                    var ncond = qmacro($i(cast_var).$f(name) != null)
                    var nfail = qmacro(qm_fail_field($i(actual_var)))
                    body |> push <| qm_guard(at, ncond, nfail)
                }
            }
        } elif (bt == Type.tHandle && xtype.annotation.name == "das_string") {
            // das_string field
            let pv = unsafe(reinterpret<das_string?> p8)
            if (!empty(*pv)) {
                var cond = qmacro($i(cast_var).$f(name) != $v(string(*pv)))
                var fail = qmacro(qm_fail_field($i(actual_var)))
                body |> push <| qm_guard(at, cond, fail)
            }
        } elif (bt == Type.tHandle && xtype.annotation.name == "dasvector`ptr`Expression") {
            // Expression vector field (block.list, call.arguments)
            let pv = unsafe(reinterpret<dasvector`ptr`Expression?> p8)
            let pat_len = length(*pv)
            if (pat_len > 0) {
                var cond = qmacro(length($i(cast_var).$f(name)) != $v(pat_len))
                var fail = qmacro(qm_fail_list($i(actual_var)))
                body |> push <| qm_guard(at, cond, fail)
                for (elem, i in *pv, range(pat_len)) {
                    let child_pattern = elem
                    if (child_pattern != null) {
                        let child_var = next_var(index)
                        body |> push <| qmacro_expr(${ var $i(child_var) = clone_expression($i(cast_var).$f(name)[$v(i)]); })
                        generate_match(child_pattern, child_var, body, index, at)
                    }
                }
            }
        } elif (bt == Type.tHandle && xtype.annotation.name == "dasvector`ptr`Variable") {
            // Variable vector (ExprLet.variables) — match name, type, init
            var pv = unsafe(reinterpret<dasvector`ptr`Variable?> p8)
            let pat_len = length(*pv)
            if (pat_len > 0) {
                var cond = qmacro(length($i(cast_var).$f(name)) != $v(pat_len))
                var fail = qmacro(qm_fail_list($i(actual_var)))
                body |> push <| qm_guard(at, cond, fail)
                for (velem, vi in *pv, range(pat_len)) {
                    {
                        var pat_v = velem
                        // Name matching
                        if (pat_v.flags.tag && pat_v.source != null) {
                            // $i capture — extract variable name
                            var src = pat_v.source
                            if (src is ExprVar) {
                                body |> push <| qmacro_expr(${ $i((src as ExprVar).name) = $i(cast_var).$f(name)[$v(vi)].name; })
                            }
                        } else {
                            if (!empty(pat_v.name)) {
                                var nc = qmacro($i(cast_var).$f(name)[$v(vi)].name != $v(string(pat_v.name)))
                                var nf = qmacro(qm_fail_field($i(actual_var)))
                                body |> push <| qm_guard(at, nc, nf)
                            }
                        }
                        // Init expression matching
                        if (pat_v.init != null) {
                            let init_pat = pat_v.init
                            let init_var = next_var(index)
                            body |> push <| qmacro_expr(${ var $i(init_var) = clone_expression($i(cast_var).$f(name)[$v(vi)].init); })
                            generate_match(init_pat, init_var, body, index, at)
                        }
                        // Type matching (skip autoinfer)
                        if (pat_v._type != null) {
                            var pat_td = pat_v._type
                            if (pat_td.baseType != Type.autoinfer) {
                                let td_var = next_var(index)
                                body |> push <| qmacro_expr(${ var $i(td_var) = clone_type($i(cast_var).$f(name)[$v(vi)]._type); })
                                generate_type_match(pat_td, td_var, actual_var, body, index, at)
                            }
                        }
                    }
                }
            }
        } elif (bt == Type.tHandle && xtype.annotation.name == "dasvector`das_string") {
            // String vector (ExprFor.iterators) — match name strings
            let pv = unsafe(reinterpret<dasvector`das_string?> p8)
            let pat_len = length(*pv)
            if (pat_len > 0) {
                var cond = qmacro(length($i(cast_var).$f(name)) != $v(pat_len))
                var fail = qmacro(qm_fail_list($i(actual_var)))
                body |> push <| qm_guard(at, cond, fail)
                for (selem, si in *pv, range(pat_len)) {
                    {
                        if (!empty(selem)) {
                            var sc = qmacro($i(cast_var).$f(name)[$v(si)] != $v(string(selem)))
                            var sf = qmacro(qm_fail_field($i(actual_var)))
                            body |> push <| qm_guard(at, sc, sf)
                        }
                    }
                }
            }
        } elif (bt == Type.tInt) {
            let pv = unsafe(reinterpret<int?> p8)
            if (*pv != 0) {
                let fval = *pv
                var cond = qmacro($i(cast_var).$f(name) != $v(fval))
                var fail = qmacro(qm_fail_field($i(actual_var)))
                body |> push <| qm_guard(at, cond, fail)
            }
        } elif (bt == Type.tUInt) {
            let pv = unsafe(reinterpret<uint?> p8)
            if (*pv != 0u) {
                let fval = *pv
                var cond = qmacro($i(cast_var).$f(name) != $v(fval))
                var fail = qmacro(qm_fail_field($i(actual_var)))
                body |> push <| qm_guard(at, cond, fail)
            }
        } elif (bt == Type.tBool) {
            let pv = unsafe(reinterpret<bool?> p8)
            if (*pv) {
                var cond = qmacro($i(cast_var).$f(name) == false)
                var fail = qmacro(qm_fail_field($i(actual_var)))
                body |> push <| qm_guard(at, cond, fail)
            }
        }
        // Skip: tBitfield, tHandle(LineInfo), tString(__rtti), tPointer(non-smart)
    }
}

// ── Block matching code generation ───────────────────────────────────────

[macro_function]
def private get_wildcard_name(pattern : Expression?) : string {
    //! Get the wildcard type name, or "" if not a wildcard.
    if (!(pattern is ExprCall)) {
        return ""
    }
    let call = pattern as ExprCall
    if (call.name == "_wildcard" || call.name == "_wildcard1" || call.name == "_optional" || call.name == "_any") {
        return string(call.name)
    }
    return ""
}

[macro_function]
def private count_fixed_stmts(pat_block : ExprBlock?) : int {
    //! Count non-wildcard statements in a pattern block.
    var count = 0
    for (stmt in pat_block.list) {
        if (empty(get_wildcard_name(stmt))) {
            count ++
        }
    }
    return count
}

[macro_function]
def private get_single_fixed_stmt(var pat_block : ExprBlock?) : Expression? {
    //! If pattern has exactly one non-wildcard statement (possibly surrounded by wildcards),
    //! return it. Otherwise return null.
    for (stmt in pat_block.list) {
        if (empty(get_wildcard_name(stmt))) {
            return stmt
        }
    }
    return null
}

[macro_function]
def private generate_block_match(var pat_block : ExprBlock?; actual_var : string; var body : array<Expression?>&; var index : int&; at : LineInfo; check_args : bool = true) {
    //! Generate code to match a block's statement list with wildcard support.
    //! If the optimizer folded a single-statement block into a bare expression,
    //! and the pattern has exactly one fixed statement (with optional wildcards),
    //! match the bare expression directly against that statement.

    // Check if pattern is a single fixed statement (possibly with wildcards).
    // If actual is not ExprBlock, try matching the single statement directly.
    // Skip this optimization if pattern has arguments or return type — those require a real block.
    let has_pat_args = length(pat_block.arguments) > 0
    let has_pat_ret = pat_block.returnType != null && pat_block.returnType.baseType != Type.autoinfer && pat_block.returnType.baseType != Type.tVoid
    let n_fixed = count_fixed_stmts(pat_block)
    if (n_fixed == 1 && !has_pat_args && !has_pat_ret) {
        var single_pat = get_single_fixed_stmt(pat_block)
        if (single_pat != null) {
            // Generate: if (!(actual is ExprBlock) && !(actual is ExprMakeBlock))
            //              match single_pat against actual directly
            //           else
            //              fall through to normal block matching
            // For simplicity: try block match first. If RTTI fails (not a block),
            // fall back to direct match. We use a runtime helper for this.
            let direct_var = next_var(index)
            body |> push <| qmacro_expr(${ var $i(direct_var) = qm_rtti($i(actual_var)) != "ExprBlock" && qm_rtti($i(actual_var)) != "ExprMakeBlock"; })
            // If it's not a block, match directly
            var direct_body : array<Expression?>
            generate_match(single_pat, actual_var, direct_body, index, at)
            direct_body |> push <| qmacro_expr(${return qm_ok(); })
            // Build: if (direct_var) { direct match + return }
            var direct_blk = new ExprBlock(at = at)
            for (stmt in direct_body) {
                direct_blk.list |> emplace(stmt)
            }
            var direct_cond = make_var_ref(at, direct_var)
            var direct_if = new ExprIfThenElse(at = at, cond = direct_cond, if_true = direct_blk)
            body |> emplace(direct_if)
            // Fall through to block matching if it IS a block
        }
    }

    // Navigate to ExprBlock — actual may be ExprMakeBlock (wraps ExprBlock) or ExprBlock directly.
    let blk_var = next_var(index)
    body |> push <| qmacro_expr(${ var $i(blk_var) = clone_expression($i(actual_var)); })
    // Unwrap ExprMakeBlock if present
    var vref_blk = make_var_ref(at, blk_var)
    var is_mkblk = make_is_check(at, vref_blk, "ExprMakeBlock")
    let tmp_var = next_var(index)
    var unwrap_1 = qmacro_expr(${ var $i(tmp_var) = clone_expression(($i(blk_var) as ExprMakeBlock)._block); })
    var unwrap_2 = qmacro_expr(${ $i(blk_var) <- $i(tmp_var); })
    var unwrap_blk = new ExprBlock(at = at)
    unwrap_blk.list |> emplace(unwrap_1)
    unwrap_blk.list |> emplace(unwrap_2)
    var unwrap_if = new ExprIfThenElse(at = at, cond = is_mkblk, if_true = unwrap_blk)
    body |> emplace(unwrap_if)
    // Now verify it's ExprBlock
    body |> push <| qm_rtti_guard(at, blk_var, "ExprBlock")
    let cast_var = next_var(index)
    var as_val = make_var_ref(at, blk_var)
    var as_cast = new ExprAsVariant(at = at, value = as_val, name := "ExprBlock")
    body |> push <| qmacro_expr(${ var $i(cast_var) = $e(as_cast); })

    // Match block arguments and return type (if requested and pattern specifies them)
    if (check_args) {
        generate_pattern_args_match(pat_block, cast_var, "returnType", actual_var, body, index, at)
    }

    // Declare position cursor: var _qm_pos = 0
    let pos_var = next_var(index)
    body |> push <| qmacro_expr(${ var $i(pos_var) = 0; })

    // Get the total length of the actual block for bounds checking
    let len_var = next_var(index)
    body |> push <| qmacro_expr(${ var $i(len_var) = length($i(cast_var).list); })

    // Walk through pattern statements
    // Track pending $b capture: save_var holds start position, b_vname holds user variable name
    var pending_b_save = ""   // name of the var holding wildcard start position
    var pending_b_vname = ""  // name of the user's $b capture variable

    let pat_len = length(pat_block.list)
    for (pat_stmt, pat_idx in pat_block.list, range(pat_len)) {
        let pat = pat_stmt
        let wc_name = get_wildcard_name(pat)

        if (wc_name == "_wildcard") {
            // Zero or more — just skip. The next fixed pattern will scan forward.
            // Save start position for $b capture
            let b_vname = get_wildcard_b_tag(pat)
            if (!empty(b_vname)) {
                let save_var = next_var(index)
                body |> push <| qmacro_expr(${ var $i(save_var) = $i(pos_var); })
                pending_b_save = save_var
                pending_b_vname = b_vname
            }
            if (pat_idx == pat_len - 1) {
                // Trailing wildcard — matches rest, done
                // Emit $b capture: [save_pos, len)
                if (!empty(pending_b_vname)) {
                    body |> push <| qmacro_expr(${ qm_extract_stmts($i(blk_var), $i(pending_b_save), $i(len_var), $i(pending_b_vname)); })
                    pending_b_save = ""
                    pending_b_vname = ""
                }
            }
            // Otherwise, the next fixed statement will handle scanning
        } elif (wc_name == "_wildcard1") {
            // One or more — advance pos by at least 1
            let b_vname = get_wildcard_b_tag(pat)
            if (!empty(b_vname)) {
                let save_var = next_var(index)
                body |> push <| qmacro_expr(${ var $i(save_var) = $i(pos_var); })
                pending_b_save = save_var
                pending_b_vname = b_vname
            }
            var cond = qmacro($i(pos_var) >= $i(len_var))
            var fail = qmacro(qm_fail_wildcard($i(actual_var)))
            body |> push <| qm_guard(at, cond, fail)
            body |> push <| qmacro_expr(${ $i(pos_var) ++; })
            if (pat_idx == pat_len - 1) {
                // Trailing _wildcard1 — capture [save_pos, len)
                if (!empty(pending_b_vname)) {
                    body |> push <| qmacro_expr(${ qm_extract_stmts($i(blk_var), $i(pending_b_save), $i(len_var), $i(pending_b_vname)); })
                    pending_b_save = ""
                    pending_b_vname = ""
                }
            }
        } elif (wc_name == "_optional") {
            // Zero or one — like _wildcard but limited to at most 1 statement
            let b_vname = get_wildcard_b_tag(pat)
            if (!empty(b_vname)) {
                let save_var = next_var(index)
                body |> push <| qmacro_expr(${ var $i(save_var) = $i(pos_var); })
                pending_b_save = save_var
                pending_b_vname = b_vname
            }
            if (pat_idx == pat_len - 1) {
                // Trailing _optional — at most 1 remaining statement
                var cond = qmacro($i(len_var) - $i(pos_var) > 1)
                var fail = qmacro(qm_fail_list($i(actual_var)))
                body |> push <| qm_guard(at, cond, fail)
                if (!empty(pending_b_vname)) {
                    body |> push <| qmacro_expr(${ qm_extract_stmts($i(blk_var), $i(pending_b_save), $i(len_var), $i(pending_b_vname)); })
                    pending_b_save = ""
                    pending_b_vname = ""
                }
            }
        } elif (wc_name == "_any") {
            // Exactly one — advance pos by exactly 1
            let b_vname = get_wildcard_b_tag(pat)
            var cond = qmacro($i(pos_var) >= $i(len_var))
            var fail = qmacro(qm_fail_wildcard($i(actual_var)))
            body |> push <| qm_guard(at, cond, fail)
            if (!empty(b_vname)) {
                // _any captures exactly one statement
                body |> push <| qmacro_expr(${ qm_extract_stmts($i(blk_var), $i(pos_var), $i(pos_var) + 1, $i(b_vname)); })
            }
            body |> push <| qmacro_expr(${ $i(pos_var) ++; })
        } else {
            // Fixed statement — must match at current position
            // Check if the previous statement was a wildcard (need to scan)
            let prev_is_wildcard = pat_idx > 0 && !empty(get_wildcard_name(pat_block.list[pat_idx - 1]))
            let prev_wc = prev_is_wildcard ? get_wildcard_name(pat_block.list[pat_idx - 1]) : ""

            if (prev_wc == "_wildcard" || prev_wc == "_wildcard1" || prev_wc == "_optional") {
                // Scan forward: build an inner matcher block and call qm_scan
                let elem_var = next_var(index)

                // Generate match code for the fixed pattern
                var match_body : array<Expression?>
                generate_match(pat, elem_var, match_body, index, at)
                match_body |> push <| qmacro_expr(${return qm_ok(); })

                // Build inner block: $(var elem : Expression?) : QMatchResult { match_code }
                var inner_blk = new ExprBlock(at = at)
                inner_blk.blockFlags |= ExprBlockFlags.isClosure
                inner_blk.returnType  = typeinfo ast_typedecl(type<QMatchResult>)
                var inner_arg <- new Variable(at = at, name := elem_var)
                inner_arg._type  = typeinfo ast_typedecl(type<Expression?>)
                inner_arg.access_flags |= VariableAccessFlags.access_ref
                inner_blk.arguments |> emplace(inner_arg)
                for (stmt in match_body) {
                    inner_blk.list |> emplace(stmt)
                }
                var inner_mkblk = new ExprMakeBlock(at = at, _block = inner_blk)

                // Generate: var scan_r = qm_scan(blk_var, pos_var, scan_limit, inner_block)
                // For _optional, limit scan to pos+2 (try at most positions pos and pos+1)
                let scan_r_var = next_var(index)
                if (prev_wc == "_optional") {
                    let limit_var = next_var(index)
                    body |> push <| qmacro_expr(${ var $i(limit_var) = $i(pos_var) + 2 < $i(len_var) ? $i(pos_var) + 2 : $i(len_var); })
                    body |> push <| qmacro_expr(${ var $i(scan_r_var) = qm_scan($i(blk_var), $i(pos_var), $i(limit_var), $e(inner_mkblk)); })
                } else {
                    body |> push <| qmacro_expr(${ var $i(scan_r_var) = qm_scan($i(blk_var), $i(pos_var), $i(len_var), $e(inner_mkblk)); })
                }
                // Guard on scan result
                var scan_cond = qmacro($i(scan_r_var).matched == false)
                var scan_fail = qmacro($i(scan_r_var))
                body |> push <| qm_guard(at, scan_cond, scan_fail)

                // Emit pending $b capture: [save_pos, pos - 1) — pos was updated by qm_scan to after the match
                if (!empty(pending_b_vname)) {
                    body |> push <| qmacro_expr(${ qm_extract_stmts($i(blk_var), $i(pending_b_save), $i(pos_var) - 1, $i(pending_b_vname)); })
                    pending_b_save = ""
                    pending_b_vname = ""
                }
            } else {
                // No preceding wildcard — must match at exact position
                // Emit pending $b capture before this fixed match (for _wildcard1 followed by fixed)
                if (!empty(pending_b_vname)) {
                    body |> push <| qmacro_expr(${ qm_extract_stmts($i(blk_var), $i(pending_b_save), $i(pos_var), $i(pending_b_vname)); })
                    pending_b_save = ""
                    pending_b_vname = ""
                }
                // Bounds check
                var cond = qmacro($i(pos_var) >= $i(len_var))
                var fail = qmacro(qm_fail_wildcard($i(actual_var)))
                body |> push <| qm_guard(at, cond, fail)
                // Get element at position
                let elem_var = next_var(index)
                body |> push <| qmacro_expr(${ var $i(elem_var) = clone_expression($i(cast_var).list[$i(pos_var)]); })
                // Match it
                generate_match(pat, elem_var, body, index, at)
                // Advance
                body |> push <| qmacro_expr(${ $i(pos_var) ++; })
            }
        }
    }

    // After all patterns, check that we consumed everything (unless trailing wildcard)
    let last_is_wildcard = pat_len > 0 && !empty(get_wildcard_name(pat_block.list[pat_len - 1]))
    if (!last_is_wildcard) {
        var cond = qmacro($i(pos_var) != $i(len_var))
        var fail = qmacro(qm_fail_list($i(actual_var)))
        body |> push <| qm_guard(at, cond, fail)
    }
}

// ── Argument validation ─────────────────────────────────────────────────

[macro_function]
def private qm_validate_expr_arg(prog : ProgramPtr; arg : Expression?; at : LineInfo; var result : Expression?&) : bool {
    if (arg._type == null) {
        macro_error(prog, at, "qmatch: first argument has no type")
        return false
    }
    if (arg._type.baseType == Type.tPointer) {
        result = clone_expression(arg)
        return true
    } else {
        macro_error(prog, at, "qmatch: first argument must be Expression? or ExpressionPtr, got {describe(arg._type)}")
        return false
    }
}

// ── Call macros ─────────────────────────────────────────────────────────

[call_macro(name="qmatch")]
class private QMatch : AstCallMacro {
    def override canVisitArgument(call : ExprCallMacro?; argIndex : int) : bool {
        return argIndex == 0
    }
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        if (call.arguments |> length != 2) {
            macro_error(prog, call.at, "expecting qmatch(expression, pattern)")
            return null
        }
        let at = call.at
        let pattern = call.arguments[1]

        // Build block: $(var _qm_arg : Expression?) : QMatchResult { ... }
        var blk = new ExprBlock(at = at)
        blk.blockFlags |= ExprBlockFlags.isClosure
        blk.returnType  = typeinfo ast_typedecl(type<QMatchResult>)
        var arg_var <- new Variable(at = at, name := "_qm_arg")
        arg_var._type  = typeinfo ast_typedecl(type<Expression?>)
        arg_var.access_flags |= VariableAccessFlags.access_ref
        blk.arguments |> emplace(arg_var)

        // Null check
        blk.list |> emplace_new <| qm_null_guard(at, "_qm_arg")
        // Pattern matching
        var index = 0
        var body : array<Expression?>
        generate_match(pattern, "_qm_arg", body, index, at)
        for (stmt in body) {
            blk.list |> emplace(stmt)
        }
        blk.list |> emplace_new <| qmacro_expr(${return qm_ok(); })

        var expr_arg : Expression?
        if (!qm_validate_expr_arg(prog, call.arguments[0], at, expr_arg)) {
            return null
        }

        var mkblk = new ExprMakeBlock(at = at, _block = blk)
        return new ExprCall(at = at,
            name := "qm_run",
            arguments := array<Expression?>(expr_arg, mkblk)
        )
    }
}

[call_macro(name="qmatch_block")]
class private QMatchBlock : AstCallMacro {
    def override canVisitArgument(call : ExprCallMacro?; argIndex : int) : bool {
        return argIndex == 0  // QMatchBlock
    }
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        if (call.arguments |> length != 2) {
            macro_error(prog, call.at, "expecting qmatch_block(expression, block_pattern)")
            return null
        }
        let at = call.at

        // arg[1] is ExprMakeBlock containing ExprBlock
        let pat_arg = call.arguments[1]
        if (!(pat_arg is ExprMakeBlock)) {
            macro_error(prog, at, "qmatch_block: second argument must be a block")
            return null
        }
        let pat_mkblk = pat_arg as ExprMakeBlock
        var pat_block = pat_mkblk._block as ExprBlock
        if (pat_block == null) {
            macro_error(prog, at, "qmatch_block: could not extract block from pattern")
            return null
        }

        // Build block: $(var _qm_arg : Expression?) : QMatchResult { ... }
        var blk = new ExprBlock(at = at)
        blk.blockFlags |= ExprBlockFlags.isClosure
        blk.returnType  = typeinfo ast_typedecl(type<QMatchResult>)
        var arg_var <- new Variable(at = at, name := "_qm_arg")
        arg_var._type  = typeinfo ast_typedecl(type<Expression?>)
        arg_var.access_flags |= VariableAccessFlags.access_ref
        blk.arguments |> emplace(arg_var)

        // Null check
        blk.list |> emplace_new <| qm_null_guard(at, "_qm_arg")
        // Block matching
        var index = 0
        var body : array<Expression?>
        generate_block_match(pat_block, "_qm_arg", body, index, at)
        for (stmt in body) {
            blk.list |> emplace(stmt)
        }
        blk.list |> emplace_new <| qmacro_expr(${return qm_ok(); })

        var expr_arg : Expression?
        if (!qm_validate_expr_arg(prog, call.arguments[0], at, expr_arg)) {
            return null
        }

        var mkblk = new ExprMakeBlock(at = at, _block = blk)
        return new ExprCall(at = at,
            name := "qm_run",
            arguments := array<Expression?>(expr_arg, mkblk)
        )
    }
}

[call_macro(name="qmatch_function")]
class private QMatchFunction : AstCallMacro {
    //! ``qmatch_function(func, block_pattern)`` — match function body against block pattern.
    def override canVisitArgument(call : ExprCallMacro?; argIndex : int) : bool {
        return argIndex == 0
    }
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        if (call.arguments |> length != 2) {
            macro_error(prog, call.at, "expecting qmatch_function(func, block_pattern)")
            return null
        }
        let at = call.at

        let pat_arg = call.arguments[1]
        if (!(pat_arg is ExprMakeBlock)) {
            macro_error(prog, at, "qmatch_function: second argument must be a block")
            return null
        }
        let pat_mkblk = pat_arg as ExprMakeBlock
        var pat_block = pat_mkblk._block as ExprBlock
        if (pat_block == null) {
            macro_error(prog, at, "qmatch_function: could not extract block from pattern")
            return null
        }

        // Build block: $(var _qm_arg : ExpressionPtr; _qm_func : Function?) : QMatchResult { ... }
        var blk = new ExprBlock(at = at)
        blk.blockFlags |= ExprBlockFlags.isClosure
        blk.returnType  = typeinfo ast_typedecl(type<QMatchResult>)
        // First param: body (ExpressionPtr)
        var arg_body <- new Variable(at = at, name := "_qm_arg")
        arg_body._type  = typeinfo ast_typedecl(type<Expression?>)
        arg_body.access_flags |= VariableAccessFlags.access_ref
        blk.arguments |> emplace(arg_body)
        // Second param: function pointer (Function?)
        var arg_func <- new Variable(at = at, name := "_qm_func")
        arg_func._type  = typeinfo ast_typedecl(type<Function?>)
        blk.arguments |> emplace(arg_func)

        blk.list |> emplace_new <| qm_null_guard(at, "_qm_arg")

        var index = 0
        var body : array<Expression?>

        // Match function arguments and return type against _qm_func
        generate_pattern_args_match(pat_block, "_qm_func", "result", "_qm_arg", body, index, at)

        // Match function body (skip arg check — already done against _qm_func)
        generate_block_match(pat_block, "_qm_arg", body, index, at, false)
        for (stmt in body) {
            blk.list |> emplace(stmt)
        }
        blk.list |> emplace_new <| qmacro_expr(${return qm_ok(); })

        var func_arg = clone_expression(call.arguments[0])
        var mkblk_f = new ExprMakeBlock(at = at, _block = blk)
        return new ExprCall(at = at,
            name := "qm_run_function",
            arguments := array<Expression?>(func_arg, mkblk_f)
        )
    }
}
