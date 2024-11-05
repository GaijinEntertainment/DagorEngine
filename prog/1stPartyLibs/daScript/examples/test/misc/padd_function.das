module padd_function


require daslib/typemacro_boost
require daslib/templates_boost

struct public PaddStructureBase

[type_macro(name="padd_structure")]
class PaddDimMacro : AstTypeMacro
    def override visit ( prog:ProgramPtr; mod:Module?; td, passT:TypeDeclPtr ) : TypeDeclPtr
        if length(td.dimExpr) != 3
            macro_error(compiling_program(), td.at, "expecting 2 arguments")
            return <- [[TypeDeclPtr()]]
        var count = -1
        if td.dimExpr[2] is ExprConstInt
            count = (td.dimExpr[2] as ExprConstInt).value
        elif td.dimExpr[2] is ExprConstPtr
            count = -1
        else
            macro_error(compiling_program(), td.at, "expecting integer argument or null")
            return <- [[TypeDeclPtr()]]
        if !(td.dimExpr[1] is ExprTypeDecl)
            macro_error(compiling_program(), td.at, "expecting type argument")
            return <- [[TypeDeclPtr()]]
        let pbase = compiling_program() |> find_unique_structure("padd_function::PaddStructureBase")
        if td.dimExpr[1]._type == null
            // we are inferring generic here
            if passT ==null
                macro_error(compiling_program(), td.at, "expecting argument type to infer")
                return <- [[TypeDeclPtr()]]
            // so we return whatever was passed, assuming its a PaddStructure
            if passT.baseType == Type tStructure && passT.structType.parent==pbase
                let field = passT.structType |> find_structure_field("data")
                // we need to check if size matches
                if field == null || field._type == null
                    return <- [[TypeDeclPtr()]]
                if length(field._type.dim) != 1
                    return <- [[TypeDeclPtr()]]
                if count != -1 && count != field._type.dim[0]
                    return <- [[TypeDeclPtr()]]
                // we need to check if type matches
                var inscope argT <- clone_type((td.dimExpr[1] as ExprTypeDecl).typeexpr)
                if argT == null
                    return <- [[TypeDeclPtr()]]
                if argT.isAutoOrAlias
                    var inscope fieldT <- clone_type(field._type)
                    var inscope resT <- infer_generic_type(argT, fieldT, true, true)
                    if resT == null
                        macro_error(compiling_program(), td.at, "type mismatch, expecting {describe(argT)} got {describe(fieldT)}")
                        return <- [[TypeDeclPtr()]]
                    if !fieldT |> is_same_type(resT, RefMatters no, ConstMatters no, TemporaryMatters yes)
                        macro_error(compiling_program(), td.at, "type mismatch, expecting {describe(argT)} got {describe(fieldT)}")
                        return <- [[TypeDeclPtr()]]
                    compiling_program() |> update_alias_map(argT, resT)
                else
                    // do we need to check if passT is auto and do 2-sided infer?
                    var inscope fieldT <- clone_type(field._type)
                    fieldT.dim |> pop
                    if !fieldT |> is_same_type(argT, RefMatters no, ConstMatters no, TemporaryMatters yes)
                        macro_error(compiling_program(), td.at, "type mismatch, expecting {describe(argT)} got {describe(fieldT)}")
                        return <- [[TypeDeclPtr()]]
                return <- clone_type(passT)
            return <- [[TypeDeclPtr()]]
        else
            if td.dimExpr[1]._type.isAutoOrAlias
                return <- [[TypeDeclPtr()]]
            var inscope elemType <- clone_type(td.dimExpr[1]._type)
            elemType.flags &= ~TypeDeclFlags ref
            let hash = elemType |> get_mangled_name()
            let structName = "PaddStructure`{count}`{hash}"
            var st_ptr = compiling_program() |> find_unique_structure(structName)
            if st_ptr == null
                var inscope st <- new [[Structure() at=td.at, name := structName]]
                st.flags |= StructureFlags privateStructure | StructureFlags generated
                st.parent = pbase
                elemType.dim |> push(count)
                st |> add_structure_field ( "data", clone_type(elemType), [[ExpressionPtr]])
                st_ptr = get_ptr(st)
                compiling_module() |> add_structure(st)
            return <- new [[TypeDecl() at=td.at, baseType=Type tStructure, structType=st_ptr]]

/*
[typemacro_function]
def padd_structure ( macroArgument:TypeDeclPtr; elemType:TypeDeclPtr; count:int )
    if elemType==null
        return <- new [[TypeDecl() at=macroArgument.at, baseType=Type autoinfer]]
    let structName = "PaddStructure`{count}`{macroArgument.at.line}`{macroArgument.at.column}"
    var st_ptr = compiling_program() |> find_unique_structure(structName)
    if st_ptr == null
        var inscope st <- new [[Structure() at=macroArgument.at, name := "PaddStructure_{count}_{macroArgument.at.line}_{macroArgument.at.column}"]]
        st.flags |= StructureFlags privateStructure | StructureFlags generated
        var inscope st_type <- clone_type(elemType)
        st_type.dim |> push(count)
        st |> add_structure_field ( "data", clone_type(st_type), [[ExpressionPtr]])
        st_ptr = get_ptr(st)
        compiling_module() |> add_structure(st)
    return <- new [[TypeDecl() at=macroArgument.at, baseType=Type tStructure, structType=st_ptr]]
*/

