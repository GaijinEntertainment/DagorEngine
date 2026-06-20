options gen2

options rtti = true
options strict_smart_pointers = false

require daslib/rtti
require daslib/ast
require daslib/ast_boost
require daslib/strings_boost


// CanonicalVisitor walks one function and emits a flat tag stream
// where every user identifier is replaced by <var_N>, every type by
// <type>, and every literal by <lit>. The token stream is what we
// hash for exact clustering and shingle for MinHash.
class CanonicalVisitor : AstVisitor {
    writer : StringBuilderWriter?
    rename : table<string; string>
    next_var : int = 0
    typ_depth : int = 0

    def emit(tag : string) {
        write(*writer, tag)
        write(*writer, " ")
    }

    def assign_var(name : das_string) : string {
        let key = string(name)
        if (key_exists(rename, key)) return rename[key]
        let placeholder = "<var_{next_var}>"
        rename |> insert(key, placeholder)
        next_var++
        return placeholder
    }

    def lookup_var(name : das_string) : string {
        let key = string(name)
        if (key_exists(rename, key)) return rename[key]
        return "<sym>"
    }

// function
    def override preVisitFunction(fun : FunctionPtr) {
        rename |> clear
        next_var = 0
        typ_depth = 0
        emit(fun.flags._lambda ? "LAMBDA" : "FN")
    }
    def override visitFunction(var fun : FunctionPtr) : FunctionPtr {
        emit("ENDFN")
        return fun
    }
    def override preVisitFunctionArgument(fun : FunctionPtr; arg : VariablePtr; lastArg : bool) {
        let p = assign_var(arg.name)
        emit("ARG")
        emit(p)
    }
    def override preVisitFunctionBody(fun : FunctionPtr; expr : ExpressionPtr) {
        emit("BODY")
    }

// type
    def override preVisitTypeDecl(typ : TypeDeclPtr) {
        if (typ_depth == 0) {
            emit("TYP")
        }
        typ_depth++
    }
    def override visitTypeDecl(var typ : TypeDeclPtr) : TypeDeclPtr {
        typ_depth--
        return typ
    }

// alias
    def override preVisitAlias(typ : TypeDeclPtr; name : das_string) {
        emit("ALIAS")
    }

// block
    def override preVisitExprBlock(blk : ExprBlock?) {
        emit("BLK")
    }
    def override visitExprBlock(blk : ExprBlock?) : ExpressionPtr {
        emit("ENDBLK")
        return unsafe(reinterpret<ExpressionPtr>(blk))
    }
    def override preVisitExprBlockArgument(blk : ExprBlock?; arg : VariablePtr; lastArg : bool) {
        let p = assign_var(arg.name)
        emit("BLK_ARG")
        emit(p)
    }
    def override preVisitExprBlockExpression(blk : ExprBlock?; expr : ExpressionPtr) {
        emit("STMT")
    }
    def override preVisitExprBlockFinalExpression(blk : ExprBlock?; expr : ExpressionPtr) {
        emit("FIN_STMT")
    }

// let
    def override preVisitExprLet(expr : ExprLet?) {
        emit("LET")
    }
    def override preVisitExprLetVariable(expr : ExprLet?; arg : VariablePtr; lastArg : bool) {
        let p = assign_var(arg.name)
        emit("LET_VAR")
        emit(p)
    }

// for
    def override preVisitExprFor(expr : ExprFor?) {
        emit("FOR")
    }
    def override preVisitExprForVariable(expr : ExprFor?; svar : VariablePtr; last : bool) {
        let p = assign_var(svar.name)
        emit("FOR_VAR")
        emit(p)
    }
    def override preVisitExprForBody(expr : ExprFor?) {
        emit("FOR_BODY")
    }

// while
    def override preVisitExprWhile(expr : ExprWhile?) {
        emit("WHILE")
    }
    def override preVisitExprWhileBody(expr : ExprWhile?; right : ExpressionPtr) {
        emit("WHILE_BODY")
    }

// if
    def override preVisitExprIfThenElse(expr : ExprIfThenElse?) {
        emit("IF")
    }
    def override preVisitExprIfThenElseIfBlock(expr : ExprIfThenElse?; ifBlock : ExpressionPtr) {
        emit("THEN")
    }
    def override preVisitExprIfThenElseElseBlock(expr : ExprIfThenElse?; elseBlock : ExpressionPtr) {
        emit("ELSE")
    }

// try-catch
    def override preVisitExprTryCatch(expr : ExprTryCatch?) {
        emit("TRY")
    }
    def override preVisitExprTryCatchCatch(expr : ExprTryCatch?; right : ExpressionPtr) {
        emit("CATCH")
    }

// with / assume
    def override preVisitExprWith(expr : ExprWith?) {
        emit("WITH")
    }
    def override preVisitExprAssume(expr : ExprAssume?) {
        emit("ASSUME")
    }

// control flow
    def override preVisitExprReturn(expr : ExprReturn?) {
        emit("RET")
    }
    def override preVisitExprYield(expr : ExprYield?) {
        emit("YIELD")
    }
    def override preVisitExprBreak(expr : ExprBreak?) {
        emit("BREAK")
    }
    def override preVisitExprContinue(expr : ExprContinue?) {
        emit("CONT")
    }
    def override preVisitExprLabel(expr : ExprLabel?) {
        emit("LABEL")
    }
    def override preVisitExprGoto(expr : ExprGoto?) {
        emit("GOTO")
    }

// operators
    def override preVisitExprOp1(expr : ExprOp1?) {
        emit("OP1:{expr.op}")
    }
    def override preVisitExprOp2(expr : ExprOp2?) {
        emit("OP2:{expr.op}")
    }
    def override preVisitExprOp3(expr : ExprOp3?) {
        emit("OP3")
    }

// copy/move/clone
    def override preVisitExprCopy(expr : ExprCopy?) {
        emit("COPY")
    }
    def override preVisitExprMove(expr : ExprMove?) {
        emit("MOVE")
    }
    def override preVisitExprClone(expr : ExprClone?) {
        emit("CLONE")
    }

// new / delete / cast / ascend
    def override preVisitExprNew(expr : ExprNew?) {
        emit("NEW")
    }
    def override preVisitExprDelete(expr : ExprDelete?) {
        emit("DEL")
    }
    def override preVisitExprCast(expr : ExprCast?) {
        emit("CAST")
    }
    def override preVisitExprAscend(expr : ExprAscend?) {
        emit("ASC")
    }
    def override preVisitExprAddr(expr : ExprAddr?) {
        emit("ADDR")
    }
    def override preVisitExprPtr2Ref(expr : ExprPtr2Ref?) {
        emit("P2R")
    }
    def override preVisitExprRef2Value(expr : ExprRef2Value?) {
        emit("R2V")
    }
    def override preVisitExprRef2Ptr(expr : ExprRef2Ptr?) {
        emit("R2P")
    }
    def override preVisitExprUnsafe(expr : ExprUnsafe?) {
        emit("UNSAFE")
    }

// at / safe at
    def override preVisitExprAt(expr : ExprAt?) {
        emit("AT")
    }
    def override preVisitExprSafeAt(expr : ExprSafeAt?) {
        emit("SAFE_AT")
    }

// is / variant
    def override preVisitExprIs(expr : ExprIs?) {
        emit("IS")
    }
    def override preVisitExprIsVariant(expr : ExprIsVariant?) {
        emit("IS_VAR")
    }
    def override preVisitExprAsVariant(expr : ExprAsVariant?) {
        emit("AS_VAR")
    }
    def override preVisitExprSafeAsVariant(expr : ExprSafeAsVariant?) {
        emit("SAFE_AS_VAR")
    }

// null coalescing
    def override preVisitExprNullCoalescing(expr : ExprNullCoalescing?) {
        emit("NCOA")
    }

// make-* aggregates
    def override preVisitExprMakeStruct(expr : ExprMakeStruct?) {
        emit("MK_STR")
    }
    def override preVisitExprMakeArray(expr : ExprMakeArray?) {
        emit("MK_ARR")
    }
    def override preVisitExprMakeTuple(expr : ExprMakeTuple?) {
        emit("MK_TUP")
    }
    def override preVisitExprMakeVariant(expr : ExprMakeVariant?) {
        emit("MK_VAR")
    }
    def override preVisitExprMakeBlock(expr : ExprMakeBlock?) {
        emit("MK_BLK")
    }
    def override preVisitExprMakeGenerator(expr : ExprMakeGenerator?) {
        emit("MK_GEN")
    }
    def override preVisitExprArrayComprehension(expr : ExprArrayComprehension?) {
        emit("ARR_COMP")
    }
    def override preVisitExprStringBuilder(expr : ExprStringBuilder?) {
        emit("SBLD")
    }

// type info
    def override preVisitExprTypeInfo(expr : ExprTypeInfo?) {
        emit("TYPEINFO")
    }
    def override preVisitExprTypeDecl(expr : ExprTypeDecl?) {
        emit("EXPR_TYPE")
    }

// looks-like-call family. Keep the called function name — it carries
// real signal (`push`, `delete`, `assert`, ...) and is not a user
// identifier we want to alpha-rename.
    def override preVisitExprLooksLikeCall(expr : ExprLooksLikeCall?) {
        emit("CALL:{expr.name}")
    }
    def override preVisitExprCall(expr : ExprCall?) {
        emit("CALL:{expr.name}")
    }
    def override preVisitExprNamedCall(expr : ExprNamedCall?) {
        emit("NCALL:{expr.name}")
    }
    def override preVisitExprCallMacro(expr : ExprCallMacro?) {
        emit("CALL_MACRO:{expr.name}")
    }
    def override preVisitExprAssert(expr : ExprAssert?) {
        emit("ASSERT")
    }
    def override preVisitExprStaticAssert(expr : ExprStaticAssert?) {
        emit("STATIC_ASSERT")
    }
    def override preVisitExprQuote(expr : ExprQuote?) {
        emit("QUOTE")
    }
    def override preVisitExprDebug(expr : ExprDebug?) {
        emit("DEBUG")
    }
    def override preVisitExprInvoke(expr : ExprInvoke?) {
        emit("INVOKE")
    }
    def override preVisitExprErase(expr : ExprErase?) {
        emit("ERASE")
    }
    def override preVisitExprFind(expr : ExprFind?) {
        emit("FIND")
    }
    def override preVisitExprKeyExists(expr : ExprKeyExists?) {
        emit("KEY_EXISTS")
    }
    def override preVisitExprSetInsert(expr : ExprSetInsert?) {
        emit("SET_INSERT")
    }
    def override preVisitExprMemZero(expr : ExprMemZero?) {
        emit("MEMZERO")
    }

// var
    def override preVisitExprVar(expr : ExprVar?) {
        let p = lookup_var(expr.name)
        emit(p)
    }

// field access (post-visit so the receiver is emitted first)
    def override visitExprField(var expr : ExprField?) : ExpressionPtr {
        emit(".FLD")
        return unsafe(reinterpret<ExpressionPtr>(expr))
    }
    def override visitExprSafeField(var expr : ExprSafeField?) : ExpressionPtr {
        emit("?.FLD")
        return unsafe(reinterpret<ExpressionPtr>(expr))
    }
    def override visitExprSwizzle(var expr : ExprSwizzle?) : ExpressionPtr {
        emit(".SWZ")
        return unsafe(reinterpret<ExpressionPtr>(expr))
    }

// constants — every numeric / string / bool collapses to <lit>.
// Keep enum literals separate so cross-enum confusion is detectable.
    def override preVisitExprConst(expr : ExprConst?) {
        pass
    }
    def override preVisitExprConstPtr(expr : ExprConstPtr?) {
        emit("LIT")
    }
    def override preVisitExprConstInt8(expr : ExprConstInt8?) {
        emit("LIT")
    }
    def override preVisitExprConstInt16(expr : ExprConstInt16?) {
        emit("LIT")
    }
    def override preVisitExprConstInt(expr : ExprConstInt?) {
        emit("LIT")
    }
    def override preVisitExprConstInt64(expr : ExprConstInt64?) {
        emit("LIT")
    }
    def override preVisitExprConstInt2(expr : ExprConstInt2?) {
        emit("LIT")
    }
    def override preVisitExprConstInt3(expr : ExprConstInt3?) {
        emit("LIT")
    }
    def override preVisitExprConstInt4(expr : ExprConstInt4?) {
        emit("LIT")
    }
    def override preVisitExprConstUInt8(expr : ExprConstUInt8?) {
        emit("LIT")
    }
    def override preVisitExprConstUInt16(expr : ExprConstUInt16?) {
        emit("LIT")
    }
    def override preVisitExprConstUInt(expr : ExprConstUInt?) {
        emit("LIT")
    }
    def override preVisitExprConstUInt64(expr : ExprConstUInt64?) {
        emit("LIT")
    }
    def override preVisitExprConstUInt2(expr : ExprConstUInt2?) {
        emit("LIT")
    }
    def override preVisitExprConstUInt3(expr : ExprConstUInt3?) {
        emit("LIT")
    }
    def override preVisitExprConstUInt4(expr : ExprConstUInt4?) {
        emit("LIT")
    }
    def override preVisitExprConstFloat(expr : ExprConstFloat?) {
        emit("LIT")
    }
    def override preVisitExprConstFloat2(expr : ExprConstFloat2?) {
        emit("LIT")
    }
    def override preVisitExprConstFloat3(expr : ExprConstFloat3?) {
        emit("LIT")
    }
    def override preVisitExprConstFloat4(expr : ExprConstFloat4?) {
        emit("LIT")
    }
    def override preVisitExprConstDouble(expr : ExprConstDouble?) {
        emit("LIT")
    }
    def override preVisitExprConstBool(expr : ExprConstBool?) {
        emit("LIT")
    }
    def override preVisitExprConstString(expr : ExprConstString?) {
        emit("LIT")
    }
    def override preVisitExprConstRange(expr : ExprConstRange?) {
        emit("LIT")
    }
    def override preVisitExprConstURange(expr : ExprConstURange?) {
        emit("LIT")
    }
    def override preVisitExprConstRange64(expr : ExprConstRange64?) {
        emit("LIT")
    }
    def override preVisitExprConstURange64(expr : ExprConstURange64?) {
        emit("LIT")
    }
    def override preVisitExprConstBitfield(expr : ExprConstBitfield?) {
        emit("LIT")
    }
    def override preVisitExprConstEnumeration(expr : ExprConstEnumeration?) {
        emit("ENUM_LIT")
    }

// reader / fake
    def override preVisitExprReader(expr : ExprReader?) {
        emit("READER")
    }
    def override preVisitExprFakeContext(expr : ExprFakeContext?) {
        emit("FAKE_CTX")
    }
    def override preVisitExprFakeLineInfo(expr : ExprFakeLineInfo?) {
        emit("FAKE_LI")
    }
    def override preVisitExprTag(expr : ExprTag?) {
        emit("TAG")
    }
}


// canonicalize_function walks `fn` and returns its canonical text.
def public canonicalize_function(fn : FunctionPtr) : string {
    return build_string() $(var sb) {
        var v = new CanonicalVisitor(writer = unsafe(addr(sb)))
        make_visitor(*v) $(adapter) {
            visit(fn, adapter)
        }
        unsafe { delete v; }
    }
}


// tokenize_canonical splits a canonical text into a flat token list.
// `emit()` writes each tag followed by exactly one space, so plain
// split(" ") + drop-empty is correct and O(n).
def public tokenize_canonical(text : string) : array<string> {
    var raw <- text |> split(" ")
    var out : array<string>
    out |> reserve(length(raw))
    for (t in raw) {
        if (!empty(t)) {
            out |> push(t)
        }
    }
    return <- out
}
