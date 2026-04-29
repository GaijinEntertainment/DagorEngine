// Macro Tutorial 6: Structure Macros — Module
//
// This module defines the [serializable] structure macro, which
// demonstrates all three AstStructureAnnotation methods:
//
//   apply()  — runs before inference.  Validates annotation arguments,
//              adds a _version field, and generates a stub describe_X()
//              function (header only — no field prints yet).
//
//   patch()  — runs after inference.  Types are now known, so we can
//              inspect each field's baseType.  We find the stub function,
//              append per-field print statements (skipping lambdas,
//              blocks, and function pointers), and set astChanged to
//              trigger re-inference of the modified body.
//
//   finish() — runs after all inference is complete.  Prints a
//              compile-time diagnostic summary of the struct.
//
// Structure macros cannot be used in the module that defines them —
// see 06_structure_macro.das for usage.

options gen2

module structure_macro_mod public

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require daslib/templates_boost

// ============================================================================
// The [serializable] structure macro
//
// Given:
//
//   [serializable(version=2)]
//   struct Player {
//       name : string
//       health : int
//       on_hit : function<(damage:int):void>   // non-serializable
//   }
//
// The macro:
//   1. (apply)  adds `_version : int = 2` and a stub `describe_Player()`
//               that only prints the header line
//   2. (patch)  fills in field prints (skipping on_hit), sets astChanged
//   3. (finish) prints "[serializable] Player: 2 serializable, 1 skipped"
//
// ============================================================================

[structure_macro(name="serializable")]
class SerializableMacro : AstStructureAnnotation {

    // ------------------------------------------------------------------
    // apply() — before type inference
    //
    // Runs during parsing, before types are resolved.  We:
    //   - validate annotation arguments (only `version` is allowed)
    //   - add a `_version : int` field to the struct
    //   - generate a STUB describe_StructName() that prints only the
    //     header line ("StructName (version N):")
    //
    // The function must exist before inference so callers can reference
    // it.  Per-field printing is deferred to patch() where types are
    // known.
    // ------------------------------------------------------------------
    def override apply(var st : StructurePtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        // Validate arguments: only `version` (int) is allowed
        var version = 1
        for (arg in args) {
            if (arg.name == "version") {
                let val = get_annotation_argument_value(arg)
                if (val is tInt) {
                    version = val as tInt
                } else {
                    errors := "[serializable] 'version' argument must be an integer"
                    return false
                }
            } else {
                errors := "[serializable] unknown argument '{arg.name}' — only 'version' is supported"
                return false
            }
        }

        // Add a _version field with the specified version.
        // Pass temporaries — add_structure_field moves its arguments.
        st |> add_structure_field("_version", clone_type(qmacro_type(type<int>)), qmacro($v(version)))

        // Generate a STUB function: prints the header only.
        //
        // IMPORTANT: inside qmacro(), string interpolation like "{x}"
        // creates AST nodes that reference `x` at RUNTIME.  Macro-time
        // variables (st, fld) don't exist at runtime.  Use $v() to
        // splice compile-time strings, and only `obj` (the generated
        // function's parameter) for runtime references.
        let funcName = "describe_{st.name}"
        var inscope bodyExprs : array<ExpressionPtr>

        // Header: "StructName (version N):\n"
        bodyExprs |> emplace_new <| qmacro(print($v("{st.name} (version ")))
        bodyExprs |> emplace_new <| qmacro(print("{obj._version}"))
        bodyExprs |> emplace_new <| qmacro(print($v("):\n")))

        // Build and register the stub.
        // $t(st) splices the struct type, $b() injects the statements.
        var inscope fn <- qmacro_function(funcName) $(obj : $t(st)) {
            $b(bodyExprs)
        }
        fn.flags |= FunctionFlags.generated
        fn.body |> force_at(st.at)
        add_function(st._module, fn)

        return true
    }

    // ------------------------------------------------------------------
    // patch() — after type inference
    //
    // Types are now resolved.  We find the stub describe function and
    // append per-field print statements, SKIPPING fields whose
    // baseType is tLambda or tFunction (non-serializable).
    // (Blocks cannot appear as struct fields, so no tBlock check.)
    //
    // This is the key reason for using patch(): in apply() we don't
    // know field types yet, so we can't filter by type.  Here we can.
    //
    // After modifying the body we set astChanged = true, which tells
    // the compiler to re-run inference on the updated function.
    //
    // We mark the annotation with a "patched" argument to avoid
    // infinite loops — without it, astChanged would trigger patch()
    // again endlessly.
    // ------------------------------------------------------------------
    def override patch(var st : StructurePtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        // Guard: if we already patched, do nothing.
        // This prevents infinite re-inference loops.
        if (find_arg(args, "patched") is tBool) {
            return true
        }

        // Find the stub function generated in apply()
        let funcName = "describe_{st.name}"
        var inscope fn <- st._module |> find_unique_function(funcName)
        if (fn == null) {
            errors := "[serializable] internal error: '{funcName}' not found"
            return false
        }

        // Get body as ExprBlock so we can append statements
        unsafe {
            var blk = reinterpret<ExprBlock?> fn.body

            // Append one print group per serializable field
            for (fld in st.fields) {
                if (fld.name == "_version") {
                    continue
                }
                // Skip non-serializable types — only known after inference
                if (fld._type.baseType == Type.tLambda || fld._type.baseType == Type.tFunction) {
                    continue
                }
                blk.list |> emplace_new <| qmacro(print($v("  {fld.name} = ")))
                blk.list |> emplace_new <| qmacro(print("{obj.$f(fld.name)}"))
                blk.list |> emplace_new <| qmacro(print($v("\n")))
            }
        }

        // Mark as patched and trigger re-inference
        for (ann in st.annotations) {
            if (ann.annotation.name == "serializable") {
                ann.arguments |> add_annotation_argument("patched", true)
            }
        }
        astChanged = true
        return true
    }

    // ------------------------------------------------------------------
    // finish() — after inference is complete
    //
    // Read-only inspection phase.  All types are final.  We print a
    // compile-time diagnostic: how many fields are serializable, how
    // many were skipped, and the version number.
    // ------------------------------------------------------------------
    def override finish(var st : StructurePtr; var group : ModuleGroup;
                        args : AnnotationArgumentList; var errors : das_string) : bool {
        var serializable = 0
        var skipped = 0
        for (fld in st.fields) {
            if (fld.name == "_version") {
                continue
            }
            if (fld._type.baseType == Type.tLambda || fld._type.baseType == Type.tFunction) {
                skipped++
            } else {
                serializable++
            }
        }
        print("[serializable] {st.name}: {serializable} serializable field(s)")
        if (skipped > 0) {
            print(", {skipped} skipped")
        }
        let ver = find_arg(args, "version")
        if (ver is tInt) {
            print(", version {ver as tInt}")
        } else {
            print(", version 1")
        }
        print("\n")
        return true
    }
}
