// Macro Tutorial 4: Advanced Function Macros — Module
//
// This module defines the [memoize] function macro, which demonstrates
// three AstFunctionAnnotation methods working together:
//
//   apply()     — validates the target function (rejects generics,
//                  void returns, no-argument functions, non-cloneable results)
//
//   patch()     — runs after type inference.  Clones the original function
//                  (stripping [memoize] to avoid transform loops), generates
//                  a private global cache table and a wrapper function that
//                  checks the cache before calling the clone.  Sets astChanged
//                  to restart inference so the new functions get type-checked.
//
//   transform() — runs on every call site.  After the wrapper function
//                  exists (second inference pass), redirects all calls to
//                  the original function to the memoized wrapper instead.
//
// Inspired by the [quirrel_bind] pattern in dasQuirrel and the
// [template] call-redirection pattern in daslib/templates.
//
// Function macros cannot be used in the module that defines them —
// see 04_advanced_function_macro.das for usage.

options gen2

module advanced_function_macro_mod public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/strings_boost

// ============================================================================
// The [memoize] function macro
//
// Given:
//
//   [memoize]
//   def fib(n : int) : int {
//       if (n <= 1) { return n; }
//       return fib(n - 1) + fib(n - 2)
//   }
//
// The macro generates:
//
//   1. A private global variable:
//        var private `memoize`cache`fib : table<uint64; int>
//
//   2. A private wrapper function:
//        def private `memoize`fib(n : int) : int {
//            let key = hash(n)
//            if (key_exists(`memoize`cache`fib, key)) {
//                return `memoize`cache`fib[key]   // unsafe table access
//            }
//            let result = ::fib(n)
//            `memoize`cache`fib |> insert_clone(key, result)
//            return result
//        }
//
//   3. All calls to fib(...) are redirected to `memoize`fib(...)
//      via transform(), so recursive calls are also memoized.
//
// ============================================================================

[function_macro(name="memoize")]
class MemoizeMacro : AstFunctionAnnotation {

