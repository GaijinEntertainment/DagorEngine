options gen2
options indenting = 4
options no_unused_function_arguments = false

module flat_api_macro shared private

//! A stand-in "backend" annotation that exercises the PUBLIC `flatten_function`
//! entry the Option-2 way: it calls `flatten_function` directly with its OWN
//! primitive set (here just {max, min}) instead of going through the bundled
//! `[flatten]` annotation. Lives in its own module — the real backend topology
//! (the macro consumer `require`s it, just as a backend would `require flatten`).

require daslib/flatten
require daslib/ast
require daslib/ast_boost
require daslib/rtti

// "myrec" is a USER function with a body (and recursive) that the caller declares
// a leaf primitive: flatten must KEEP the call, not inline it. If whitelist
// precedence regressed, flatten would try to inline a recursive function and fail.
let private MY_PRIMS : table<string> <- { "max", "min", "myrec", "g_writer" }

def private set_api_phase(var args : AnnotationArgumentList; ph : int) {
    for (a in args) {
        if (a.name == "phase") {
            a.iValue = ph
            return
        }
    }
    args |> add_annotation_argument("phase", ph)
}

[function_macro(name="flat_api")]
class FlatApiMacro : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        let tn = "{func.name}_apiflat"
        if (find_unique_function(compiling_module(), tn, true) == null) {
            var fn = clone_function(func)
            fn.name := tn
            fn.flags |= FunctionFlags.generated | FunctionFlags.exports
            while (fn.annotations |> length != 0) {
                fn.annotations |> erase(0)
            }
            compiling_module() |> add_function(fn)
        }
        return true
    }
    def override patch(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; progArgs : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        // Skip under lint (const-fold off → the twin is irrelevant to linting source,
        // and re-running flatten under lint passes doubles the mask decls). Mirrors the
        // [flatten] annotation and the backend; the inert clone from apply is what lint sees.
        if (is_in_lint_check()) return true
        let ph = args |> find_arg("phase") ?as tInt ?? -1
        if (ph >= 2) return true
        var twin = find_unique_function(compiling_module(), "{func.name}_apiflat", true)
        if (twin == null) {
            errors := "flat_api: twin not declared"
            return false
        }
        if (ph == 1) {
            // Cycle 2 (re-inferred): run the PUBLIC fold companion the same way a
            // backend would — collapse const selects + drop pure self-assigns.
            let folded = flatten_fold(twin)
            set_api_phase(args, 2)
            if (folded) {
                astChanged = true
            }
            return true
        }
        var ctx = flatten_function(twin, MY_PRIMS)
        if (!ctx.ok) {
            errors := ctx.err
            return false
        }
        set_api_phase(args, 1)
        astChanged = true
        return true
    }
}
