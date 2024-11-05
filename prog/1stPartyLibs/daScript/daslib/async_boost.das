options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module async_boost shared private

require strings
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost


def public await_next_frame()
    //! This function is used to suspend coroutine until next frame.
    panic("await_next_frame() should not be called directly, add [async] annotation to the function in that you want to use it")

[tag_function(await_tag)]
def public await(var a : iterator<variant<res : auto(T); wait : bool>>) : T
    //! This function is used to wait for the result of the async function.
    panic("await( ) should not be called directly, add [async] annotation to the function in that you want to use it")
    var res : T
    return <- res

[tag_function_macro(tag="await_tag")]
class AwaitMacro : AstFunctionAnnotation
    def override transform(var call : smart_ptr<ExprCallFunc>; var errors : das_string) : ExpressionPtr
        errors := "result should be assigned to a variable, use 'let res <- await <| {call.arguments[0] |> describe()}'"
        return <- [[ExpressionPtr]]


[tag_function(await_coroutine_tag)]
def public await(var a : iterator<bool>) : bool
    //! This function is used to wait for the result of the async function.
    panic("await() should not be called directly, add [async] annotation to the function in that you want to use it")
    return false

[tag_function_macro(tag="await_coroutine_tag")]
class AwaitCoroutineMacro : AstFunctionAnnotation
    //! This macro converts await(<coroutine>) expression into::
    //!
    //!     for t in THAT
    //!         yield t
    //!
    //! The idea is that coroutine or generator can continuesly yield from another sub-coroutine or generator.
    def override transform(var call : smart_ptr<ExprCallFunc>; var errors : das_string) : ExpressionPtr
        let iname = make_unique_private_name("_await_coroutine", call.at)
        var inscope res <- qmacro_block <|
            for $i(iname) in $e(call.arguments[0])
                yield $i(iname)
        res |> force_at(call.at)
        return <- res


[function_macro(name="async")]
class AsyncMacro : AstFunctionAnnotation
    //! This macro converts function into generator.
    //! Generator yields bool if its a void function (coroutine), and yields the return type otherwise (async return).
    //! async function can wait for another async function using await(<async fn call>).
    //! use 'return false' to immediately return from the generator.
    [unused_argument(group, args, errors)] def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
        if func.result.baseType == Type autoinfer
            errors := "async function should have explicit return type"
            return false
        var inscope retT : TypeDeclPtr
        var voidRoutine = true
        if func.result.baseType == Type tVoid
            move_new(retT) <| qmacro_type(type<bool>)
        else
            voidRoutine = false
            retT |> move_new() <| new [[TypeDecl() baseType=Type tVariant, at=func.at]]
            retT.argNames |> resize(2)
            retT.argNames[0] := "res"
            retT.argNames[1] := "wait"

            retT.argTypes |> resize(2)
            retT.argTypes[0] |> move_new() <| clone_type(func.result)
            retT.argTypes[1] |> move_new() <| typeinfo(ast_typedecl type<bool>)

        var astVisitor = new CollectAndReplaceIteratorFields()
        astVisitor.voidRoutine = voidRoutine
        astVisitor.retType := retT
        var inscope astVisitorAdapter <- make_visitor(*astVisitor)
        visit(func.body, astVisitorAdapter)
        delete astVisitor.retType
        errors := astVisitor.errors
        unsafe
            delete astVisitor

        if length(errors) > 0
            return false

        var inscope blk <- qmacro_expr <|
            return <- generator<$t(retT)> () <|
                $e(func.body)
                return false

        blk.at := func.at
        func.body |> move() <| blk
        func.result |> move_new() <| qmacro_type(type<iterator<$t(retT)>>)
        return true

[macro_function]
def validateAwaitFn(fn : smart_ptr<ast::Function>&; subCallName : das_string; var errors : string&) : bool
    if fn == null
        errors = "{errors}\nexpecting await(<async fn>), but function {subCallName} is not found, make sure it is defined before the coroutine"
        return false
    if fn.result.baseType != Type tIterator
        errors = "{errors}\nexpecting await(<async fn>), add [async] annotation to the function {subCallName}"
        return false
    if fn.result.firstType.baseType != Type tVariant
        errors = "{errors}\nexpecting await(<async fn>), add [async] annotation to the function {subCallName}"
        return false
    if fn.result.firstType.argTypes |> length() != 2
        errors = "{errors}\nexpecting await(<async fn>), add [async] annotation to the function {subCallName}"
        return false
    return true

enum AssignOp
    copy
    move
    clone

[macro_function]
def move_clone_copy(var expr; voidRoutine : bool; var retType : smart_ptr<TypeDecl>&; var errors : string&; assignOp : AssignOp) : ExpressionPtr
    if expr.right is ExprCall
        var call = expr.right as ExprCall
        if call.name == "await"
            if call.arguments[0] is ExprCall
                var subCall = call.arguments[0] as ExprCall
                var inscope fn <- compiling_module() |> find_unique_function("{subCall.name}")
                if !validateAwaitFn(fn, subCall.name, errors)
                    return <- expr

                let name = make_unique_private_name("_async_iterator", expr.at)
                let useMove = assignOp == AssignOp move
                let useClone = assignOp == AssignOp clone
                var inscope blk <- qmacro_block <|
                    for $i(name) in $e(call.arguments[0])
                        if $i(name) is res
                            if $v(useMove)
                                $e(expr.left) <- $i(name) as res
                            elif $v(useClone)
                                $e(expr.left) := $i(name) as res
                            else
                                $e(expr.left) = $i(name) as res
                        else
                            if $v(voidRoutine)
                                yield true
                            else
                                yield <- [[$t(retType) wait=true]]
                var exprBlock = blk as ExprBlock
                blk |> force_at(expr.at)
                return <- blk
    return <- expr

