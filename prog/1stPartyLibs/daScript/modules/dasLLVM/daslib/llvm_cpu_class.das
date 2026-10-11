options gen2
options indenting = 4

module llvm_cpu_class shared public

require llvm/daslib/llvm_env
require daslib/strings_boost
require strings

//! The CPU classes a shipped defaults profile is keyed by, with the target each class compiles
//! for. A class is a feature SET every box of the class carries - the same set the minting box
//! stamped into the profile's `features` fingerprint - never a box model. `DAS_JIT_BASELINE`
//! names one, and then the target machine, the emitter's tier gates, `requires=` eligibility and
//! the profile ladder all read the class's set instead of this box's cpuid.

struct public JitCpuClass {
    name : string       // the profile key: `<dir>/<name>.tune-defaults.json`
    arch : string       // `get_architecture_name()` spelling of the arch the class belongs to
    cpu : string        // the LLVM cpu the class's target machine schedules for
    features : string   // ';'-joined LLVM feature names every box of the class has
}

let public JIT_CPU_CLASSES = [
    // the cpu is the arch's bare baseline on every row: a level like x86-64-v3 implies features
    // the set does not name (bmi2, lzcnt, movbe; v4 adds avx512cd/dq), which the backend would emit
    // and cpuid is never asked for - the set alone is what the box is tested for and what LLVM enables
    JitCpuClass(name = "x86-base", arch = "x86_64", cpu = "x86-64", features = "sse4.2"),
    JitCpuClass(name = "x86-avx2", arch = "x86_64", cpu = "x86-64", features = "avx2;f16c;fma;sse4.2"),
    JitCpuClass(name = "x86-vnni256", arch = "x86_64", cpu = "x86-64", features = "avx2;f16c;fma;sse4.2;avxvnni"),
    JitCpuClass(name = "x86-vnni512", arch = "x86_64", cpu = "x86-64",
        features = "avx2;f16c;fma;sse4.2;avx512f;avx512bw;avx512vl;avx512vnni;avx512vbmi"),
    JitCpuClass(name = "x86-amx", arch = "x86_64", cpu = "x86-64",
        features = "avx2;f16c;fma;sse4.2;avxvnni;avx512f;avx512bw;avx512vl;avx512vnni;avx512vbmi;amx-tile;amx-int8"),
    JitCpuClass(name = "arm-neon", arch = "arm64", cpu = "generic", features = "neon;dotprod;fullfp16;lse"),
    JitCpuClass(name = "arm-i8mm", arch = "arm64", cpu = "generic", features = "neon;dotprod;i8mm;bf16;fullfp16;lse")
]

//! The adoption ladder of an arch, most capable class first; "" for an arch with no classes.
def public jit_cpu_class_ladder(arch : string) : string {
    if (arch == "x86_64") {
        return "x86-amx;x86-vnni512;x86-vnni256;x86-avx2;x86-base"
    }
    if (arch == "arm64" || arch == "aarch64") {
        return "arm-i8mm;arm-neon"
    }
    return ""
}

//! The class row named, or a row with an empty name when no class is spelled that way.
def public jit_cpu_class_row(name : string) : JitCpuClass {
    for (c in JIT_CPU_CLASSES) {
        if (c.name == name) {
            return c
        }
    }
    return JitCpuClass()
}

def public jit_cpu_class_names() : string {
    var names <- [for (c in JIT_CPU_CLASSES); c.name]
    let joined = join(names, ", ")
    delete names
    return joined
}

//! True when the class carries the feature (LLVM spelling, as `requires=` and cpuid use).
def public jit_class_has(name, feature : string) : bool {
    for (f in split(jit_cpu_class_row(name).features, ";")) {
        if (f == feature) {
            return true
        }
    }
    return false
}

//! The class's features as an LLVM target-feature append: "+avx2,+f16c,...". "" for no class.
def public jit_class_plus_features(name : string) : string {
    var plus <- [for (f in split(jit_cpu_class_row(name).features, ";")); "+{f}"; where !empty(f)]
    let joined = join(plus, ",")
    delete plus
    return joined
}

//! The baseline class this process builds for - `DAS_JIT_BASELINE` - or "" when the build
//! targets the running box.
def public jit_baseline_class() : string {
    return strip(g_env_jit.jit_baseline)
}

//! True when a baseline is set and its class carries the feature.
def public jit_baseline_has(feature : string) : bool {
    let baseline = jit_baseline_class()
    return !empty(baseline) && jit_class_has(baseline, feature)
}

//! "" when the baseline is unset or names a class of `target_arch`; else the refusal text.
[arch(at = "../ARCHITECTURE_TARGET_FEATURES.md#cpu-classes")]
def public jit_baseline_error(target_arch : string) : string {
    let baseline = jit_baseline_class()
    if (empty(baseline)) {
        return ""
    }
    let row = jit_cpu_class_row(baseline)
    if (empty(row.name)) {
        return "DAS_JIT_BASELINE={baseline} names no CPU class - the classes are {jit_cpu_class_names()}"
    }
    if (row.arch != target_arch) {
        return "DAS_JIT_BASELINE={baseline} is a {row.arch} class, but the build targets {target_arch} - a baseline never crosses architectures (use --jit-target for that)"
    }
    return ""
}
