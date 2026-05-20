options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module lpipe shared private

//! Left-pipe macro.
//!
//! ``lpipe`` allows piping an additional block argument to the previous call
//! on the preceding line. Useful when a function takes multiple block arguments
//! and only ``<|`` can be used for the first one.

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

def lpipe_expr(var fnCall : ExpressionPtr&; var arg : ExpressionPtr&) : ExpressionPtr {
    if (fnCall is ExprVar) {
        var pVar = fnCall as ExprVar
        var pCall = make_call(pVar.at, "{pVar.name}")
        fnCall = null
        var pCallFunc = unsafe(reinterpret<ExprLooksLikeCall?> pCall)
        pCallFunc.arguments |> emplace(arg)
        return pCall
    } elif (fnCall is ExprCopy) {
        var cCopy = clone_expression(fnCall)
        (cCopy as ExprCopy).right = lpipe_expr((fnCall as ExprCopy).right, arg)
        return cCopy
    } elif (fnCall is ExprMove) {
        var cCopy = clone_expression(fnCall)
        (cCopy as ExprMove).right = lpipe_expr((fnCall as ExprMove).right, arg)
        return cCopy
    } elif (fnCall is ExprClone) {
        var cCopy = clone_expression(fnCall)
        (cCopy as ExprClone).right = lpipe_expr((fnCall as ExprClone).right, arg)
        return cCopy
    } elif (fnCall is ExprLet) {
        var pLet = fnCall as ExprLet
        if (length(pLet.variables) != 1 || pLet.variables[0].init == null) {
            arg = null
            return null
        }
        var cLet = clone_expression(fnCall)
        (cLet as ExprLet).variables[0].init = lpipe_expr(pLet.variables[0].init, arg)
        return cLet
    } elif (fnCall |> is_expr_like_call()) {
        var pCall = unsafe(reinterpret<ExprLooksLikeCall?> fnCall)
        if (pCall.name == "lpipe") {// not piping pipe into pipe
            return null
        }
        pCall.arguments |> emplace(arg)
        return fnCall
    } else {
        arg = null
        return null
    }
}


[call_macro(name="lpipe")]
class LpipeMacro : AstCallMacro {
    //! This macro will implement the lpipe function. It allows piping blocks the previous line call. For example::
    //!
    //!     def take2(a,b:block)
    //!         invoke(a)
    //!         invoke(b)
    //!     ...
    //!     take2 <|
    //!         print("block1\n")
    //!     lpipe <|    // this block will pipe into take2
    //!         print("block2\n")
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : ExpressionPtr {
        macro_verify(call.arguments |> length == 1, prog, call.at, "expecting lpipe(value)")
        var success = false
        compiling_program() |> get_ast_context(call) $(valid, astc) {
            if (!valid) {
                macro_error(prog, call.at, "can't get valid program context")
                return
            }
            if (astc.scopes |> length <= 0) {
                macro_error(prog, call.at, "missing scope")
                return
            }
            var blk = astc.scopes[astc.scopes |> length - 1] as ExprBlock
            var eidx = -1
            for (i, expr in count(), blk.list) {
                if (expr == call) {
                    eidx = i
                    break
                }
            }
            if (eidx == -1) {
                macro_error(prog, call.at, "can't find lpipe in the block")
                return
            }
            if (eidx == 0) {
                macro_error(prog, call.at, "can't pipe first expression in the block")
                return
            }
            let pidx = eidx - 1
            var pipe = lpipe_expr(blk.list[pidx], call.arguments[0])
            if (pipe == null) {
                macro_error(prog, call.at, "can only pipe to a function call, and not {blk.list[pidx].__rtti}")
                return
            }
            blk.list[pidx] = pipe
            blk.list[eidx] = null
            success = true
        }
        if (!success) {
            return null
        }
        return quote() {
            pass
        }
    }
}
