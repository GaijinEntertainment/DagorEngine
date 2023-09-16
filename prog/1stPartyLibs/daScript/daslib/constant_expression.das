options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module constant_expression shared private

require ast
require daslib/ast_boost
require daslib/strings_boost
require daslib/templates_boost
require daslib/macro_boost
require daslib/algorithm

[function_macro(name=constexpr)]
class ConstExprAnnotation : AstFunctionAnnotation
    //! This macro implements a constexpr function argument checker. Given list of arguments to verify, it will fail for every one where non-constant expression is passed. For example::
    //!
    //!     [constexpr (a)]
    //!     def foo ( t:string; a : int )
    //!         print("{t} = {a}\n")
    //!     var BOO = 13
    //!     [export]
    //!     def main
    //!         foo("blah", 1)
    //!         foo("ouch", BOO)    // comilation error: `a is not a constexpr, BOO`
    def override verifyCall ( var call : smart_ptr<ExprCallFunc>; args,progArgs:AnnotationArgumentList; var errors : das_string ) : bool
        for aa in args
            if aa.basicType == Type tBool
                for ca,ce in call.func.arguments,call.arguments
                    if ca.name==aa.name
                        if !isConstantExpression(ce)
                            errors := "{aa.name} is not a constexpr, {describe(ce)}"
                            return false
        return true

[macro_function]
def public isConstantExpression ( expr:ExpressionPtr ) : bool
    //! This macro function retrusn true if the expression is a constant expression
    if expr is ExprRef2Value
        return isConstantExpression( (expr as ExprRef2Value).subexpr )
    else
        return expr.__rtti |> starts_with("ExprConst")

[function_macro(name="constant_expression")]
class ConstantExpressionMacro : AstFunctionAnnotation
    //! This function annotation implments constant expression folding for the given arguments.
    //! When argument is specified in the annotation, and is passed as a contstant expression,
    //! custom version of the function is generated, and an argument is substituted with a constant value.
    //! This allows using of static_if expression on the said arguments, as well as other optimizations.
    //! For example::
    //!
    //!     [constant_expression(constString)]
    //!     def take_const_arg(constString:string)
    //!         print("constant string is = {constString}\n")   // note - constString here is not an argument
    //!
    def override transform ( var expr : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        macro_verify(expr.func!=null,compiling_program(),expr.at,"expecting inferred function")
        var argi : array<int>
        var anni = -1
        for ann,idd in expr.func.annotations,count()
            if ann.annotation.name=="constant_expression"
                anni = idd
                for arg in ann.arguments
                    macro_verify(arg.basicType==Type tBool, compiling_program(), expr.at,"unsupported argument type {arg.basicType}")
                    for farg,fidx in expr.func.arguments,count()
                        if farg.name==arg.name
                            if !isConstantExpression(expr.arguments[fidx])
                                if arg.bValue
                                    compiling_program() |> macro_error(expr.at,"function {expr.func.name} argument {arg.name} is not a compilation time constant")
                                return <- [[ExpressionPtr]]
                            argi |> push(fidx)
                break
        argi |> sort_unique
        argi |> reverse
        if empty(argi)
            return <- [[ExpressionPtr]]
        var inscope func_copy <- clone_function(FunctionPtr(expr.func))
        var func_name = "{func_copy.name}`constant_expression"
        for i in argi
            let fhash = hash(describe(expr.arguments[i]))
            func_name += "`{fhash}"
        func_copy.name := func_name
        if expr.func.fromGeneric != null
            func_copy.body := null
            func_copy.result := null
            unsafe
                func_copy.body <- clone_expression(expr.func.fromGeneric.body)
                func_copy.result <- clone_type(expr.func.fromGeneric.result)

        apply_template(func_copy.body) <| $ ( rules )
            for i in argi
                rules |> replaceVariable(string(expr.func.arguments[i].name)) <| clone_expression(expr.arguments[i])
        for i in argi
            func_copy.arguments |> erase(i)
        // func_copy.flags |= FunctionFlags exports
        func_copy.flags |= FunctionFlags generated
        func_copy.annotations |> erase(anni)
        compiling_module() |> add_function(func_copy)
        var inscope new_call <- clone_expression(expr)
        var ncall = new_call as ExprCall
        ncall.name := func_name
        for i in argi
            ncall.arguments |> erase(i)
        return <- new_call
