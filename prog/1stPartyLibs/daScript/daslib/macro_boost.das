options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true


module macro_boost shared private

require daslib/ast_boost
require daslib/templates_boost

[macro_function,tag_function(macro_verify_tag)]
def public macro_verify ( expr:bool; prog:ProgramPtr; at:LineInfo; message:string )
    //! Same as verify, only the check will produce macro error, followed by return [[ExpressionPtr]]
    pass

[tag_function_macro(tag="macro_verify_tag")]
class MacroVerifyMacro : AstFunctionAnnotation
    //! This macro convert macro_verify(expr,message,prog,at) to the following code::
    //!    if !expr
    //!        macro_error(prog,at,message)
    //!        return [[ExpressionPtr]]
    def override transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        // if ..
        var inscope res <- qmacro_block() <|
            if !$e(call.arguments[0])
                macro_error($e(call.arguments[1]),$e(call.arguments[2]),$e(call.arguments[3]))
                return <- [[ExpressionPtr]]
        res |> force_at(call.at)
        return <- res

[macro]
class CaptureBlock : AstVisitor
    [[do_not_delete]] scope : table<Variable?>
    [[do_not_delete]] capture : table<Variable?; ExprVar?>
    def override preVisitExprVar(expr:smart_ptr<ExprVar>) : void
        if !(expr.varFlags.local || expr.varFlags.argument || expr.varFlags._block)
            return
        let vptr = get_ptr(expr.variable)
        if !(scope |> key_exists(vptr))
            capture[vptr] = get_ptr(expr)
    def override preVisitExprLetVariable(expr:smart_ptr<ExprLet>;arg:VariablePtr;lastArg:bool) : void
        scope |> insert(get_ptr(arg))
    def override preVisitExprForVariable(expr:smart_ptr<ExprFor>;svar:VariablePtr;last:bool) : void
        scope |> insert(get_ptr(svar))
    def override preVisitExprBlockArgument(blk:smart_ptr<ExprBlock>;arg:VariablePtr;lastArg:bool): void
        scope |> insert(get_ptr(arg))

struct public CapturedVariable
    //! Stored captured variable together with the `ExprVar` which uses it
    variable : Variable?
    expression : ExprVar?

[macro_function]
def public capture_block(expr:ExpressionPtr) : array<CapturedVariable>
    //! Collect all captured variables in the expression.
    var astVisitor = new CaptureBlock()
    var inscope adapter <- make_visitor(*astVisitor)
    visit(expr, adapter)
    var res <- [{for k,v in keys(astVisitor.capture),values(astVisitor.capture); [[CapturedVariable variable=k,expression=v]] }]
    unsafe
        delete astVisitor
    return <- res

[macro]
class ColletFinally : AstVisitor
    [[do_not_delete]] blocks : table<ExprBlock?>
    def override preVisitExprBlock(blk:smart_ptr<ExprBlock>) : void
        if length(blk.finalList) > 0
            blocks |> insert(get_ptr(blk))
    def override canVisitMakeBlockBody(expr:smart_ptr<ExprMakeBlock>) : bool
        return false

[macro_function]
def public collect_finally(expr:ExpressionPtr)
    //! Collect all finally blocks in the expression.
    //! Returns array of ExprBlock? with all the blocks which have `finally` section
    //! Does not go into 'make_block' expression, such as `lambda`, or 'block' expressions
    var astVisitor = new ColletFinally()
    var inscope adapter <- make_visitor(*astVisitor)
    visit(expr, adapter)
    var res <- [{for k in keys(astVisitor.blocks); k }]
    unsafe
        delete astVisitor
    return <- res

[macro]
class ColletLabels : AstVisitor
    labels : table<int>
    def override preVisitExprLabel(expr:smart_ptr<ExprLabel>) : void
        labels |> insert(expr.labelName)
    def override canVisitMakeBlockBody(expr:smart_ptr<ExprMakeBlock>) : bool
        return false

[macro_function]
def public collect_labels(expr:ExpressionPtr)
    //! Collect all labels in the expression. Returns array of integer with label indices
    //! Does not go into 'make_block' expression, such as `lambda`, or 'block' expressions
    var astVisitor = new ColletLabels()
    var inscope adapter <- make_visitor(*astVisitor)
    visit(expr, adapter)
    var res <- [{for k in keys(astVisitor.labels); k }]
    unsafe
        delete astVisitor
    return <- res

[call_macro(name="return_skip_lockcheck")]
class private ReturnSkipLockcheck : AstCallMacro
    //! this is similar to regular return <-, but it does not check for locks
    def override visit ( prog:ProgramPtr; mod:Module?; var call:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        macro_verify( call.arguments |> length==1,prog,call.at,"expecting return_skip_lockcheck(expr)" )
        return <- new [[ExprReturn()
            at=call.at,
            subexpr <- clone_expression(call.arguments[0]),
            returnFlags = ExprReturnFlags skipLockCheck | ExprReturnFlags moveSemantics
        ]]
