options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers

module typemacro_boost shared private

require daslib/ast_boost
require daslib/macro_boost
require daslib/templates_boost

require daslib/ast_boost public

def public int64_to_enum ( _enu : auto(ET); value: int64 ) : ET
    static_if typeinfo(sizeof type<ET>)==1
        return unsafe(reinterpret<ET>(int8(value)))
    static_elif typeinfo(sizeof type<ET>)==2
        return unsafe(reinterpret<ET>(int16(value)))
    else
        return unsafe(reinterpret<ET>(int(value)))

[macro_function]
def append_expressions ( var blk : ExprBlock?; var eblk : ExpressionPtr )
    for e in (eblk as ExprBlock).list
        blk.list |> emplace(e)

[macro_function]
def is_typedecl_ptr ( t : TypeDeclPtr )
    if t.baseType!=Type tPointer || !t.flags.smartPtr
        return false
    assume pt = t.firstType
    if pt==null
        return false
    if pt.baseType!=Type tHandle || pt.annotation.name!="TypeDecl" || pt.annotation._module.name!="ast"
        return false
    return true

[function_macro(name="typemacro_function")]
class TypeMacroMacro : AstFunctionAnnotation
    //! This macro converts function into a type macro.
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool
        let className = "{func.name}`TypeMacro"
        var inscope parentType <- typeinfo(ast_typedecl type<ast::AstTypeMacro>)
        if parentType == null
            errors := "ast::AstTypeMacro not found"
            return false
        var inscope cls <- make_class(className, parentType.structType, compiling_module())
        let cvtFnName = "{func.name}`cvt`arguments"
        var inscope cvtFn <- qmacro_function(cvtFnName) <| $ ( prog:ProgramPtr; mod:Module?; td:TypeDeclPtr ) : TypeDeclPtr
            return <- [[TypeDeclPtr]]
        compiling_module() |> add_function(cvtFn)
        let visitName = className + "`visit"
        var inscope visitFn <- qmacro_method(visitName, cls) <| $ ( var slf:$t(cls); prog:ProgramPtr; mod:Module?; td:TypeDeclPtr ) : TypeDeclPtr
            return <- $c(cvtFnName)(prog, mod, td)
        compiling_module() |> add_function(visitFn)
        var field = find_structure_field(get_ptr(cls), "visit")
        if field == null
            errors := "visit field does not exist"
            return false
        field.init |> move_new <| qmacro(cast<auto> @@$i(visitName))
        cls |> append_annotation("ast_boost", "type_macro", [{auto
            [[auto "name", [[RttiValue tString=string(func.name)]] ]]
        }])
        compiling_module() |> add_structure(cls)
        return true

    def override patch ( var func:FunctionPtr; var group:ModuleGroup; args,progArgs:AnnotationArgumentList; var errors : das_string; var astChanged:bool& ) : bool

        for ann in func.annotations
            if ann.annotation.name=="typemacro_function"
                if !(find_arg(ann.arguments,"patched") is nothing)
                    return true

        // we expect the following arguments
        // prog:ProgramPtr
        // mod:Module?
        // td:TypeDeclPtr
        // all other arguments (exprt arg 0, which is typemacro name)
        if length(func.arguments) < 1
            errors := "expecting at least 1 argument (macroArgument: TypeDeclPtr)"
            return false

        let cvtFnName = "{func.name}`cvt`arguments"
        var inscope cvtFn <- compiling_module() |> find_unique_function(cvtFnName)
        if cvtFn == null
            errors := "cvt function not found"
            return false

        var body = cvtFn.body as ExprBlock
        body.list |> clear

        // we need to check, if there are enough arugments
        let expectedArgs = length(func.arguments)
        var inscope checkE <- qmacro_block() <|
            if length(td.dimExpr) != $v(expectedArgs)
                macro_error(compiling_program(), td.at, "expecting {$v(expectedArgs)} arguments, got {length(td.dimExpr)}")
                return <- [[TypeDeclPtr()]]
        append_expressions(body, checkE)

        var inscope retCall <- qmacro($c(func.name)(td))

        // allowed are basic types (int, float etc) and TypeDeclPtr
        for ai in range(1,length(func.arguments))
            let argIndex = ai - 1
            let macroArgIndex = ai
            assume argType = func.arguments[ai]._type
            if length(argType.dim) > 0
                errors := "expecting non-array arguments"
                return false
            if argType.baseType==Type tInt
                var inscope expr <- qmacro_block() <|
                    if !(td.dimExpr[$v(macroArgIndex)] is ExprConstInt)
                        macro_error(compiling_program(), td.at, "expecting {$v(string(func.arguments[ai].name))} to be int const")
                        return <- [[TypeDeclPtr]]
                append_expressions(body, expr)
                (retCall as ExprCall).arguments |> emplace_new <| qmacro((td.dimExpr[$v(macroArgIndex)] as ExprConstInt).value)
            elif argType.baseType==Type tBool
                var inscope expr <- qmacro_block() <|
                    if !(td.dimExpr[$v(macroArgIndex)] is ExprConstBool)
                        macro_error(compiling_program(), td.at, "expecting {$v(string(func.arguments[ai].name))} to be bool const")
                        return <- [[TypeDeclPtr]]
                append_expressions(body, expr)
                (retCall as ExprCall).arguments |> emplace_new <| qmacro((td.dimExpr[$v(macroArgIndex)] as ExprConstBool).value)
            elif argType.baseType==Type tBitfield
                var inscope expr <- qmacro_block() <|
                    if !(td.dimExpr[$v(macroArgIndex)] is ExprConstBitfield)
                        macro_error(compiling_program(), td.at, "expecting {$v(string(func.arguments[ai].name))} to be bool const")
                        return <- [[TypeDeclPtr]]
                append_expressions(body, expr)
                (retCall as ExprCall).arguments |> emplace_new <| qmacro(bitfield((td.dimExpr[$v(macroArgIndex)] as ExprConstBitfield).value))
            elif argType.baseType==Type tEnumeration || argType.baseType==Type tEnumeration16 || argType.baseType==Type tEnumeration64 || argType.baseType==Type tEnumeration8
                let argTypeName = describe(argType)
                var inscope expr <- qmacro_block() <|
                    if !(td.dimExpr[$v(macroArgIndex)] is ExprConstEnumeration)
                        macro_error(compiling_program(), td.at, "expecting {$v(string(func.arguments[ai].name))} to be {$v(argTypeName)}")
                        return <- [[TypeDeclPtr]]
                append_expressions(body, expr)
                (retCall as ExprCall).arguments |> emplace_new <| qmacro(
                    int64_to_enum(type<$t(argType)>,
                        find_enum_value(td.dimExpr[$v(macroArgIndex)]._type.enumType,
                            string((td.dimExpr[$v(macroArgIndex)] as ExprConstEnumeration).value))))
            elif is_typedecl_ptr(argType)
                (retCall as ExprCall).arguments |> emplace_new <| qmacro(td.dimExpr[$v(macroArgIndex)]._type)
            else
                errors := "unsupported argument type {describe(argType)}"
                return false

        var inscope retExpr <- qmacro_block() <|
            return <- $e(retCall)
        append_expressions(body, retExpr)

        for ann in func.annotations
            if ann.annotation.name=="typemacro_function"
                astChanged = true
                ann.arguments |> add_annotation_argument("patched",true)

        return true
