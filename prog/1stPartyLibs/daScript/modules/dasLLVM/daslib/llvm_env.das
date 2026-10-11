options gen2

//! dasLLVM's environment knobs, declared as `[EnvConfig]` structs (daslib/clargs) — one field
//! per variable, the doc line on the declaration. `ENVIRONMENT.md` is GENERATED from these
//! (`harness/gen_env_doc.das`) and `tests/llvm_env_registry.das` fails when a variable is read
//! anywhere in the module without a declaration here.
//!
//! Both structs load ONCE at context init (`g_env_jit`, `g_env_tune`); `set_env_variable`
//! after startup is invisible to this process and arms CHILDREN only (the tuner's re-exec
//! model). In-process overrides go through the tune setters instead
//! (`set_tune_manifest_runtime_path`, `tune_set_verbosity`, `tune_set_noise_cv`,
//! `tune_set_noise_override`, `tune_set_history_dir`), which also arm the child environment.
//! Ambient variables the module lists but does not own stay live-read via `env_value_of`.

module llvm_env shared public

require daslib/clargs
require daslib/env_registry public
require daslib/rtti

// ===== JIT =====

[EnvConfig(env_prefix = "DAS")]
struct public JitEnv {
    @clarg_doc = "Split-JIT LTO probe: partitions emit bitcode and the link runs lld LTO (pass /opt:lldlto=2 plus the CRT /LIBPATHs via --jit-linker-string). Dev instrument for measuring cross-module-inlining recovery; needs --jit-split-modules + --jit-obj-cache=0, announces itself, and folds into the cache keys so a probe artifact never serves a normal run."
    jit_probe_lto : bool = false

    @clarg_doc = "Split-JIT key forensics: log every (partition, mangled name, aot hash) the obj-cache chain folds. Diff two runs to locate WHERE a key diverged - distinguishes a changed hash from a changed fold order."
    jit_dump_hashes : bool = false

    @clarg_doc = "Comma-separated x64 CPU features to force on (e.g. avx2,f16c), bypassing detection; LLVM target-feature spellings. Also satisfies cpu_supports-based tune eligibility. Executing a forced instruction the host lacks is an illegal instruction, not a diagnostic."
    jit_x64_force_features : string = ""

    @clarg_doc = "The arm64 twin (e.g. dotprod,i8mm)."
    jit_arm64_force_features : string = ""

    @clarg_default_doc = "unset (the build targets the running box)"
    @clarg_doc = "Build for a CPU class instead of this box: x86-base, x86-avx2, x86-vnni256, x86-vnni512, x86-amx, arm-neon, or arm-i8mm (llvm_cpu_class). The target machine, the emitter's tier gates, requires= eligibility and the shipped-profile ladder all read the class's feature set, so an -exe built here runs on every box of that class; the class must belong to the build's architecture. Environment-only by construction: the AST module cache keys on DAS*, and a flag would serve stamps minted for another class."
    jit_baseline : string = ""
}

let public g_env_jit = env_config(type<JitEnv>)

// ===== Kernel tuning =====

[EnvConfig(env_prefix = "DAS")]
struct public TuneEnv {
    @clarg_default_doc = "normal"
    @clarg_doc = "Compile-time tune mode: normal, tune, or test."
    tune_mode : string = ""

    @clarg_path
    @clarg_default_doc = "<app>.tune.json beside the app"
    @clarg_doc = "The tune sidecar to read and write - how you tune when the application directory is read-only. Setting it also gates the tune policy off (a redirect means manual)."
    tune_manifest : string = ""

    @clarg_path
    @clarg_default_doc = "<home>/.tune-history/<box>/"
    @clarg_doc = "Where mint archives land. Every mint archives (failures kept, marked .FAILED.json); the history is never deleted."
    tune_history : string = ""

    @clarg_default_doc = "2"
    @clarg_doc = "The noise probe's note threshold in percent (a louder probe stamps the mint noisy; refusal sits at max(10, this)) - a calibration lever. Garbage or <= 0 reads as unset."
    tune_noise_cv : float = 0.0

    @clarg_doc = "Mint through the busy-box refusal (a probe cv past the hard ceiling). The sidecar is stamped noise overridden, so the escape always leaves a mark. Validation never refuses - a winner that does not hold is demoted per kernel."
    tune_noise_override : bool = false

    @clarg_default_doc = "normal"
    @clarg_doc = "What a tune shows: silent, normal, or verbose. Anything unrecognized reads as normal, so a typo never silences a tune."
    tune_verbosity : string = ""

    @clarg_default_doc = "declared by [tune_policy]"
    @clarg_doc = "Override the missing-scope policy: fallback, reference, warn, error, auto, or restart. The announce line says when the environment shaped the policy."
    tune_policy : string = ""

    @clarg_path
    @clarg_doc = "A supervisor's stop channel: while the named file exists, tune_interrupt_requested() is true and tuners abort at the next kernel-family boundary without minting. The watchdog sets it and owns the file's lifetime; the measurement in flight always completes."
    tune_control : string = ""

