# dasLLVM environment variables

GENERATED from the `[EnvConfig]` declarations in `daslib/llvm_env.das` - do not edit by
hand. It is regenerated in the daslang source tree (`modules/dasLLVM/harness/
gen_env_doc.das`; `modules/dasLLVM/tests/llvm_env_registry.das` fails on drift and on any
variable read in the module without a declaration).

Types: **flag** is unset-means-default, `0`/`false`/`off`/`no` (any case) is false and
anything else true; **number** falls back to the default when unset or unparseable, with
a logged warning on garbage; **text** and **path** are taken verbatim. A SET-BUT-EMPTY
variable counts as unset everywhere. Every knob loads ONCE, at context init, into the
`g_env_*` globals - hot code reads struct fields, and `set_env_variable` after startup
is invisible (arm a child process's environment instead).

## JIT

Loaded once at context init into `g_env_jit`. The force-features pair exists to reproduce another box's codegen locally or check that a feature-gated kernel is the one being measured; both override `cpu_supports`, which `requires=` on a `[tune_perm]` and `suffix:requires` fallback entries match against.

| Variable | Type | Default | Effect |
|---|---|---|---|
| `DAS_JIT_PROBE_LTO` | flag | off | Split-JIT LTO probe: partitions emit bitcode and the link runs lld LTO (pass /opt:lldlto=2 plus the CRT /LIBPATHs via --jit-linker-string). Dev instrument for measuring cross-module-inlining recovery; needs --jit-split-modules + --jit-obj-cache=0, announces itself, and folds into the cache keys so a probe artifact never serves a normal run. |
| `DAS_JIT_DUMP_HASHES` | flag | off | Split-JIT key forensics: log every (partition, mangled name, aot hash) the obj-cache chain folds. Diff two runs to locate WHERE a key diverged - distinguishes a changed hash from a changed fold order. |
| `DAS_JIT_X64_FORCE_FEATURES` | text | unset | Comma-separated x64 CPU features to force on (e.g. avx2,f16c), bypassing detection; LLVM target-feature spellings. Also satisfies cpu_supports-based tune eligibility. Executing a forced instruction the host lacks is an illegal instruction, not a diagnostic. |
| `DAS_JIT_ARM64_FORCE_FEATURES` | text | unset | The arm64 twin (e.g. dotprod,i8mm). |
| `DAS_JIT_BASELINE` | text | unset (the build targets the running box) | Build for a CPU class instead of this box: x86-base, x86-avx2, x86-vnni256, x86-vnni512, x86-amx, arm-neon, or arm-i8mm (llvm_cpu_class). The target machine, the emitter's tier gates, requires= eligibility and the shipped-profile ladder all read the class's feature set, so an -exe built here runs on every box of that class; the class must belong to the build's architecture. Environment-only by construction: the AST module cache keys on DAS*, and a flag would serve stamps minted for another class. |

## Kernel tuning

Loaded once at context init into `g_env_tune`; tuner children inherit the environment at spawn, and in-process overrides go through the tune setters (`set_tune_manifest_runtime_path`, `tune_set_verbosity`, `tune_set_noise_cv`, `tune_set_noise_override`, `tune_set_history_dir`), which arm the child environment too. The framework itself is documented in `skills/tune.md`.

| Variable | Type | Default | Effect |
|---|---|---|---|
| `DAS_TUNE_MODE` | text | normal | Compile-time tune mode: normal, tune, or test. |
| `DAS_TUNE_MANIFEST` | path | <app>.tune.json beside the app | The tune sidecar to read and write - how you tune when the application directory is read-only. Setting it also gates the tune policy off (a redirect means manual). |
| `DAS_TUNE_HISTORY` | path | <home>/.tune-history/<box>/ | Where mint archives land. Every mint archives (failures kept, marked .FAILED.json); the history is never deleted. |
| `DAS_TUNE_NOISE_CV` | number | 2 | The noise probe's note threshold in percent (a louder probe stamps the mint noisy; refusal sits at max(10, this)) - a calibration lever. Garbage or <= 0 reads as unset. |
| `DAS_TUNE_NOISE_OVERRIDE` | flag | off | Mint through the busy-box refusal (a probe cv past the hard ceiling). The sidecar is stamped noise overridden, so the escape always leaves a mark. Validation never refuses - a winner that does not hold is demoted per kernel. |
| `DAS_TUNE_VERBOSITY` | text | normal | What a tune shows: silent, normal, or verbose. Anything unrecognized reads as normal, so a typo never silences a tune. |
| `DAS_TUNE_POLICY` | text | declared by [tune_policy] | Override the missing-scope policy: fallback, reference, warn, error, auto, or restart. The announce line says when the environment shaped the policy. |
| `DAS_TUNE_CONTROL` | path | unset | A supervisor's stop channel: while the named file exists, tune_interrupt_requested() is true and tuners abort at the next kernel-family boundary without minting. The watchdog sets it and owns the file's lifetime; the measurement in flight always completes. |
| `DAS_TUNE_ONLY` | text | unset (every family races) | Comma-separated re-mint filter: a tuner races only kernel families whose name contains one of these tokens, and every skipped family's sidecar entry survives the upsert. Set by --tune-only on the application; the tuner children inherit it. |
| `DAS_TUNE_FAT_CLASS` | text | unset (every class this box is in) | Runtime pin for a fat exe (DAS_TUNE_MODE=fat): the one CPU class whose kernel clones run, in place of every class cpuid says this box is in. A class the box is not in refuses to start. The A/B lever for the clones an exe carries. |
| `DAS_TUNE_RELAUNCH` | number | 0 | Relaunch depth, armed by the auto policy's re-exec for its child. A relaunched process that still reads a scope as untuned neither tunes nor relaunches again - it reports that the tune did not converge and runs the stamps it has - so a tune that cannot converge costs one restart instead of stacking processes until the box runs out of memory. |

## Ambient variables dasLLVM reads but does not own

Read live at the point of use (`env_value_of`), not loaded at context init.

| Variable | Type | Default | Effect |
|---|---|---|---|
| `HOME` | path | unset | The tune-history home (USERPROFILE on Windows when unset). |
| `USERPROFILE` | path | unset | Windows fallback for the tune-history home. |
| `PROCESSOR_IDENTIFIER` | text | unset | Half of the Windows tune box identity (measurements are a property of the box AND its OS). |
| `OS` | text | unset | The other half of the Windows tune box identity. |
| `ASAN_OPTIONS` | text | unset | The asan runtime's options; the sanitizer tests add exitcode for the children they start, so a stop reads off the exit code. |
| `UBSAN_OPTIONS` | text | unset | The ubsan runtime's options; the sanitizer tests add exitcode for the children they start, as for ASAN_OPTIONS. |
| `DAS_LOG_LEVEL` | text | warning | The runtime's log floor; the sanitizer tests set info for their children, so the IR dump and the announce print. |
