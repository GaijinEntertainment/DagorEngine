// A standalone -exe carrying [llvm_code] kernels must target the build box (CPU + features),
// decided BEFORE the target-flag pass runs — generic-CPU flags make every feature-gated
// generator decline to its reference body, which is how release exes shipped 11-15x-slower
// scalar kernels while -jit ran the tuned tiers. A kernel-free exe stays generic /
// redistributable. Two witnesses: the BUILD log decision line (prints before the link step,
// so sanitizer-lane link failures cannot flake it), and the selftest host_features probe —
// the generator bakes the resolved feature-rail truth into the exe as a constant, so running
// it distinguishes host (1) from generic (0) from not-generated (-1).
options gen2
options no_aot

require dastest/testing_boost
require daslib/fio
require daslib/strings_boost
require strings
require _llvm_cache_common

let OUTPUT_DIR = "{get_das_root()}/build/tests"
let KERNEL_SCRIPT = "{OUTPUT_DIR}/exe_host_cpu_kernel.das"
let KERNEL_EXE = "{OUTPUT_DIR}/exe_host_cpu_kernel"
let PLAIN_SCRIPT = "{OUTPUT_DIR}/exe_host_cpu_plain.das"
let PLAIN_EXE = "{OUTPUT_DIR}/exe_host_cpu_plain"
let BASELINE_SCRIPT = "{get_das_root()}/modules/dasLLVM/tests/_exe_baseline_intrin.das"
let BASELINE_EXE = "{OUTPUT_DIR}/exe_baseline_intrin"
let BASELINE_ANSWER = "INTRIN=[1496,2480,1496]"   //! the fallback bodies over w = x = 1..16: sum of squares per lane group, the 2x2 block products, the same dot again

def private write_probe_script(path : string; with_kernel : bool) : bool {
    var ok = false
    mkdir_rec(OUTPUT_DIR)
    fopen(path, "w") $(f) {
        if (f != null) {
            fprint(f, "options gen2\n")
            if (with_kernel) {
                fprint(f, "require daslib/tune\n")
                fprint(f, "[llvm_code(name = \"llvm_code_selftest::host_features\")]\n")
                fprint(f, "def host_rail() : int \{ return -1 \}\n")
                fprint(f, "[export] def main() \{ print(\"HOSTF=[\{host_rail()}]\\n\"); \}\n")
            } else {
                fprint(f, "[export] def main() \{ print(\"R=5\\n\"); \}\n")
            }
            ok = true
        }
    }
    return ok
}

// env prefix leads, so no sacrificial-quote wrap is needed (cmd.exe only strips a LEADING quote)
def private neutral_env() : string {
    return (get_platform_name() == "windows" ?
        "set DAS_TUNE_MODE=&& " :
        "DAS_TUNE_MODE= ")
}

def private spawn_child(cmd : string; var lines : array<string>) : int {
    let rc = unsafe(popen_timeout("{cmd} 2>&1", 300.0) $(f) {
        if (f == null) {
            return
        }
        while (!feof(f)) {
            let ln = strip(fgets(f))
            if (!empty(ln)) {
                lines |> push("{ln}")
            }
        }
    })
    return rc
}

def private has(lines : array<string>; needle : string) : bool {
    for (ln in lines) {
        if (find(ln, needle) >= 0) return true
    }
    return false
}

def private plan_targets_host(script : string) : bool {
    var res = false
    compile_for_jit(script, true) $(var program) {
        simulate(program) $(sok, context, _serrors) {
            if (sok) {
                let cfg : JitCliOptions
                let plan <- make_jit_plan(get_ptr(program), get_ptr(context), false, cfg)
                res = plan.use_host_cpu
            }
        }
    }
    return res
}

[test]
def test_exe_host_cpu(t : T?) {
    if (!jit_enabled()) {
        // the decision under test lives in the LLVM backend's exe rail
        t |> success(true)
        return
    }
    let args <- get_command_line_arguments()  // locked view — do not delete
    let bin = args[0]

    t |> success(write_probe_script(KERNEL_SCRIPT, true), "failed to write kernel probe script")
    t |> success(plan_targets_host(KERNEL_SCRIPT), "kernel-carrying exe build must target the host cpu")
    var lk : array<string>
    spawn_child("{neutral_env()}\"{bin}\" -exe -output \"{KERNEL_EXE}\" \"{KERNEL_SCRIPT}\"", lk)

    t |> success(write_probe_script(PLAIN_SCRIPT, false), "failed to write plain probe script")
    t |> success(!plan_targets_host(PLAIN_SCRIPT), "kernel-free exe build must stay generic")

    //! an intrinsic call alone must not promote the exe off the generic baseline - the gates decline to the fallback bodies, so the build emits nothing that machine cannot select
    t |> success(!plan_targets_host(BASELINE_SCRIPT), "an intrinsic call alone never promotes the exe to the host cpu")
    var lb : array<string>
    let rcb = spawn_child("{neutral_env()}\"{bin}\" -exe -output \"{BASELINE_EXE}\" \"{BASELINE_SCRIPT}\"", lb)
    t |> equal(0, rcb, "the intrinsic-calling kernel-free exe must build on the generic baseline: {join(lb, "\n")}")

    // run the built exes where the platform can load the shared lib (same gates as jit_exe.das:
    // Windows lacks RPATH; static builds ship no runtime lib for the exe to call into)
    let can_run = get_platform_name() != "windows" && das_is_dll_build()
    if (can_run && fexist("{KERNEL_EXE}.exe")) {
        var lr : array<string>
        spawn_child("\"{KERNEL_EXE}.exe\"", lr)
        t |> success(lr |> has("HOSTF=[1]"), "the exe must bake the HOST feature rail, not generic (0) or reference (-1)")
    }
    if (can_run && fexist("{BASELINE_EXE}.exe")) {
        var lr : array<string>
        spawn_child("\"{BASELINE_EXE}.exe\"", lr)
        t |> success(lr |> has(BASELINE_ANSWER), "the baseline exe computes the fallback bodies' answers: {join(lr, "\n")}")
    }
}
