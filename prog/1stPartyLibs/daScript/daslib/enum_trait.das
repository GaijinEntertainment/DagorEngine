options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module enum_trait shared private

//! Enumeration reflection traits.
//!
//! Generates helper functions for enumerations: ``each`` (iterate over all
//! values), ``each_name`` (iterate over all name strings), and
//! ``find_value`` (look up a value by name).

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/templates
require daslib/contracts

[expect_any_enum(tt), unused_argument(tt), nodiscard]
def public each(tt : auto(TT)) : iterator<TT -const -&> {
    //! Returns an iterator over all values of the given enumeration type.
    concept_assert(typeinfo is_enum(tt), "expecting 'each(any_enum_value)'")
    var iter : iterator<TT -const -&>
    _builtin_make_enum_iterator(iter)
    unsafe {
        return <- iter
    }
}

[expect_any_enum(arg)]
def public string(arg) {
    //! converts enum value to string
    //!  usage: let s = string(EnumValue)
    return "{arg}"
}

[template(ent), unused_argument(ent)]
def public to_enum(ent : auto(EnumT), name : string) : EnumT {
    //! converts string to enum value, panics if not found
    //!  usage: let e = to_enum(type<EnumType>,"EnumValueName")
    for (e in type<EnumT>) {
        if ("{e}" == name) {
            return e
        }
    }
    panic("to_enum: enum value '{name}' not found in enum '{typeinfo typename(type<EnumT>)}'")
    return default<EnumT>
}

[template(ent), unused_argument(ent)]
def public to_enum(ent : auto(EnumT), name : string; defaultValue : EnumT) : EnumT {
    //! converts string to enum value, returns defaultValue if not found
    //!  usage: let e = to_enum(type<EnumType>,"EnumValueName", EnumType.DefaultValue)
    for (e in type<EnumT>) {
        if ("{e}" == name) {
            return e
        }
    }
    return defaultValue
}

[template(ent), unused_argument(ent)]
def public enum_to_table(ent : auto(EnumT)) : table<string, EnumT -const -&> {
    //! converts enum type to a table of name => value pairs
    //!  usage: let t = enum_to_table(type<EnumType>)
    var tab : table<string, EnumT -const -&>
    for (e in type<EnumT>) {
        unsafe(tab["{e}"]) = e
    }
    return <- tab
}

[enumeration_macro(name="string_to_enum")]
class EnumFromStringConstruction : AstEnumerationAnnotation {
    //! Enumeration annotation which implements string constructor for enumeration.
    def override apply(var enu : EnumerationPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        //! implements enum string constructor
        var inscope enumT <- new TypeDecl(baseType = Type.tEnumeration, enumType = enu.get_ptr())
        // lets make a table variable
        let varName = "_`enum`table`{enu.name}"
        add_global_private_let(compiling_module(), varName, enu.at, qmacro(enum_to_table(type<$t(enumT)>)))
        // panic one
        var inscope enumFn <- qmacro_function("{enu.name}") $(src : string) : $t(enumT) {
            if (!key_exists($i(varName), src)) {
                panic("string_to_enum: enum value '{src}' not found in enum '{typeinfo typename(type<$t(enumT)>)}'")
            }
            return $i(varName)?[src] ?? default<$t(enumT)>
        }
        enumFn.flags &= ~FunctionFlags.privateFunction
        force_at(enumFn, enu.at)
        force_generated(enumFn, true)
        compiling_module() |> add_function(enumFn)
        // with default
        var inscope enumFnDefault <- qmacro_function("{enu.name}") $(src : string; defaultValue : $t(enumT)) : $t(enumT) {
            return $i(varName)?[src] ?? defaultValue
        }
        enumFnDefault.flags &= ~FunctionFlags.privateFunction
        force_at(enumFnDefault, enu.at)
        force_generated(enumFnDefault, true)
        compiling_module() |> add_function(enumFnDefault)
        return true

    }
}

[typeinfo_macro(name="enum_length")]
class TypeInfoGetEnumLength : AstTypeInfoMacro {
    //! Implements typeinfo enum_length(EnumOrEnumType) which returns total number of elements in enumeration.
    def override getAstChange(expr : smart_ptr<ExprTypeInfo>; var errors : das_string) : ExpressionPtr {
        //! returns number of enum values
        if (expr.typeexpr != null) {
            if (!expr.typeexpr.isEnum) {
                errors := "expecting enumeration"
                return <- default<ExpressionPtr>
            }
            let sz = length(expr.typeexpr.enumType.list)
            return <- new ExprConstInt(at = expr.at, value = sz)
        }
        errors := "type is missing or not inferred"
        return <- default<ExpressionPtr>
    }
}

[typeinfo_macro(name="enum_names")]
class TypeInfoGetEnumNames : AstTypeInfoMacro {
    //! Implements typeinfo enum_names(EnumOrEnumType) which returns array of strings with enumValue names.
    def override getAstChange(expr : smart_ptr<ExprTypeInfo>; var errors : das_string) : ExpressionPtr {
        //! returns array of enum value names
        if (expr.typeexpr == null) {
            errors := "type is missing or not inferred"
            return <- default<ExpressionPtr>
        }
        if (!expr.typeexpr.isEnum) {
            errors := "expecting enumeration"
            return <- default<ExpressionPtr>
        }
        var inscope arr <- new ExprMakeArray(at = expr.at, makeType <- typeinfo ast_typedecl(type<string>))
        for (i in iter_range(expr.typeexpr.enumType.list)) {
            {
                assume name = expr.typeexpr.enumType.list[i].name
                var inscope nameExpr <- new ExprConstString(at = expr.at, value := name)
                arr.values |> emplace <| nameExpr
            }
        }
        return <- arr
    }
}
