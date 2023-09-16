options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module ast_block_to_loop shared private

require daslib/ast_boost

class B2LVisitor : AstVisitor
    inClosure : int
    inArchetype : int
    failOnReturn : bool
    replaceReturnWithContinue : bool
    requireContinueCond : bool
    loop_depth : array<int>
    def B2LVisitor(fOnR,rRetWC,rCC:bool)
        failOnReturn = fOnR
        replaceReturnWithContinue = rRetWC
        requireContinueCond = rCC
        loop_depth |> push(0)
    def override visitExprReturn(var expr:smart_ptr<ExprReturn>) : ExpressionPtr
        if inClosure==0 && inArchetype==0
            if failOnReturn
                return <- make_static_assert_false("return is not allowed inside this macros's block", expr.at)
            elif replaceReturnWithContinue
                if expr.subexpr != null
                    return  <- new [[ExprIfThenElse() at=expr.at,
                        cond <- clone_expression(expr.subexpr),
                        if_true <- new [[ExprReturn() at=expr.at,
                                subexpr <- new [[ExprConstBool() at=expr.at, value:=true]]
                            ]],
                        if_false <- new [[ExprContinue() at=expr.at]]
                    ]]
                else
                    if requireContinueCond
                        macro_error(compiling_program(),expr.at,"expecting return boolean (continue or stop)")
                    else
                        return <- new [[ExprContinue() at=expr.at]]
        return <- expr
    def override preVisitExprFor(expr:smart_ptr<ExprFor>) : void
        loop_depth |> back ++
    def override visitExprFor(var expr:smart_ptr<ExprFor>) : ExpressionPtr
        loop_depth |> back --
        return <- expr
    def override preVisitExprWhile(expr:smart_ptr<ExprWhile>) : void
        loop_depth |> back ++
    def override visitExprWhile(var expr:smart_ptr<ExprWhile>) : ExpressionPtr
        loop_depth |> back --
        return <- expr
    def override preVisitExprBlock(blk:smart_ptr<ExprBlock>) : void
        if blk.blockFlags.isClosure
            inClosure ++
            loop_depth |> push(0)
    def override visitExprBlock(var blk:smart_ptr<ExprBlock>) : ExpressionPtr
        if blk.blockFlags.isClosure
            inClosure --
            loop_depth |> pop()
        return <- blk
    def override preVisitExprCall(expr:smart_ptr<ExprCall>): void
        if expr.name=="for_each_archetype"
            inArchetype ++
            loop_depth |> push(0)
    def override visitExprCall(var expr:smart_ptr<ExprCall>) : ExpressionPtr
        if expr.name=="for_each_archetype"
            loop_depth |> pop()
            inArchetype --
        return <- expr
    def override visitExprBreak(var expr:smart_ptr<ExprBreak>) : ExpressionPtr
        if inClosure!=0 || inArchetype!=0 || loop_depth|>back!=0
            return <- expr
        return <- make_static_assert_false("break is not allowed inside this macros's block {loop_depth}", expr.at)
    def override visitExprContinue(var expr:smart_ptr<ExprContinue>) : ExpressionPtr
        if inClosure!=0 || inArchetype!=0 || loop_depth|>back!=0
            return <- expr
        return <- make_static_assert_false("continue is not allowed inside this macros's block {loop_depth}", expr.at)

[macro_function]
def public convert_block_to_loop(var blk:smart_ptr<Expression>; failOnReturn,replaceReturnWithContinue,requireContinueCond:bool )
    //! Converts closure block to loop.
    //! If `failOnReturn` is true, then returns are not allowed inside the block.
    //! If `replaceReturnWithContinue` is true, then `return cond;` are replaced with `if cond; continue;`.
    //! If `requireContinueCond` is false, then `return;` is replaced with `continue;`, otherwise it is an error.
    var astVisitor = new B2LVisitor(failOnReturn,replaceReturnWithContinue,requireContinueCond)
    var inscope astVisitorAdapter <- make_visitor(*astVisitor)
    visit(blk, astVisitorAdapter)
    unsafe
        delete astVisitor