    // ------------------------------------------------------------------
    // apply() — validation before type inference
    //
    // Runs when the annotation is first seen.  Rejects functions that
    // cannot be memoized: generics (types unknown), void return (nothing
    // to cache), zero arguments (nothing to hash).
    // ------------------------------------------------------------------
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        if (func.isGeneric) {
            errors := "cannot memoize a generic function — all argument types must be specified"
            return false
        }
        if (func.result.isVoid) {
            errors := "cannot memoize a void function — there is nothing to cache"
            return false
        }
        if (length(func.arguments) == 0) {
            errors := "cannot memoize a function with no arguments — there is nothing to hash"
            return false
        }
        return true
    }

    // ------------------------------------------------------------------
    // patch() — code generation after type inference
    //
    // At this point all types are resolved.  We generate:
    //   - A private global cache table: table<uint64; ReturnType>
    //   - A private wrapper function that checks the cache, calls the
    //     original on miss, and stores the result via insert_clone
    //
    // The "already processed" guard prevents re-generation on the
    // second inference pass (after astChanged restarts inference).
    // ------------------------------------------------------------------
    def override patch(var fn : FunctionPtr; var group : ModuleGroup;
                       args, progArgs : AnnotationArgumentList;
                       var errors : das_string; var astChanged : bool&) : bool {

        // Guard: already processed?
        if (find_arg(args, "patched") is tBool) {
            return true
        }

        // Validate: return type must be cloneable (needed for insert_clone)
        if (!fn.result.canClone) {
            errors := "cannot memoize '{string(fn.name)}' — return type '{describe(fn.result)}' cannot be cloned"
            return false
        }

        // Mark as processed and trigger inference restart
        for (ann in fn.annotations) {
            if (ann.annotation.name == "memoize") {
                astChanged = true
                ann.arguments |> add_annotation_argument("patched", true)
            }
        }

        // Names for generated entities
        let cacheName = "`memoize`cache`{fn.name}"
        let wrapperName = "`memoize`{fn.name}"
        let originalCopyName = "`memoize`original`{fn.name}"

        // Check that we haven't already generated a wrapper with this name
        var inscope existingWrapper <- fn._module |> find_unique_function(wrapperName)
        if (existingWrapper != null) {
            errors := "duplicate memoize wrapper for '{string(fn.name)}'"
            return false
        }

        // Step 1: Clone the original function (without [memoize] annotation)
        //         so the wrapper can call it without triggering transform()
        var inscope originalCopy <- clone_function(fn)
        originalCopy.name := originalCopyName
        originalCopy.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
        // Remove [memoize] from the clone to prevent infinite transform loop
        let memoizeIdx = find_index_if(each(originalCopy.annotations)) $(ann) {
            return ann.annotation.name == "memoize"
        }
        if (memoizeIdx >= 0) {
            originalCopy.annotations |> erase(memoizeIdx)
        }
        if (!(compiling_module() |> add_function(originalCopy))) {
            errors := "failed to add original copy '{originalCopyName}'"
            return false
        }

        // Step 2: Create the private global cache variable
        //         var private `memoize`cache`fib : table<uint64; RetType>
        var inscope retType <- clone_type(fn.result)
        retType.flags &= ~TypeDeclFlags.constant
        retType.flags &= ~TypeDeclFlags.ref
        // Clone again — retType will be consumed by the cache table type,
        // we need a separate copy for the wrapper function's return type.
        var inscope wrapperRetType <- clone_type(retType)
        // Build table<uint64; RetType> type manually — $t() doesn't work
        // inside typeinfo ast_typedecl for table value types
        var inscope keyType <- new TypeDecl(baseType = Type.tUInt64, at = fn.at)
        var inscope cacheType <- new TypeDecl(baseType = Type.tTable, at = fn.at)
        move(cacheType.firstType) <| keyType
        move(cacheType.secondType) <| retType
        add_global_var(compiling_module(), cacheName, clone_type(cacheType), fn.at, true)

        // Step 3: Build the wrapper function arguments (clone of original)
        var inscope wrapperArgs : array<VariablePtr>
        for (arg in fn.arguments) {
            wrapperArgs |> emplace_new <| clone_variable(arg)
        }

        // Step 4: Build the hash key computation
        //         let key = hash(a) ^ hash(b) ^ ...
        var inscope hashExprs : array<ExpressionPtr>
        for (arg in fn.arguments) {
            hashExprs |> emplace_new <| qmacro(hash($i(arg.name)))
        }

        // Combine hashes with XOR
        var inscope keyExpr <- hashExprs[0]
        for (i in range(1, length(hashExprs))) {
            {
                var inscope xorExpr <- qmacro($e(keyExpr) ^ $e(hashExprs[i]))
                unsafe { keyExpr <- xorExpr; }
            }
        }

        // Step 5: Build the call to the original (unannotated) copy
        //         `memoize`original`fib(arg1, arg2, ...)
        var inscope callArgs : array<ExpressionPtr>
        for (arg in fn.arguments) {
            callArgs |> emplace_new <| qmacro($i(arg.name))
        }

        // Step 6: Assemble the wrapper body
        var inscope bodyExprs : array<ExpressionPtr>
        bodyExprs |> emplace_new <| qmacro_expr(${ let key = $e(keyExpr); })
        bodyExprs |> emplace_new <| qmacro_expr(${ if (key_exists($i(cacheName), key)) { unsafe { return $i(cacheName)[key]; } } })
        bodyExprs |> emplace_new <| qmacro_expr(${ let result = $c(originalCopyName)($a(callArgs)); })
        bodyExprs |> emplace_new <| qmacro_expr(${ $i(cacheName) |> insert_clone(key, result); })
        bodyExprs |> emplace_new <| qmacro_expr(${ return result; })

        // Step 7: Create the wrapper function
        var inscope wrapperFn <- qmacro_function(wrapperName) $($a(wrapperArgs)) : $t(wrapperRetType) {
            $b(bodyExprs)
        }
        wrapperFn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
        wrapperFn.body |> force_at(fn.body.at)

        // Step 8: Add the wrapper to the module
        if (!(compiling_module() |> add_function(wrapperFn))) {
            errors := "failed to add memoize wrapper function '{wrapperName}'"
            return false
        }

        // Store the wrapper name in the annotation for transform() to read
        for (ann in fn.annotations) {
            if (ann.annotation.name == "memoize") {
                ann.arguments |> add_annotation_argument("wrapper", wrapperName)
            }
        }

        return true
    }

    // ------------------------------------------------------------------
    // transform() — call-site redirection
    //
    // After patch() generates the wrapper and inference restarts,
    // transform() runs on every call to the annotated function.
    // It replaces each call with a call to the memoized wrapper.
    //
    // Compare with daslib/templates.das TemplateMacro, which uses
    // the same clone-and-redirect pattern.
    // ------------------------------------------------------------------
    def override transform(var call : smart_ptr<ExprCallFunc>;
                           var errors : das_string) : ExpressionPtr {

        // Read the wrapper name stored by patch()
        for (ann in call.func.annotations) {
            if (ann.annotation.name == "memoize") {
                let wrapperArg = find_arg(ann.arguments, "wrapper")
                if (wrapperArg is tString) {
                    let wrapperName = wrapperArg as tString
                    // Clone the call and redirect to the wrapper
                    var inscope newCall <- clone_expression(call)
                    (newCall as ExprCall).name := wrapperName
                    return <- newCall
                }
            }
        }

        // No wrapper yet (first inference pass before patch runs) — keep original
        return <- default<ExpressionPtr>
    }
}
