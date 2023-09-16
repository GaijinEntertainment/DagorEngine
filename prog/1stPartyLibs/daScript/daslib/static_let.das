options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module static_let shared private

require ast
require rtti
require daslib/ast_boost
require daslib/templates_boost

[tag_function(static_let_tag)]
def public static_let ( blk : block )
    //! Given a scope with the variable declarations, this function will make those variables global.
    //! Variable will be renamed under the hood, and all local access to it will be renamed as well.
    pass

[tag_function(static_let_tag)]
def public static_let_finalize ( blk : block )
    //! This is very similar to regular static_let, but additionally the variable will be deleted on the context shutdown.
    pass

def private make_shutdown_function ( varname:string; at:LineInfo )
    var inscope fn <- new [[Function() at=at, atDecl=at, name:="shutdown`{varname}"]]
    fn.flags |= FunctionFlags generated
    fn.flags |= FunctionFlags shutdown
    move_new(fn.result) <| new [[TypeDecl() baseType=Type tVoid, at=at]]
    var inscope blk <- new [[ExprBlock() at=at]]
    var inscope delv <- new [[ExprDelete() at=at,
        subexpr <- new [[ExprVar() at=at, name:=varname]]
    ]]
    delv.genFlags |= ExprGenFlags alwaysSafe
    emplace(blk.list,delv)
    move(fn.body) <| blk
    return compiling_module() |> add_function(fn)

[tag_function_macro(tag="static_let_tag")]
class StaticLetMacro : AstFunctionAnnotation
    //! This macro implements the `static_let` and `static_let_finalize` functions.
    def override transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        assert(call.arguments[0] is ExprMakeBlock)
        let mblk = call.arguments[0] as ExprMakeBlock
        var blk = mblk._block as ExprBlock
        if blk.finalList |> length != 0
            errors := "not expecting finally section in the static_let"
            return <- [[ExpressionPtr]]
        let prefix = "`static`at_line_{int(call.at.line)}`"
        var inscope renames : array<tuple<old_name:string;new_name:string>>
        for ex in blk.list
            if !(ex is ExprLet)
                errors := "expecting let ... expressions in the static_let"
                return <- [[ExpressionPtr]]
            var lexpr = ex as ExprLet
            for vvar in lexpr.variables
                if true
                    var inscope vclone <- clone_variable(vvar)
                    vclone.flags |= VariableFlags private_variable
                    vclone.flags |= VariableFlags generated
                    let old_name = "{vclone.name}"
                    let new_name = "{prefix}{vclone.name}"
                    vclone.name := new_name
                    renames |> push([[auto old_name,new_name]])
                    if !(compiling_module() |> add_variable(vclone))
                        errors := "can't add global variable {new_name}"
                        return <- [[ExpressionPtr]]
                    if "{call.name}"=="static_let_finalize"
                        if !make_shutdown_function(new_name,vvar.at)
                            errors := "can't add shutdown function {new_name}"
                            return <- [[ExpressionPtr]]
        var success = true
        compiling_program() |> get_ast_context(call) <| $(valid, astc)
            if !valid
                errors := "can't get valid program context"
                success = false
            else
                apply_template(astc.scopes[astc.scopes |> length-1]) <| $ ( rules )
                    for r in renames
                        rules |> renameVariable(r.old_name) <| r.new_name
        if !success
            return <- [[ExpressionPtr]]
        return <- quote <|
            pass

