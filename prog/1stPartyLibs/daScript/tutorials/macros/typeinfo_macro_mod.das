options gen2
options no_aot

// Tutorial macro module: typeinfo macros (AstTypeInfoMacro).
//
// Provides three [typeinfo_macro] implementations demonstrating
// getAstChange — compile-time type introspection that replaces a
// typeinfo(...) expression with a constant AST node.
//
//   struct_info            — returns a string describing a struct's fields
//   enum_value_strings     — returns an array<string> of enum value names
//   has_non_static_method  — returns true/false whether a class has a method
//
// Each macro receives an ExprTypeInfo node from the compiler:
//   expr.typeexpr  — the TypeDeclPtr for the type argument
//   expr.subtrait  — the <name> in typeinfo trait<name>(...)
//   expr.at        — source location for generated nodes

module typeinfo_macro_mod

require daslib/ast
require strings
require daslib/ast_boost

// ---------------------------------------------------------------------------
// Section 1: struct_info
//
// typeinfo struct_info(type<T>) → string
// Returns "StructName(field1:type1, field2:type2, ...)" at compile time.
// ---------------------------------------------------------------------------

[typeinfo_macro(name="struct_info")]
class TypeInfoGetStructInfo : AstTypeInfoMacro {
    def override getAstChange(expr : smart_ptr<ExprTypeInfo>; var errors : das_string) : ExpressionPtr {
        if (expr.typeexpr == null) {
            errors := "type is missing or not inferred"
            return <- default<ExpressionPtr>
        }
        if (!expr.typeexpr.isStructure) {
            errors := "expecting structure type"
            return <- default<ExpressionPtr>
        }
        // Build "Name(f1:t1, f2:t2, ...)" string.
        var result = build_string() <| $(var w) {
            w |> write("{expr.typeexpr.structType.name}(")
            var first = true
            for (i in iter_range(expr.typeexpr.structType.fields)) {
                assume fld = expr.typeexpr.structType.fields[i]
                if (fld.flags.classMethod) {
                    continue  // skip methods — show only data fields
                }
                if (!first) {
                    w |> write(", ")
                }
                w |> write("{fld.name}:{describe(fld._type, false, false, false)}")
                first = false
            }
            w |> write(")")
        }
        return <- new ExprConstString(at = expr.at, value := result)
    }
}

// ---------------------------------------------------------------------------
// Section 2: enum_value_strings
//
// typeinfo enum_value_strings(type<E>) → array<string>
// Returns an array of enum value name strings at compile time.
// ---------------------------------------------------------------------------

[typeinfo_macro(name="enum_value_strings")]
class TypeInfoGetEnumValueStrings : AstTypeInfoMacro {
    def override getAstChange(expr : smart_ptr<ExprTypeInfo>; var errors : das_string) : ExpressionPtr {
        if (expr.typeexpr == null) {
            errors := "type is missing or not inferred"
            return <- default<ExpressionPtr>
        }
        if (!expr.typeexpr.isEnum) {
            errors := "expecting enumeration type"
            return <- default<ExpressionPtr>
        }
        // Build ExprMakeArray of ExprConstString entries.
        var inscope arr <- new ExprMakeArray(at = expr.at, makeType <- typeinfo ast_typedecl(type<string>))
        for (i in iter_range(expr.typeexpr.enumType.list)) {
            {
                assume entry = expr.typeexpr.enumType.list[i]
                var inscope nameExpr <- new ExprConstString(at = expr.at, value := entry.name)
                arr.values |> emplace <| nameExpr
            }
        }
        return <- arr
    }
}

// ---------------------------------------------------------------------------
// Section 3: has_non_static_method
//
// typeinfo has_non_static_method<method_name>(type<T>) → bool
// Returns true if the struct/class has a non-static method with the given
// name.  Uses the subtrait to get the method name, and checks the struct's
// fields for a classMethod flag.
// ---------------------------------------------------------------------------

[typeinfo_macro(name="has_non_static_method")]
class TypeInfoHasNonStaticMethod : AstTypeInfoMacro {
    def override getAstChange(expr : smart_ptr<ExprTypeInfo>; var errors : das_string) : ExpressionPtr {
        if (expr.typeexpr == null) {
            errors := "type is missing or not inferred"
            return <- default<ExpressionPtr>
        }
        if (!expr.typeexpr.isStructure) {
            errors := "expecting structure or class type"
            return <- default<ExpressionPtr>
        }
        if (empty(expr.subtrait)) {
            errors := "expecting method name as subtrait: typeinfo has_non_static_method<method_name>(type<T>)"
            return <- default<ExpressionPtr>
        }
        // Check if any field with the given name has classMethod flag.
        var found = false
        for (i in iter_range(expr.typeexpr.structType.fields)) {
            assume fld = expr.typeexpr.structType.fields[i]
            if (fld.name == expr.subtrait && fld.flags.classMethod) {
                found = true
                break
            }
        }
        return <- new ExprConstBool(at = expr.at, value = found)
    }
}
