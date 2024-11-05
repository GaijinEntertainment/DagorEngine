options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module dynamic_cast_rtti shared private

require rtti
require daslib/contracts
require daslib/templates
require daslib/templates_boost
require daslib/ast_boost
require daslib/type_traits

[expect_class(instance), template(otherclass), unused_argument(otherclass)]
def public is_instance_of(instance : auto(TCLS)?; otherclass : auto(TT) )
    if instance==null
        return false
    if *instance |> is_subclass_of(otherclass)
        return true
    elif otherclass |> is_subclass_of(*instance)
        var refinfo = unsafe(reinterpret<TypeInfo?> typeinfo(rtti_classinfo type<TT - const>))
        var clsinfo = unsafe(reinterpret<TypeInfo?> instance.__rtti)
        while clsinfo != null
            if clsinfo.hash == refinfo.hash
                return true
            clsinfo = clsinfo.firstType
        return false
    else
        return false

[expect_class(instance), template(otherclass), unused_argument(otherclass)]
def public dynamic_type_cast(instance; otherclass : auto(TT) ) : TT -const ?
    if instance==null
        return null
    if *instance |> is_subclass_of(otherclass)
        return unsafe(reinterpret<TT - const?> instance)
    elif otherclass |> is_subclass_of(*instance)
        var refinfo = unsafe(reinterpret<TypeInfo?> typeinfo(rtti_classinfo type<TT - const>))
        var clsinfo = unsafe(reinterpret<TypeInfo?> instance.__rtti)
        while clsinfo != null
            if clsinfo.hash == refinfo.hash
                return unsafe(reinterpret<TT - const?> instance)
            clsinfo = clsinfo.firstType
        return null
    else
        return null

[expect_class(instance), template(otherclass), unused_argument(otherclass)]
def public force_dynamic_type_cast(instance; otherclass : auto(TT) ) : TT -const ?
    if instance==null
        panic("null pointer cast")
    if *instance |> is_subclass_of(otherclass)
        return unsafe(reinterpret<TT - const?> instance)
    elif otherclass |> is_subclass_of(*instance)
        var refinfo = unsafe(reinterpret<TypeInfo?> typeinfo(rtti_classinfo type<TT - const>))
        var clsinfo = unsafe(reinterpret<TypeInfo?> instance.__rtti)
        while clsinfo != null
            if clsinfo.hash == refinfo.hash
                return unsafe(reinterpret<TT - const?> instance)
            clsinfo = clsinfo.firstType
        panic("not an instance of {typeinfo(typename type<TT-const>)}")
        return null
    else
        panic("not an instance of {typeinfo(typename type<TT-const>)}")
        return null

[variant_macro(name="ClassAsIs")]
class ClassAsIs : AstVariantMacro
    def override visitExprIsVariant     ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprIsVariant> ) : ExpressionPtr
        assume vtype = expr.value._type
        if !(vtype.isPointer && vtype.firstType!=null && vtype.firstType.isStructure && vtype.firstType.structType.flags.isClass)
            return <- [[ExpressionPtr]]
        var inscope atype <- new [[TypeDecl() at=expr.at, baseType=Type alias, alias:=expr.name]]
        return <- qmacro(is_instance_of($e(expr.value), type<$t(atype)>))
    def override visitExprAsVariant     ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprAsVariant> ) : ExpressionPtr
        assume vtype = expr.value._type
        if !(vtype.isPointer && vtype.firstType!=null && vtype.firstType.isStructure && vtype.firstType.structType.flags.isClass)
            return <- [[ExpressionPtr]]
        var inscope atype <- new [[TypeDecl() at=expr.at, baseType=Type alias, alias:=expr.name]]
        return <- qmacro(force_dynamic_type_cast($e(expr.value), type<$t(atype)>))
    def override visitExprSafeAsVariant ( prog:ProgramPtr; mod:Module?; expr:smart_ptr<ExprSafeAsVariant> ) : ExpressionPtr
        assume vtype = expr.value._type
        if !(vtype.isPointer && vtype.firstType!=null && vtype.firstType.isStructure && vtype.firstType.structType.flags.isClass)
            return <- [[ExpressionPtr]]
        var inscope atype <- new [[TypeDecl() at=expr.at, baseType=Type alias, alias:=expr.name]]
        return <- qmacro(dynamic_type_cast($e(expr.value), type<$t(atype)>))


