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

let OUTPUT_DIR = "{get_das_root()}/build/tests"
let KERNEL_SCRIPT = "{OUTPUT_DIR}/exe_host_cpu_kernel.das"
let KERNEL_EXE = "{OUTPUT_DIR}/exe_host_cpu_kernel"
let PLAIN_SCRIPT = "{OUTPUT_DIR}/exe_host_cpu_plain.das"
let PLAIN_EXE = "{OUTPUT_DIR}/exe_host_cpu_plain"
let HOST_CPU_LINE = "targeting the current box"

def private write_probe_script(path : string; with_kernel : bool) : bool {
    var ok = false
    mkdir_rec(OUTPUT_DIR)
    fopen(path, "w") $(f) {
        if (f != null) {
            fprint(f, "options gen2\n")
            if (with_kernel) {
                fprint(f, "require llvm/daslib/llvm_code\n")
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
    var rc : int
    unsafe {
        rc = popen_timeout("{cmd} 2>&1", 300.0) $(f) {
            if (f == null) {
                return
            }
            while (!feof(f)) {
                let ln = strip(fgets(f))
                if (!empty(ln)) {
                    lines |> push("{ln}")
                }
            }
        }
    }
    return rc
}

def private has(lines : array<string>; needle : string) : bool {
    for (ln in lines) {
        if (find(ln, needle) >= 0) return true
    }
    return false
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
    var lk : array<string>
    spawn_child("{neutral_env()}\"{bin}\" -exe -output \"{KERNEL_EXE}\" \"{KERNEL_SCRIPT}\"", lk)
    t |> success(lk |> has(HOST_CPU_LINE), "kernel-carrying exe build must target the host cpu")

    t |> success(write_probe_script(PLAIN_SCRIPT, false), "failed to write plain probe script")
    var lp : array<string>
    spawn_child("{neutral_env()}\"{bin}\" -exe -output \"{PLAIN_EXE}\" \"{PLAIN_SCRIPT}\"", lp)
    t |> success(!(lp |> has(HOST_CPU_LINE)), "kernel-free exe build must stay generic")

    // run the built exe where the platform can load the shared lib (same gates as jit_exe.das:
    // Windows lacks RPATH; static builds ship no runtime lib for the exe to call into)
    if (get_platform_name() != "windows" && das_is_dll_build() && fexist("{KERNEL_EXE}.exe")) {
        var lr : array<string>
        spawn_child("\"{KERNEL_EXE}.exe\"", lr)
        t |> success(lr |> has("HOSTF=[1]"), "the exe must bake the HOST feature rail, not generic (0) or reference (-1)")
    }
}
