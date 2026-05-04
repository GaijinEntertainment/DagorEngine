options gen2
options no_aot

// Tutorial macro module: template type macro (structure generation).
//
// This module defines a `pair` type macro that produces a structure
// with two fields — `first : T1` and `second : T2`.  The macro is
// implemented manually (without the `[template_structure]` shorthand)
// to show the mechanics behind template type resolution:
//
//   1. `[typemacro_function]` annotation converts a regular function
//      into an AstTypeMacro.  It also auto-extracts the dimExpr
//      arguments into the function parameters.
//
//   2. On the **concrete** path (`passArgument == null`) the macro
//      generates a new structure via `qmacro_template_class`, gives
//      it a mangled name like `Pair<int,float>`, and stores each type
//      parameter as a **structure alias** — these aliases tell the
//      compiler how to resolve `T1` and `T2` inside the cloned struct.
//
//   3. On the **generic** path (`passArgument != null`) the macro
//      reads the aliases back from the concrete struct that was passed
//      as the argument, then uses `infer_template_types` to match
//      `auto(T1)` / `auto(T2)` against the actual alias values and
//      update the compiler's alias map.

module template_type_macro_mod

require daslib/typemacro_boost

// ---------------------------------------------------------------------------
// Template definition.
//
// `struct template` prevents direct instantiation — fields use alias
// names (T1, T2) that only make sense after template substitution.
// ---------------------------------------------------------------------------

struct template Pair {
    first  : T1
    second : T2
}

// ---------------------------------------------------------------------------
// Type macro function.
//
// `[typemacro_function]` generates the AstTypeMacro boilerplate:
//   • Creates a class derived from AstTypeMacro
//   • Registers it with add_new_type_macro(name="pair")
//   • Auto-extracts dimExpr arguments into function parameters
//     (macroArgument = td, passArgument = passT, then user args)
// ---------------------------------------------------------------------------

[typemacro_function]
def pair(macroArgument, passArgument : TypeDeclPtr; T1, T2 : TypeDeclPtr) : TypeDeclPtr {
    // Get the TypeDecl that points to our template struct.
    var inscope template_type <- typeinfo ast_typedecl(type<Pair>)

    // Describe the template arguments — names must match the alias
    // names used in the struct fields (T1, T2).
    var inscope template_arguments <- [
        TypeMacroTemplateArgument(name = "T1", argument_type <- clone_type(T1)),
        TypeMacroTemplateArgument(name = "T2", argument_type <- clone_type(T2))
    ]

    // No extra (non-type) arguments for this macro.
    var inscope extra_template_arguments : array<tuple<string; string>>

    // --- generic path ---
    // The compiler passes a concrete type it wants to match against
    // our template signature.  For example, when a function parameter
    // is `pair(type<auto(A)>, type<auto(B)>)` and the caller provides
    // a `Pair<int,float>`, passArgument is the Pair<int,float> type.
    if (passArgument != null) {
        // Verify the concrete type is an instance of our Pair template.
        if (!is_typemacro_template_instance(passArgument, template_type, extra_template_arguments)) {
            return <- TypeDeclPtr()
        }
        // Read the stored aliases (T1, T2) from the concrete struct
        // back into template_arguments[i].inferred_type.
        if (!infer_struct_aliases(passArgument.structType, template_arguments)) {
            return <- TypeDeclPtr()
        }
        // Match the inferred concrete types against the template
        // parameters (which may contain auto(...) patterns) and
        // update the compiler's alias map for further resolution.
        return <- infer_template_types(passArgument, template_arguments)
    }

    // --- concrete path ---
    // passArgument is null — the user wrote a concrete type like
    // `pair(type<int>, type<float>)`.  We need to generate (or
    // look up) the corresponding structure.

    // All type arguments must be fully resolved (no remaining auto).
    if (!verify_arguments(template_arguments)) {
        return <- TypeDeclPtr()
    }

    // Build a mangled name like "Pair<int,float>" for deduplication.
    var struct_name = template_structure_name(template_type.structType, template_arguments, extra_template_arguments)

    // If this exact instantiation already exists, reuse it.
    var existing_struct = compiling_program().find_unique_structure(struct_name)
    if (existing_struct != null) {
        return <- new TypeDecl(baseType = Type.tStructure, structType = existing_struct, at = template_type.at)
    }

    // Clone the template struct, rename it, clear the template flag,
    // and apply substitution rules so field types resolve correctly.
    var inscope resType <- qmacro_template_class(struct_name, type<Pair>)

    // Annotate the new struct so `is_typemacro_template_instance`
    // can identify it as an instance of Pair later (for generics).
    make_typemacro_template_instance(resType.structType, template_type.structType, extra_template_arguments)

    // Store T1 and T2 as structure aliases — this is how
    // `infer_struct_aliases` will read them back on the generic path.
    add_structure_aliases(resType.structType, template_arguments)

    return <- resType
}