    @clarg_default_doc = "unset (every family races)"
    @clarg_doc = "Comma-separated re-mint filter: a tuner races only kernel families whose name contains one of these tokens, and every skipped family's sidecar entry survives the upsert. Set by --tune-only on the application; the tuner children inherit it."
    tune_only : string = ""

    @clarg_default_doc = "unset (every class this box is in)"
    @clarg_doc = "Runtime pin for a fat exe (DAS_TUNE_MODE=fat): the one CPU class whose kernel clones run, in place of every class cpuid says this box is in. A class the box is not in refuses to start. The A/B lever for the clones an exe carries."
    tune_fat_class : string = ""

    @clarg_default_doc = "0"
    @clarg_doc = "Relaunch depth, armed by the auto policy's re-exec for its child. A relaunched process that still reads a scope as untuned neither tunes nor relaunches again - it reports that the tune did not converge and runs the stamps it has - so a tune that cannot converge costs one restart instead of stacking processes until the box runs out of memory."
    tune_relaunch : int = 0
}

let public g_env_tune = env_config(type<TuneEnv>)

// ===== the generated documentation (ENVIRONMENT.md) =====

// Variables dasLLVM reads but does not own - documented so the coverage scan knows them.
def private ambient_rows() : array<EnvDocRow> {
    return <- [
        EnvDocRow(name = "HOME", kind = "path", dflt = "unset",
            doc = "The tune-history home (USERPROFILE on Windows when unset)."),
        EnvDocRow(name = "USERPROFILE", kind = "path", dflt = "unset",
            doc = "Windows fallback for the tune-history home."),
        EnvDocRow(name = "PROCESSOR_IDENTIFIER", kind = "text", dflt = "unset",
            doc = "Half of the Windows tune box identity (measurements are a property of the box AND its OS)."),
        EnvDocRow(name = "OS", kind = "text", dflt = "unset",
            doc = "The other half of the Windows tune box identity."),
        EnvDocRow(name = "ASAN_OPTIONS", kind = "text", dflt = "unset",
            doc = "The asan runtime's options; the sanitizer tests add exitcode for the children they start, so a stop reads off the exit code."),
        EnvDocRow(name = "UBSAN_OPTIONS", kind = "text", dflt = "unset",
            doc = "The ubsan runtime's options; the sanitizer tests add exitcode for the children they start, as for ASAN_OPTIONS."),
        EnvDocRow(name = "DAS_LOG_LEVEL", kind = "text", dflt = "warning",
            doc = "The runtime's log floor; the sanitizer tests set info for their children, so the IR dump and the announce print.")
    ]
}

//! Renders every declared knob as the `ENVIRONMENT.md` body. `modules/dasLLVM/ENVIRONMENT.md`
//! is this output verbatim - `harness/gen_env_doc.das` writes it and
//! `tests/llvm_env_registry.das` fails when the checked-in file drifts from it.
def public env_markdown() : string {
    return build_string() $(var w) {
        w |> write("# dasLLVM environment variables\n\n")
        w |> write("GENERATED from the `[EnvConfig]` declarations in `daslib/llvm_env.das` - do not edit by\n")
        w |> write("hand. It is regenerated in the daslang source tree (`modules/dasLLVM/harness/\n")
        w |> write("gen_env_doc.das`; `modules/dasLLVM/tests/llvm_env_registry.das` fails on drift and on any\n")
        w |> write("variable read in the module without a declaration).\n\n")
        write_env_doc_semantics(w)
        write_env_section(w, "JIT",
            "Loaded once at context init into `g_env_jit`. The force-features pair exists to reproduce another box's codegen locally or check that a feature-gated kernel is the one being measured; both override `cpu_supports`, which `requires=` on a `[tune_perm]` and `suffix:requires` fallback entries match against.")
        write_env_info_rows(w, get_env_config_info(type<JitEnv>))
        write_env_section(w, "Kernel tuning",
            "Loaded once at context init into `g_env_tune`; tuner children inherit the environment at spawn, and in-process overrides go through the tune setters (`set_tune_manifest_runtime_path`, `tune_set_verbosity`, `tune_set_noise_cv`, `tune_set_noise_override`, `tune_set_history_dir`), which arm the child environment too. The framework itself is documented in `skills/tune.md`.")
        write_env_info_rows(w, get_env_config_info(type<TuneEnv>))
        write_env_section(w, "Ambient variables dasLLVM reads but does not own",
            "Read live at the point of use (`env_value_of`), not loaded at context init.")
        var extras <- ambient_rows()
        for (e in extras) {
            write_env_row(w, e)
        }
        delete extras
    }
}

//! Every declared name, in documentation order - the coverage test consumes this.
def public registered_env_names() : array<string> {
    var out : array<string>
    out |> reserve(16)
    push_info_names(out, get_env_config_info(type<JitEnv>))
    push_info_names(out, get_env_config_info(type<TuneEnv>))
    var extras <- ambient_rows()
    for (e in extras) {
        out |> push(e.name)
    }
    delete extras
    return <- out
}
