options gen2
options indenting = 4

options strict_smart_pointers

module with_boost shared private

//! ``with_`` call macro: bind one or more array / table element references
//! inside a block, with an automatic ``lock`` around the body so
//! push/erase/resize/clear panic at runtime instead of silently dangling.
//! The macro emits the full lock / invoke / unlock sequence inline — no
//! helper functions — so any arity and any mix of array / table args works
//! uniformly. See ``tutorials/macros/18_with_boost.das`` for examples.
//!
//! Restrictions:
//!
//! * Each container arg must be an ``ExprAt`` (``arr[i]`` / ``tab[k]``). Plain
//!   locals, struct fields on locals, and function-call results are refused —
//!   use built-in ``with`` for those.
//! * At most one table-keyed arg per call (a second insert/erase would
//!   rehash and invalidate the pinned entry).
//! * Block bodies are void (no ``return`` value); ``with_`` is for in-place
//!   mutation. To compute a value, write to a local:
//!   ``var v : T; with_(arr[0]) { v = _.f }``.

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

// ──────────────────────────────────────────────────────────────────────────────
// Call macro
// ──────────────────────────────────────────────────────────────────────────────

def private container_kind(atNode : ExprAt?) : string {
    //! Returns "array", "table", or "" if neither.
    let subT = atNode.subexpr._type
    return "" if (subT == null)
    return "array" if (subT.baseType == Type.tArray)
    return "table" if (subT.baseType == Type.tTable)
    return ""
}

def private is_lvalue_chain(e : ExpressionPtr) : bool {
    //! True if e is reachable from an ExprVar root through ExprField
    //! hops only — i.e. binding `var x & = e` is safe (after
    //! `unsafe(...)`) because e refers to addressable storage outside
    //! this expression AND the chain doesn't contain a nested container
    //! that the macro can't lock. ExprAt / ExprSafeAt are deliberately
    //! NOT followed: `outer[i].innerArr[j]` would lock only `innerArr`
    //! while `outer` stayed mutable, leaving the inner ref vulnerable
    //! to outer-array reallocation from inside the body.
    return false if (e == null)
    return true if (e is ExprVar)
    return is_lvalue_chain((e as ExprField).value) if (e is ExprField)
    return is_lvalue_chain((e as ExprSafeField).value) if (e is ExprSafeField)
    return false
}

