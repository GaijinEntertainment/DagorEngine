options gen2
options indenting = 4
options no_unused_function_arguments = false

module flat_pack_macro shared private

//! A stand-in "backend" annotation that exercises the PUBLIC `flatten_pack` entry the way the
//! example shader backend does: clone a twin, `flatten_function` it (cycle 1), then on the
//! re-inferred body run `flatten_pack` (scalar-packing map-reduce re-roll) PRE-fold, then
//! `flatten_fold`. Lives in its own module — the consuming test compiles a fixture that
//! `require`s it. The `*_packflat` twin is the real flattened+packed body, so a test can
//! introspect it (does a min/max reduction re-roll into `_pk*` vector vars, or bail clean?).

require daslib/flatten
require daslib/ast
require daslib/ast_boost
require daslib/rtti

let private PACK_PRIMS : table<string> <- { "sin", "cos", "sqrt", "min", "max" }

def private pack_phase(func : FunctionPtr) : int {
    for (ann in func.annotations) {
        if (ann.annotation.name == "flat_pack") {
            let pv = find_arg(ann.arguments, "phase")
            if (pv is tInt) return pv as tInt
        }
    }
    return -1
}

def private set_pack_phase(var func : FunctionPtr; ph : int) {
    for (ann in func.annotations) {
        if (ann.annotation.name != "flat_pack") continue
        var found = false
        for (a in ann.arguments) {
            if ("{a.name}" == "phase") {
                a.iValue = ph
                found = true
            }
        }
        if (!found) ann.arguments |> add_annotation_argument("phase", ph)
    }
}

[function_macro(name="flat_pack")]
class FlatPackMacro : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       args : AnnotationArgumentList; var errors : das_string) : bool {
        let tn = "{func.name}_packflat"
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
                       args, progArgs : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        // Skip under lint (the twin is irrelevant to linting source); mirrors [flatten] / the backend.
        if (is_in_lint_check()) return true
        let ph = pack_phase(func)
        if (ph >= 3) return true
        var twin = find_unique_function(compiling_module(), "{func.name}_packflat", true)
        if (twin == null) {
            errors := "flat_pack: twin not declared"
            return false
        }
        if (ph == 2) {
            // Cycle 2 finish: fold the packed body the way the backend does after packing.
            flatten_fold(twin)
            set_pack_phase(func, 3)
            astChanged = true
            return true
        }
        if (ph == 1) {
            // Re-inferred after flattening: run the PUBLIC pack entry PRE-fold (backend order).
            flatten_pack(twin, false)
            set_pack_phase(func, 2)
            astChanged = true
            return true
        }
        let ctx = flatten_function(twin, PACK_PRIMS)
        if (!ctx.ok) {
            errors := ctx.err
            return false
        }
        set_pack_phase(func, 1)
        astChanged = true
        return true
    }
}
