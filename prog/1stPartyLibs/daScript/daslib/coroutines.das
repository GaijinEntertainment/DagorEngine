options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module coroutines shared public

require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost
require daslib/defer

typedef
    Coroutine = iterator<bool>
    Coroutines = array<Coroutine>

[call_macro(name="yeild_from")]
class private YieldFrom : AstCallMacro
    //! This macro converts yield_from(THAT) expression into::
    //!
    //!     for t in THAT
    //!         yield t
    //!
    //! The idea is that coroutine or generator can continuesly yield from another sub-coroutine or generator.
    def override visit ( prog:ProgramPtr; mod:Module?; var call:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        macro_verify( call.arguments |> length==1,prog,call.at,"expecting yeild_from(iterator)" )
        macro_verify( call.arguments[0]._type!=null,prog,call.at,"expecting iterator" )
        macro_verify( call.arguments[0]._type.isIterator,prog,call.at,"expecting iterator" )
        let iname = make_unique_private_name("_yield_from_iterator",call.at)
        return <- qmacro_block <|
            for $i(iname) in $e(call.arguments[0])
                yield $i(iname)

[call_macro(name="co_continue")]
class private CoContinue : AstCallMacro
    //! This macro converts co_continue to yield true.
    //! The idea is that coroutine without specified type is underneath a coroutine which yields bool.
    //! That way co_continue() does not distract from the fact that it is a generator<bool>.
    def override visit ( prog:ProgramPtr; mod:Module?; var call:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        // TODO: verify if we are in coroutine
        // TODO: verify if we are in coroutine which returns bool
        macro_verify( call.arguments |> length==0,prog,call.at,"expecting co_continue()" )
        return <- qmacro_expr <|
            yield true

[call_macro(name="co_await")]
class private CoAwait : AstCallMacro
    //! This macro converts co_await(sub_coroutine) into::
    //!
    //!     for t in subroutine
    //!         yield t
    //!
    //! The idea is that coroutine or generator can wait for a sub-coroutine to finish.
    def override visit ( prog:ProgramPtr; mod:Module?; var call:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        // TODO: verify if we are calling co_await on a coroutine.
        macro_verify( call.arguments |> length==1,prog,call.at,"expecting co_await(subroutine)" )
        let iname = make_unique_private_name("_co_await_iterator",call.at)
        return <- qmacro_block <|
            for $i(iname) in $e(call.arguments[0])
                yield $i(iname)

[function_macro(name="coroutine")]
class private CoroutineMacro : AstFunctionAnnotation
    //! This macro converts coroutine function into generator, adds return false.
    //! Daslang impelmentation of coroutine is generator based. Function is converted into a state machine,
    //! which can be resumed and suspended. The function is converted into a generator.
    //! Generator yields bool if its a void coroutine, and yields the return type otherwise.
    //! If return type is specified coroutine can serve as an advanced form of a generator.
    def override apply ( var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        var inscope retT : TypeDeclPtr
        if func.result.baseType == Type tVoid || func.result.baseType == Type autoinfer
            move_new(retT) <| qmacro_type(type<bool>)
        else
            move_new(retT) <| clone_type(func.result)
        var inscope blk <- qmacro_block <|
            return <- generator<$t(retT)>() <|
                $e(func.body)
                return false
        move(func.body) <| blk
        move_new(func.result) <| qmacro_type(type<iterator<$t(retT)>>)
        return true

def public cr_run ( var a : Coroutine )
    //! This function runs coroutine until it is finished.
    for t in a
        pass

def public cr_run_all ( var a : Coroutines )
    //! This function runs all coroutines until they are finished.
    while true
        var i = length(a)
        if i==0
            break
        while i > 0
            i --
            var t : bool
            next(a[i],t)
            if empty(a[i])
                delete a[i]
                a |> erase(i)