[call_macro(name="with_")]
class private WithMacro : AstCallMacro {
    //! Implements the ``with_`` call macro. Each container arg must be an
    //! ``ExprAt`` (``arr[i]`` / ``tab[k]``); at most one table-keyed arg
    //! per call. Emits a pre-bind / lock / invoke / unlock sequence inline
    //! per container, with block-param types pinned to ``T &`` (element
    //! ref) so workhorse and struct elements mutate uniformly.
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : ExpressionPtr {
        macro_verify(length(call.arguments) >= 2, prog, call.at,
            "with_ needs at least one container arg plus a block")
        let totalArgs = length(call.arguments)
        let containerCount = totalArgs - 1
        let blockArg = call.arguments[totalArgs - 1]
        macro_verify(blockArg is ExprMakeBlock, prog, call.at,
            "with_ last argument must be a block, got {describe(blockArg)}")
        // Validate every container arg + count tables in one pass.
        var tableCount = 0
        for (i in range(containerCount)) {
            let ai = call.arguments[i]
            macro_verify(ai is ExprAt, prog, call.at,
                "with_ arg #{i} must be `arr[i]` or `tab[k]` — use built-in `with` for locals; got {describe(ai)}")
            let aiNode = ai as ExprAt
            macro_verify(aiNode.subexpr._type != null, prog, call.at,
                "with_ arg #{i} container type not inferred yet — got null _type on {describe(aiNode.subexpr)}")
            macro_verify(ai._type != null, prog, call.at,
                "with_ arg #{i} element type not inferred yet — got null _type on the ExprAt itself")
            let ki = container_kind(aiNode)
            macro_verify(ki != "", prog, call.at,
                "with_ arg #{i} container must be array or table, got {describe(aiNode.subexpr._type)}")
            macro_verify(is_lvalue_chain(aiNode.subexpr), prog, call.at,
                "with_ arg #{i} container must be a named variable (or field/index chain rooted in one); array literals and function-call results have temp lifetime and can't be safely ref-bound. Got {describe(aiNode.subexpr)}")
            if (ki == "table") {
                tableCount ++
            }
        }
        macro_verify(tableCount <= 1, prog, call.at,
            "with_ allows at most one table-keyed arg per call (got {tableCount}); a 2nd table lookup could rehash and invalidate the first pinned entry")
        // Build the rewritten block — every block param emerges as
        // parser-shaped (autoinfer, ref=false, removeConstant=true) so
        // daslang infers each name as `TT&` from the actual invoke arg
        // type (TT struct or workhorse).
        let mblk = blockArg as ExprMakeBlock
        let userBlock = mblk._block as ExprBlock
        let userArgCount = length(userBlock.arguments)
        var rewrittenBlock : ExprMakeBlock?
        var rewrittenInner : ExprBlock?
        // Per-param type: clone the container's element type (already a
        // ref after typing of `arr[i]` / `tab[k]`), strip const, force ref.
        // Inline ExprInvoke doesn't drive inference from arg → block-param,
        // so the macro pins each param to the concrete element-ref type
        // directly. Workhorse types pick up the explicit `&` here too.
        if (userArgCount == 0) {
            macro_verify(containerCount == 1, prog, call.at,
                "with_ multi-arg form requires explicit block params: $(a, b, ...)")
            var pT = clone_type(call.arguments[0]._type)
            pT.flags.ref = true
            pT.flags.constant = false
            pT.flags.removeConstant = true
            var injected = new Variable(at = call.at, name := "_", _type = pT)
            var newBlock = new ExprBlock(at = userBlock.at,
                returnType = new TypeDecl(at = userBlock.at, baseType = Type.tVoid),
                blockFlags = userBlock.blockFlags | ExprBlockFlags.isClosure)
            newBlock.arguments |> emplace_new(injected)
            for (st in userBlock.list) {
                newBlock.list |> emplace_new(clone_expression(st))
            }
            // Preserve any user-attached `finally { ... }` on the default-_ form
            // so its semantics match the explicit-params path (which clones the
            // whole block via clone_expression).
            for (st in userBlock.finalList) {
                newBlock.finalList |> emplace_new(clone_expression(st))
            }
            rewrittenBlock = new ExprMakeBlock(at = call.at, _block = newBlock)
            rewrittenInner = newBlock
        } else {
            macro_verify(userArgCount == containerCount, prog, call.at,
                "with_ block-param count must match container-arg count: {containerCount} containers, {userArgCount} block params")
            rewrittenBlock = clone_expression(mblk) as ExprMakeBlock
            rewrittenInner = rewrittenBlock._block as ExprBlock
            // Pin the block to `: void` so a stray `return value` in the user
            // body is rejected by the typer. The userArgCount==0 branch
            // already constructs newBlock with void returnType; we do the
            // same here so behavior matches across both shapes.
            rewrittenInner.returnType = new TypeDecl(at = userBlock.at, baseType = Type.tVoid)
            for (i in range(containerCount)) {
                var arg = rewrittenInner.arguments[i]
                arg._type = clone_type(call.arguments[i]._type)
                arg._type.flags.ref = true
                arg._type.flags.constant = false
                arg._type.flags.removeConstant = true
            }
        }
        // ── Inline-emission ────────────────────────────────────────────
        // For each container arg, pre-bind the subexpr to a local ref so
        // every downstream lock / invoke / unlock references the SAME
        // materialised value. Without this, splicing $e(subexpr) three
        // times (lock + invoke + unlock) would re-evaluate the subexpr
        // three times — fine for an ExprVar, fatal for a literal or call
        // (three different temp arrays → unlock hits stale memory).
        //
        // Per-container emission:
        //   var __c_<i> & = unsafe(<subexpr>)                    // always
        //   __builtin_<kind>_lock_mutable(__c_<i>)               // owned/temp alike
        //   for tables: var __tref_<i> & = unsafe(__c_<i>[<key>])
        //   <invoke argref>: __c_<i>[<idx>]  or  __tref_<i>
        //   __builtin_<kind>_unlock_mutable(__c_<i>)             // reversed at end
        var preStmts : array<ExpressionPtr>
        var lockStmts : array<ExpressionPtr>
        var unlockStmts : array<ExpressionPtr>
        var argRefs : array<ExpressionPtr>
        preStmts |> reserve(containerCount * 2)
        lockStmts |> reserve(containerCount)
        unlockStmts |> reserve(containerCount)
        argRefs |> reserve(containerCount)
        for (i in range(containerCount)) {
            let ai = call.arguments[i] as ExprAt
            let ki = container_kind(ai)
            // Hygienic names — make_unique_private_name guarantees no
            // collision with user-visible identifiers at this call site.
            let cName = make_unique_private_name("__with_c_{i}", call.at)
            preStmts |> push <| qmacro_expr() {
                var $i(cName) & = unsafe($e(ai.subexpr))
            }
            if (ki == "array") {
                lockStmts |> push <| qmacro_expr() {
                    __builtin_array_lock_mutable($i(cName))
                }
                unlockStmts |> push <| qmacro_expr() {
                    __builtin_array_unlock_mutable($i(cName))
                }
                argRefs |> push <| qmacro($i(cName)[$e(ai.index)])
            } else {  // table
                let trefName = make_unique_private_name("__with_tref_{i}", call.at)
                preStmts |> push <| qmacro_expr() {
                    var $i(trefName) & = unsafe($i(cName)[$e(ai.index)])
                }
                lockStmts |> push <| qmacro_expr() {
                    __builtin_table_lock_mutable($i(cName))
                }
                unlockStmts |> push <| qmacro_expr() {
                    __builtin_table_unlock_mutable($i(cName))
                }
                argRefs |> push <| qmacro($i(trefName))
            }
        }
        // Reverse unlock order so cleanup mirrors acquisition.
        let unlockN = length(unlockStmts)
        var unlocksReversed <- [for (i in range(unlockN)); unlockStmts[unlockN - 1 - i]]
        // Build the invoke statement via ExprInvoke (the dedicated AST node
        // for lambda/block calls — ExprCall("invoke") doesn't unify the
        // block's auto-typed params against the actual refs).
        var invokeCall = new ExprInvoke(at = call.at, name := "invoke")
        invokeCall.arguments |> emplace_new(rewrittenBlock)
        for (r in argRefs) {
            invokeCall.arguments |> emplace_new(r)
        }
        // Assemble the block: pre-resolves, locks, invoke, then unlocks
        // (reversed). No ``finally`` — daslang panic is fatal, not a C++
        // exception; if the invoke panics the process is exiting anyway
        // and skipped unlocks don't matter.
        var blk = new ExprBlock(at = call.at,
            returnType = new TypeDecl(at = call.at, baseType = Type.tVoid))
        for (s in preStmts) {
            blk.list |> emplace_new(s)
        }
        for (s in lockStmts) {
            blk.list |> emplace_new(s)
        }
        blk.list |> emplace_new(invokeCall)
        for (s in unlocksReversed) {
            blk.list |> emplace_new(s)
        }
        return <- blk
    }
}
