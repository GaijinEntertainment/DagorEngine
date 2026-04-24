options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module async_boost shared private

//! Async/await coroutine macros.
//!
//! Provides ``[async]`` function annotation and ``await`` call macro that
//! rewrite annotated functions into generator-based coroutines.

require strings
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost


def public await_next_frame() {
    //! This function is used to suspend coroutine until next frame.
    panic("await_next_frame() should not be called directly, add [async] annotation to the function in that you want to use it")
}

[tag_function(await_tag)]
def public await(var a : iterator<variant<res : auto(T); wait : bool>>) : T {
    //! This function is used to wait for the result of the async function.
    panic("await( ) should not be called directly, add [async] annotation to the function in that you want to use it")
    var res : T
    return <- res
}

[tag_function_macro(tag="await_tag")]
class AwaitMacro : AstFunctionAnnotation {
    //! Function annotation that implements coroutine await semantics.
    def override transform(var call : smart_ptr<ExprCallFunc>; var errors : das_string) : ExpressionPtr {
        macro_error(compiling_program(), call.at, "result should be assigned to a variable, use 'let res <- await <| {call.arguments[0] |> describe()}'")
        return <- default<ExpressionPtr>
    }
}


[tag_function(await_coroutine_tag)]
def public await(var a : iterator<bool>) : bool {
    //! This function is used to wait for the result of the async function.
    panic("await() should not be called directly, add [async] annotation to the function in that you want to use it")
    return false
}

[tag_function_macro(tag="await_coroutine_tag")]
class AwaitCoroutineMacro : AstFunctionAnnotation {
    //! This macro converts await(<coroutine>) expression into::
    //!
    //!     for t in THAT
    //!         yield t
    //!
    //! The idea is that coroutine or generator can continuously yield from another sub-coroutine or generator.
    def override transform(var call : smart_ptr<ExprCallFunc>; var errors : das_string) : ExpressionPtr {
        let iname = make_unique_private_name("_await_coroutine", call.at)
        var inscope res <- qmacro_block() {
            for ($i(iname) in $e(call.arguments[0])) {
                yield $i(iname)
            }
        }
        res |> force_at(call.at)
        return <- res
    }
}

[function_macro(name="async")]
class AsyncMacro : AstFunctionAnnotation {
    //! This macro converts function into generator.
    //! Generator yields bool if its a void function (coroutine), and yields the return type otherwise (async return).
    //! async function can wait for another async function using await(<async fn call>).
    //! use 'return false' to immediately return from the generator.
    [unused_argument(group, args)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        if (func.result.baseType == Type.autoinfer) {
            macro_error(compiling_program(), func.at, "async function should have explicit return type")
            return true
        }
        var inscope retT : TypeDeclPtr
        var voidRoutine = true
        if (func.result.baseType == Type.tVoid) {
            move_new(retT) <| qmacro_type(type<bool>)
        } else {
            voidRoutine = false
            retT |> move_new() <| new TypeDecl(baseType = Type.tVariant, at = func.at)
            retT.argNames |> resize(2)
            retT.argNames[0] := "res"
            retT.argNames[1] := "wait"

            retT.argTypes |> resize(2)
            retT.argTypes[0] |> move_new() <| clone_type(func.result)
            retT.argTypes[1] |> move_new() <| typeinfo ast_typedecl(type<bool>)
        }

        var astVisitor = new CollectAndReplaceIteratorFields()
        astVisitor.voidRoutine = voidRoutine
        astVisitor.retType := retT
        var inscope astVisitorAdapter <- make_visitor(*astVisitor)
        visit(func.body, astVisitorAdapter)
        delete astVisitor.retType
        unsafe {
            delete astVisitor
        }

        var inscope blk <- qmacro_expr() {
            return <- generator<$t(retT)>() <| $() {
                $e(func.body)
                return false
            }
        }

        var ret = blk as ExprReturn
        var gen = ret.subexpr as ExprMakeGenerator
        for (arg in func.arguments) {
            if (arg._type.baseType == Type.autoinfer) {
                macro_error(compiling_program(), arg.at, "async function argument '{arg.name}' should have explicit type")
                return true
            }

            if (!arg._type.canCopy) {
                let capturesNum = length(gen._capture)
                gen._capture.resize(capturesNum + 1)
                gen._capture[capturesNum].name := arg.name
                let mode = arg._type.canMove ? CaptureMode.capture_by_move : CaptureMode.capture_by_clone
                gen._capture[capturesNum].mode = mode
            }
        }

        blk.at := func.at
        func.body |> move() <| blk
        func.result |> move_new() <| qmacro_type(type<iterator<$t(retT)>>)
        return true
    }
}

