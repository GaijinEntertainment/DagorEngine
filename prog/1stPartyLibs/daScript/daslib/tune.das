options gen2
options indenting = 4
options no_aot = true

module tune shared public

require daslib/ast
require daslib/ast_boost

require [tune_framework] public

//! The six kernel-tuning annotations - `[tune_perm]`, `[tune_companion]`, `[tune]`,
//! `[tune_scope]`, `[tune_policy]` and `[llvm_code]` - plus the default-policy infer pass, as
//! shells every program can spell. The framework that implements them lives in
//! `llvm/daslib/llvm_tune` - the `tune_framework` group's member, re-exported, and itself
//! re-exporting `llvm/daslib/llvm_code` - so `require daslib/tune` gives exactly what
//! `require llvm/daslib/llvm_tune` gave. dasLLVM joins the group exactly when the build is
//! configured with it - from its `llvm` witness module and its descriptor - the das files being on
//! disk never counts; the member is then what makes `module_exists` below answer for this compile (`module_exists`, not
//! `builtin_module_exists`: visible from this module, the same answer inside a tool's nested
//! compile). Every tune annotation registers HERE and nowhere else, because a
//! module's macro state is per-module: the `[tune_scope]` bank and the pass that reads it have
//! to run in one macro context. Without dasLLVM every apply is a no-op - no sidecar read, no
//! stamp, no `_variants()` registry, no tuner spawn - and the reference body IS the function.
//! Framework semantics, arguments and the sidecar contract: `skills/tune.md`.

[function_macro(name = "tune_perm")]
class private TunePermShell : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        static_if (typeinfo module_exists(llvm_tune)) {
            return tune_perm_apply(func, group, args, errors)
        } else {
            return true
        }
    }
}

[function_macro(name = "tune_companion")]
class private TuneCompanionShell : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        static_if (typeinfo module_exists(llvm_tune)) {
            return tune_companion_apply(func, group, args, errors)
        } else {
            return true
        }
    }
}

[function_macro(name = "tune")]
class private TuneShell : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        static_if (typeinfo module_exists(llvm_tune)) {
            return tune_apply(func, group, args, errors)
        } else {
            return true
        }
    }
}

[structure_macro(name = "tune_scope")]
class private TuneScopeShell : AstStructureAnnotation {
    def override apply(var st : StructurePtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        static_if (typeinfo module_exists(llvm_tune)) {
            return tune_scope_apply(st, group, args, errors)
        } else {
            return true
        }
    }
}

[function_macro(name = "tune_policy")]
class private TunePolicyShell : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        static_if (typeinfo module_exists(llvm_tune)) {
            return tune_policy_apply(func, group, args, errors)
        } else {
            return true
        }
    }
}

[function_macro(name = "llvm_code")]
class private LlvmCodeShell : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        static_if (typeinfo module_exists(llvm_code)) {
            return llvm_code_apply(func, group, args, errors)
        } else {
            return true
        }
    }
}

[infer_macro]
class private TuneDefaultPolicyShell : AstPassMacro {
    def override apply(prog : ProgramPtr; mod : Module?) : bool {
        static_if (typeinfo module_exists(llvm_tune)) {
            return tune_default_policy_apply(prog, mod)
        } else {
            return false
        }
    }
}

//! Log the tune status table at `LOG_INFO`, prefixed by `appName`: `<n>/<total> kernels tuned for
//! this box`, one line per function, a `--tune` hint when any kernel sits on a fallback tier.
//! Silent when the table is empty, and in a build without the framework - nothing was stamped.
[arch(at = "ARCHITECTURE.md#tune-shells-module-exists")]
def log_tune_status(appName : string) {
    var inscope rows <- tune_status_rows()
    if (empty(rows)) {
        return
    }
    var tuned = 0
    for (r in rows) {
        tuned += (r.source == "manifest" || r.source == "profile") ? 1 : 0
    }
    to_log(LOG_INFO, "{appName}: tune status ({tuned}/{length(rows)} kernels tuned for this box):\n")
    var fat = false
    for (r in rows) {
        // a fat exe lists one row per class clone, and marks the ones this box runs
        let klass = empty(r.klass) ? "" : " [{r.klass}{r.active ? " <- this box" : ""}]"
        fat ||= !empty(r.klass)
        to_log(LOG_INFO, "  {r.fname}{klass}: {empty(r.suffix) ? "reference" : r.suffix} ({r.source})\n")
    }
    if (tuned < length(rows) && !fat) {
        to_log(LOG_INFO, "{appName}: run with --tune to tune the remaining kernels for this box\n")
    }
}

//! The status table as plain values - the framework's row type never crosses the guard, so the
//! caller above is one function in every build; empty without the framework.
def private tune_status_rows() : array<tuple<fname : string; suffix : string; source : string; klass : string; active : bool>> {
    var rows : array<tuple<fname : string; suffix : string; source : string; klass : string; active : bool>>
    static_if (typeinfo module_exists(llvm_tune)) {
        var inscope st <- tune_status()
        rows |> reserve(length(st))
        for (s in st) {
            let active = !empty(s.klass) && (tune_fat_mask() & tune_fat_class_bit(s.klass)) != 0
            rows |> push((fname = s.fname, suffix = s.suffix, source = s.source, klass = s.klass, active = active))
        }
    }
    return <- rows
}