[macro_interface]
class CollectAndReplaceIteratorFields : AstVisitor
    voidRoutine : bool
    retType : smart_ptr<TypeDecl>
    errors : string

    [[do_not_delete]] private blocksStack : array<ExprBlock?>


    def override visitExprYield(var expr : smart_ptr<ExprYield>) : ExpressionPtr
        if voidRoutine
            return <- expr
        if expr._type != null && expr._type |> is_same_type(retType, RefMatters no, ConstMatters no, TemporaryMatters no)
            return <- expr
        if !retType.argTypes[0].canCopy
            var inscope res <- qmacro_expr(${ yield <- [[$t(retType) res <- $e(expr.subexpr)]]; })
            res |> force_at(expr.at)
            return <- res

        var inscope res <- qmacro_expr(${ yield <- [[$t(retType) res = $e(expr.subexpr)]]; })
        res |> force_at(expr.at)
        return <- res

    def override visitExprCopy(var expr:smart_ptr<ExprCopy>) : ExpressionPtr
        return <- move_clone_copy(expr, voidRoutine, retType, errors, AssignOp copy)

    def override visitExprMove(var expr:smart_ptr<ExprMove>) : ExpressionPtr
        return <- move_clone_copy(expr, voidRoutine, retType, errors, AssignOp move)

    def override visitExprClone(var expr:smart_ptr<ExprClone>) : ExpressionPtr
        return <- move_clone_copy(expr, voidRoutine, retType, errors, AssignOp clone)

    def override visitExprCall(var expr : smart_ptr<ExprCall>) : ExpressionPtr
        if expr.name == "await_next_frame"
            if voidRoutine
                var inscope res <- qmacro_expr(${ yield true; })
                res |> force_at(expr.at)
                return <- res
            else
                var inscope res <- qmacro_expr(${ yield <- [[$t(retType) wait=true]]; })
                res |> force_at(expr.at)
                return <- res
        return <- expr

    def override preVisitExprBlock(blk:smart_ptr<ExprBlock>) : void
        blocksStack |> push(blk |> get_ptr())

    def override visitExprBlock(var blk : smart_ptr<ExprBlock>) : ExpressionPtr
        blocksStack |> erase(length(blocksStack) - 1)
        return <- blk

    def override visitExprLet(var expr : smart_ptr<ExprLet>) : ExpressionPtr
        if length(expr.variables) != 1
            return <- expr
        for v in expr.variables
            if v.init != null && v.init is ExprCall
                var call = v.init as ExprCall
                if call.name == "await"
                    if v._type == null || v._type.baseType == Type autoinfer
                        if !(call.arguments[0] is ExprCall)
                            errors = "{errors}\nexpecting await(<async fn call>), got await(<non-call>)"
                            return <- expr

                        var subCall = call.arguments[0] as ExprCall
                        var inscope fn <- compiling_module() |> find_unique_function("{subCall.name}")
                        if !validateAwaitFn(fn, subCall.name, errors)
                            return <- expr

                        v._type |> move_new() <| clone_type(fn.result.firstType.argTypes[0])

                    var name = make_unique_private_name("_async_iterator", expr.at)
                    let useMove = v.flags.init_via_move
                    let useClone = v.flags.init_via_clone
                    var inscope blk <- qmacro_block <|
                        var $i(v.name) : $t(v._type)
                        for $i(name) in $e(call.arguments[0])
                            if $i(name) is res
                                if $v(useMove)
                                    $i(v.name) <- $i(name) as res
                                elif $v(useClone)
                                    $i(v.name) := $i(name) as res
                                else
                                    $i(v.name) = $i(name) as res
                            else
                                if $v(voidRoutine)
                                    yield true
                                else
                                    yield <- [[$t(retType) wait=true]]
                    var exprBlock = blk as ExprBlock
                    blk |> force_at(expr.at)

                    var idx = length(blocksStack) - 1
                    verify(idx >= 0)
                    while idx >= 0
                        var parentBlock = blocksStack[idx]
                        if !parentBlock.blockFlags.isClosure
                            parentBlock.blockFlags |= ExprBlockFlags needCollapse
                            break
                        idx--

                    return <- blk
        return <- expr


def public async_run(var a : iterator<auto>)
    //! This function runs async function until it is finished.
    for t in a
        pass


def public async_run_all(var a : array<iterator<auto>>)
    //! This function runs all async function until they are finished (in parallel, starting from the last one).
    while true
        var i = length(a)
        if i == 0
            break
        while i > 0
            i --
            var t : bool
            next(a[i], t)
            if empty(a[i])
                delete a[i]
                a |> erase(i)