[macro_function]
def validateAwaitFn(fn : smart_ptr<ast::Function>&; subCallName : das_string; var errors : string&) : bool {
    if (fn == null) {
        errors = "{errors}\nexpecting await(<async fn>), but function {subCallName} is not found, make sure it is defined before the coroutine"
        return false
    }
    if (fn.result.baseType != Type.tIterator) {
        errors = "{errors}\nexpecting await(<async fn>), add [async] annotation to the function {subCallName}"
        return false
    }
    if (fn.result.firstType.baseType != Type.tVariant) {
        errors = "{errors}\nexpecting await(<async fn>), add [async] annotation to the function {subCallName}"
        return false
    }
    if (fn.result.firstType.argTypes |> length() != 2) {
        errors = "{errors}\nexpecting await(<async fn>), add [async] annotation to the function {subCallName}"
        return false
    }
    return true
}

enum AssignOp {
    //! Assignment operation type for async field updates.
    copy
    move
    clone
}

[macro_function]
def move_clone_copy(var expr; voidRoutine : bool; var retType : smart_ptr<TypeDecl>&; assignOp : AssignOp) : ExpressionPtr {
    if (expr.right is ExprCall) {
        var call = expr.right as ExprCall
        if (call.name == "await") {
            if (call.arguments[0] is ExprCall) {
                var subCall = call.arguments[0] as ExprCall
                var inscope fn <- compiling_module() |> find_unique_function("{subCall.name}")
                var error = ""
                if (!validateAwaitFn(fn, subCall.name, error)) {
                    macro_error(compiling_program(), expr.at, error)
                    return <- expr
                }

                let name = make_unique_private_name("_async_iterator", expr.at)
                let useMove = assignOp == AssignOp.move
                let useClone = assignOp == AssignOp.clone
                var inscope blk <- qmacro_block() {
                    for ($i(name) in $e(call.arguments[0])) {
                        if ($i(name) is res) {
                            if ($v(useMove)) {
                                $e(expr.left) <- $i(name) as res
                            } elif ($v(useClone)) {
                                $e(expr.left) := $i(name) as res
                            } else {
                                $e(expr.left) = $i(name) as res
                            }
                        }
                        if ($v(voidRoutine)) {
                            yield true
                        } else {
                            yield <- variant type<$t(retType)>(wait = true)
                        }
                    }
                }
                blk |> force_at(expr.at)
                return <- blk
            }
        }
    }
    return <- expr
}

[macro_interface]
class CollectAndReplaceIteratorFields : AstVisitor {
    //! AST visitor that collects and replaces iterator field access in async generators.
    voidRoutine : bool
    retType : smart_ptr<TypeDecl>

    @do_not_delete private blocksStack : array<ExprBlock?>


    def override visitExprYield(var expr : smart_ptr<ExprYield>) : ExpressionPtr {
        if (voidRoutine) {
            return <- expr
        }
        if (expr._type != null && expr._type |> is_same_type(retType, RefMatters.no, ConstMatters.no, TemporaryMatters.no)) {
            return <- expr
        }
        if (!retType.argTypes[0].canCopy) {
            var inscope res <- qmacro_expr(${ yield <- variant type<$t(retType)>(res <- $e(expr.subexpr)); })
            res |> force_at(expr.at)
            return <- res
        }

        var inscope res <- qmacro_expr(${ yield <- variant type<$t(retType)>(res = $e(expr.subexpr)); })
        res |> force_at(expr.at)
        return <- res
    }

    def override visitExprCopy(var expr : smart_ptr<ExprCopy>) : ExpressionPtr {
        return <- move_clone_copy(expr, voidRoutine, retType, AssignOp.copy)
    }

    def override visitExprMove(var expr : smart_ptr<ExprMove>) : ExpressionPtr {
        return <- move_clone_copy(expr, voidRoutine, retType, AssignOp.move)
    }

    def override visitExprClone(var expr : smart_ptr<ExprClone>) : ExpressionPtr {
        return <- move_clone_copy(expr, voidRoutine, retType, AssignOp.clone)
    }

    def override visitExprCall(var expr : smart_ptr<ExprCall>) : ExpressionPtr {
        if (expr.name == "await_next_frame") {
            if (voidRoutine) {
                var inscope res <- qmacro_expr(${ yield true; })
                res |> force_at(expr.at)
                return <- res
            } else {
                var inscope res <- qmacro_expr(${ yield <- variant type<$t(retType)>(wait = true); })
                res |> force_at(expr.at)
                return <- res
            }
        }
        return <- expr
    }

    def override preVisitExprBlock(var blk : smart_ptr<ExprBlock>) : void {
        blocksStack |> push(blk |> get_ptr())
    }

    def override visitExprBlock(var blk : smart_ptr<ExprBlock>) : ExpressionPtr {
        blocksStack |> erase(length(blocksStack) - 1)
        return <- blk
    }

    def override visitExprLet(var expr : smart_ptr<ExprLet>) : ExpressionPtr {
        if (length(expr.variables) != 1) {
            return <- expr
        }
        for (v in expr.variables) {
            if (v.init != null && v.init is ExprCall) {
                var call = v.init as ExprCall
                if (call.name == "await") {
                    if (v._type == null || v._type.baseType == Type.autoinfer) {
                        if (!(call.arguments[0] is ExprCall)) {
                            macro_error(compiling_program(), expr.at, "expecting await(<async fn call>), got await(<non-call>)")
                            return <- expr
                        }

                        var subCall = call.arguments[0] as ExprCall
                        var inscope fn <- compiling_module() |> find_unique_function("{subCall.name}")
                        if (fn == null) {
                            macro_error(compiling_program(), expr.at, "expecting await(<async fn>), but function {subCall.name} is not found,\ndefine variable type explicitly: let res : <type> = await <| {subCall.name}()\nor make sure the function is defined before the coroutine")
                            return <- expr
                        } else {
                            var error = ""
                            if (!validateAwaitFn(fn, subCall.name, error)) {
                                macro_error(compiling_program(), expr.at, error)
                                return <- expr
                            }

                            v._type |> move_new() <| clone_type(fn.result.firstType.argTypes[0])
                        }
                    } else {
                        var inscope rmConst = clone_type(v._type)
                        rmConst.flags &= ~TypeDeclFlags.constant
                        rmConst.flags &= ~TypeDeclFlags.removeConstant
                        v._type |> move(rmConst)
                    }

                    let name = make_unique_private_name("_async_iterator", expr.at)
                    let useMove = v.flags.init_via_move
                    let useClone = v.flags.init_via_clone
                    var inscope blk <- qmacro_block() {
                        var $i(v.name) : $t(v._type)
                        for ($i(name) in $e(call.arguments[0])) {
                            if ($i(name) is res) {
                                if ($v(useMove)) {
                                    $i(v.name) <- $i(name) as res
                                } elif ($v(useClone)) {
                                    $i(v.name) := $i(name) as res
                                } else {
                                    $i(v.name) = $i(name) as res
                                }
                            }
                            if ($v(voidRoutine)) {
                                yield true
                            } else {
                                yield <- variant type<$t(retType)>(wait = true)
                            }
                        }
                    }
                    blk |> force_at(expr.at)

                    var idx = length(blocksStack) - 1
                    verify(idx >= 0)
                    while (idx >= 0) {
                        var parentBlock = blocksStack[idx]
                        if (!parentBlock.blockFlags.isClosure) {
                            parentBlock.blockFlags |= ExprBlockFlags.needCollapse
                            break
                        }
                        idx--
                    }

                    return <- blk
                }
            }
        }
        return <- expr
    }
    def override visitExprReturn(var expr : smart_ptr<ExprReturn>) : ExpressionPtr {
        if (length(blocksStack) <= 1 && expr.subexpr != null && describe(expr.subexpr) != "false") {
            macro_error(compiling_program(), expr.at, "avoid return expression, use 'return false' to break execution, use 'yield [value]' or await_next_frame() to yield coroutine execution")
        }
        return <- expr
    }
}

def public async_run(var a : iterator<auto>) {
    //! This function runs async function until it is finished.
    for (t in a) {
        pass
    }
}


def public async_run_all(var a : array<iterator<auto>>) {
    //! This function runs all async function until they are finished (in parallel, starting from the last one).
    while (true) {
        var i = length(a)
        if (i == 0) {
            break
        }
        while (i > 0) {
            i --
            var t : bool
            next(a[i], t)
            if (empty(a[i])) {
                delete a[i]
                a |> erase(i)
            }
        }
    }
}


def public async_timeout(var a : iterator<auto>; max_frames : int) : bool {
    //! This function runs an async function for at most `max_frames` frames.
    //! Returns ``true`` if the async function completed within the limit,
    //! ``false`` if it was terminated due to timeout.
    var frames = 0
    for (t in a) {
        frames ++
        if (frames >= max_frames) {
            return false
        }
    }
    return true
}


def public async_race(var a : iterator<auto>; var b : iterator<auto>) : int {
    //! This function runs two async functions concurrently and returns the
    //! index (0 or 1) of whichever finishes first. The other is abandoned.
    var ta : bool
    var tb : bool
    while (true) {
        if (!next(a, ta) || empty(a)) {
            return 0
        }
        if (!next(b, tb) || empty(b)) {
            return 1
        }
    }
    return -1
}
